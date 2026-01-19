#include "hvlm/core/triangle.hpp"
#pragma once
#include "geometry.hpp"
#include <algorithm>
#include <stack>

struct BVHNode { Vec3 minb,maxb; int left,right; bool leaf; int start,count; BVHNode(){left=right=-1; leaf=false; start=0; count=0;} };
struct BVH {
    std::vector<BVHNode> nodes;
    std::vector<Triangle>* tris;
    void build(std::vector<Triangle>& triangles){
        tris = &triangles;
        std::vector<int> ids(triangles.size());
        for(size_t i=0;i<ids.size();++i) ids[i]=int(i);
        nodes.clear();
        build_node(ids,0,(int)ids.size());
    }
    int build_node(std::vector<int>& ids,int start,int end){
        BVHNode node;
        Vec3 minb(1e30f,1e30f,1e30f), maxb(-1e30f,-1e30f,-1e30f);
        for(int i=start;i<end;i++){
            Triangle &t = (*tris)[ids[i]];
            minb.x = std::fmin(minb.x, t.minb.x); minb.y = std::fmin(minb.y, t.minb.y); minb.z = std::fmin(minb.z, t.minb.z);
            maxb.x = std::fmax(maxb.x, t.maxb.x); maxb.y = std::fmax(maxb.y, t.maxb.y); maxb.z = std::fmax(maxb.z, t.maxb.z);
        }
        node.minb = minb; node.maxb = maxb;
        int myIndex = (int)nodes.size();
        nodes.push_back(node);
        int count = end-start;
        if(count <= 4){
            nodes[myIndex].leaf = true;
            nodes[myIndex].start = start;
            nodes[myIndex].count = count;
            return myIndex;
        }
        Vec3 ext = subv(maxb,minb);
        int axis = (ext.x>ext.y && ext.x>ext.z) ? 0 : (ext.y>ext.z?1:2);
        std::sort(ids.begin()+start, ids.begin()+end, [&](int a,int b){
            float ca = (((&(*tris)[a].minb.x)[axis] + (&(*tris)[a].maxb.x)[axis]) * 0.5f);
            float cb = (((&(*tris)[b].minb.x)[axis] + (&(*tris)[b].maxb.x)[axis]) * 0.5f);
            return ca < cb;
        });
        int mid = start + count/2;
        int left = build_node(ids,start,mid);
        int right = build_node(ids,mid,end);
        nodes[myIndex].left = left; nodes[myIndex].right = right; nodes[myIndex].leaf = false;
        return myIndex;
    }

    bool intersect_aabb(const BVHNode &node, const Ray &r){
        float tmin = r.tmin, tmax = r.tmax;
        for(int ax=0;ax<3;ax++){
            float invD = 1.0f / ((&r.d.x)[ax]);
            float t0 = (((&node.minb.x)[ax]) - (&r.o.x)[ax]) * invD;
            float t1 = (((&node.maxb.x)[ax]) - (&r.o.x)[ax]) * invD;
            if(invD < 0.0f) std::swap(t0,t1);
            tmin = t0>tmin? t0: tmin;
            tmax = t1<tmax? t1: tmax;
            if(tmax <= tmin) return false;
        }
        return true;
    }

    bool traverse_hit(const Ray &r, float &tOut, Vec3 &normalOut, int &matIdOut){
        if(nodes.empty() || !tris) return false;
        bool hit=false; tOut = r.tmax;
        std::vector<int> stack; stack.push_back(0);
        while(!stack.empty()){
            int ni = stack.back(); stack.pop_back();
            const BVHNode &node = nodes[ni];
            if(!intersect_aabb(node, r)) continue;
            if(node.leaf){
                for(int i=0;i<node.count;i++){
                    const Triangle &tri = (*tris)[node.start + i];
                    // Möller–Trumbore
                    Vec3 e1 = subv(tri.v1, tri.v0);
                    Vec3 e2 = subv(tri.v2, tri.v0);
                    Vec3 p = crossp(r.d, e2);
                    float det = dot(e1,p);
                    if(fabs(det) < 1e-8f) continue;
                    float inv = 1.0f/det;
                    Vec3 tvec = subv(r.o, tri.v0);
                    float u = dot(tvec,p)*inv;
                    if(u<0.0f||u>1.0f) continue;
                    Vec3 q = crossp(tvec,e1);
                    float v = dot(r.d,q)*inv;
                    if(v<0.0f||u+v>1.0f) continue;
                    float t = dot(e2,q)*inv;
                    if(t <= r.tmin || t >= tOut) continue;
                    tOut = t;
                    Vec3 pnt = add(r.o, mul(r.d, t));
                    normalOut = normalize(crossp(e1,e2));
                    matIdOut = tri.material_id;
                    hit = true;
                }
            } else {
                stack.push_back(node.left);
                stack.push_back(node.right);
            }
        }
        return hit;
    }
};
