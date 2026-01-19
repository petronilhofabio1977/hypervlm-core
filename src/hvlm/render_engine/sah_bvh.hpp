#include "hvlm/core/triangle.hpp"
#pragma once
#include "geometry.hpp"
#include <vector>
#include <algorithm>
#include <limits>
#include <cstdint>

struct BVHNode { Vec3 minb,maxb; int left,right; int start,count; bool leaf; BVHNode():left(-1),right(-1),start(0),count(0),leaf(false){} };
struct BVH {
    std::vector<BVHNode> nodes;
    std::vector<Triangle>* tris;
    void clear(){ nodes.clear(); tris=nullptr; }
    void build(std::vector<Triangle> &triangles){
        tris = &triangles;
        nodes.clear();
        std::vector<int> ids(triangles.size());
        for(size_t i=0;i<ids.size();++i) ids[i]=int(i);
        build_node(ids,0,(int)ids.size());
    }
private:
    int build_node(std::vector<int>& ids, int start, int end){
        BVHNode node;
        Vec3 minb(1e30f,1e30f,1e30f), maxb(-1e30f,-1e30f,-1e30f);
        for(int i=start;i<end;++i){
            Triangle &t = (*tris)[ids[i]];
            minb.x = std::fmin(minb.x, t.minb.x); minb.y = std::fmin(minb.y, t.minb.y); minb.z = std::fmin(minb.z, t.minb.z);
            maxb.x = std::fmax(maxb.x, t.maxb.x); maxb.y = std::fmax(maxb.y, t.maxb.y); maxb.z = std::fmax(maxb.z, t.maxb.z);
        }
        node.minb=minb; node.maxb=maxb;
        int my = (int)nodes.size();
        nodes.push_back(node);
        int count = end - start;
        if(count <= 4){
            nodes[my].leaf = true;
            nodes[my].start = start;
            nodes[my].count = count;
            return my;
        }
        // SAH partition (approx via bucket)
        int axis = 0;
        Vec3 ext = subv(maxb,minb);
        if(ext.y > ext.x && ext.y > ext.z) axis=1;
        else if(ext.z > ext.x && ext.z > ext.y) axis=2;
        const int NB = 16;
        struct Bucket{ int count=0; Vec3 minb{1e30f,1e30f,1e30f}; Vec3 maxb{-1e30f,-1e30f,-1e30f}; };
        Bucket buckets[NB];
        for(int i=start;i<end;++i){
            Triangle &t = (*tris)[ids[i]];
            float centroid = (((&t.minb.x)[axis] + (&t.maxb.x)[axis]) * 0.5f);
            float rel = (centroid - (&minb.x)[axis]) / ((&ext.x)[axis] + 1e-12f);
            int b = int(clampf(rel * NB, 0.0f, (float)(NB-1)));
            buckets[b].count++;
            buckets[b].minb.x = std::fmin(buckets[b].minb.x, t.minb.x); buckets[b].minb.y = std::fmin(buckets[b].minb.y, t.minb.y); buckets[b].minb.z = std::fmin(buckets[b].minb.z, t.minb.z);
            buckets[b].maxb.x = std::fmax(buckets[b].maxb.x, t.maxb.x); buckets[b].maxb.y = std::fmax(buckets[b].maxb.y, t.maxb.y); buckets[b].maxb.z = std::fmax(buckets[b].maxb.z, t.maxb.z);
        }
        // compute best split
        float bestCost = 1e30f; int best = 0;
        for(int i=1;i<NB;i++){
            Vec3 minL(1e30f,1e30f,1e30f), maxL(-1e30f,-1e30f,-1e30f); int countL=0;
            for(int j=0;j<i;j++){ if(buckets[j].count==0) continue; countL+=buckets[j].count; minL.x = std::fmin(minL.x, buckets[j].minb.x); minL.y = std::fmin(minL.y, buckets[j].minb.y); minL.z = std::fmin(minL.z, buckets[j].minb.z); maxL.x = std::fmax(maxL.x, buckets[j].maxb.x); maxL.y = std::fmax(maxL.y, buckets[j].maxb.y); maxL.z = std::fmax(maxL.z, buckets[j].maxb.z); }
            Vec3 minR(1e30f,1e30f,1e30f), maxR(-1e30f,-1e30f,-1e30f); int countR=0;
            for(int j=i;j<NB;j++){ if(buckets[j].count==0) continue; countR+=buckets[j].count; minR.x = std::fmin(minR.x, buckets[j].minb.x); minR.y = std::fmin(minR.y, buckets[j].minb.y); minR.z = std::fmin(minR.z, buckets[j].minb.z); maxR.x = std::fmax(maxR.x, buckets[j].maxb.x); maxR.y = std::fmax(maxR.y, buckets[j].maxb.y); maxR.z = std::fmax(maxR.z, buckets[j].maxb.z); }
            if(countL==0 || countR==0) continue;
            Vec3 extL = subv(maxL,minL); Vec3 extR = subv(maxR,minR);
            float areaL = 2.0f*(extL.x*extL.y + extL.y*extL.z + extL.z*extL.x);
            float areaR = 2.0f*(extR.x*extR.y + extR.y*extR.z + extR.z*extR.x);
            float cost = 0.125f + (areaL*countL + areaR*countR); // approximate SAH term; normalization not necessary for argmin
            if(cost < bestCost){ bestCost = cost; best = i; }
        }
        // partition by bucket "best"
        auto midIt = std::partition(ids.begin()+start, ids.begin()+end, [&](int id){
            Triangle &t = (*tris)[id];
            float centroid = (((&t.minb.x)[axis] + (&t.maxb.x)[axis]) * 0.5f);
            float rel = (centroid - (&minb.x)[axis]) / ((&ext.x)[axis] + 1e-12f);
            int b = int(clampf(rel * NB, 0.0f, (float)(NB-1)));
            return b < best;
        });
        int mid = start + int(std::distance(ids.begin()+start, midIt));
        if(mid==start || mid==end){
            // fallback median
            mid = start + count/2;
            std::nth_element(ids.begin()+start, ids.begin()+mid, ids.begin()+end, [&](int a,int b){
                float ca = (((&(*tris)[a].minb.x)[axis] + (&(*tris)[a].maxb.x)[axis]) * 0.5f);
                float cb = (((&(*tris)[b].minb.x)[axis] + (&(*tris)[b].maxb.x)[axis]) * 0.5f);
                return ca < cb;
            });
        }
        int left = build_node(ids, start, mid);
        int right = build_node(ids, mid, end);
        nodes[my].left = left; nodes[my].right = right;
        nodes[my].leaf = false;
        return my;
    }

public:
    bool intersect(const Ray &r, float &tOut, Vec3 &normalOut, int &matOut){
        if(nodes.empty() || !tris) return false;
        tOut = r.tmax; bool hit=false;
        std::vector<int> stack; stack.push_back(0);
        while(!stack.empty()){
            int ni = stack.back(); stack.pop_back();
            const BVHNode &node = nodes[ni];
            // AABB slab
            float tmin = r.tmin, tmax = r.tmax;
            for(int ax=0;ax<3;ax++){
                float invD = 1.0f / ((&r.d.x)[ax]);
                float t0 = (((&node.minb.x)[ax]) - (&r.o.x)[ax]) * invD;
                float t1 = (((&node.maxb.x)[ax]) - (&r.o.x)[ax]) * invD;
                if(invD < 0.0f) std::swap(t0,t1);
                tmin = t0>tmin? t0: tmin;
                tmax = t1<tmax? t1: tmax;
                if(tmax <= tmin) goto cont_loop;
            }
            if(node.leaf){
                for(int i=0;i<node.count;i++){
                    const Triangle &tri = (*tris)[node.start + i];
                    Vec3 e1 = subv(tri.v1, tri.v0);
                    Vec3 e2 = subv(tri.v2, tri.v0);
                    Vec3 p = crossp(r.d, e2);
                    float det = dot(e1,p);
                    if(fabs(det) < 1e-8f) continue;
                    float inv = 1.0f / det;
                    Vec3 tvec = subv(r.o, tri.v0);
                    float u = dot(tvec,p) * inv;
                    if(u < 0.0f || u > 1.0f) continue;
                    Vec3 q = crossp(tvec, e1);
                    float v = dot(r.d,q) * inv;
                    if(v < 0.0f || u+v > 1.0f) continue;
                    float t = dot(e2, q) * inv;
                    if(t <= r.tmin || t >= tOut) continue;
                    tOut = t;
                    normalOut = normalize(crossp(e1,e2));
                    matOut = tri.material_id;
                    hit = true;
                }
            } else {
                stack.push_back(node.left);
                stack.push_back(node.right);
            }
            cont_loop: ;
        }
        return hit;
    }
};
