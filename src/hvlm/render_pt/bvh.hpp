#include "hvlm/core/triangle.hpp"
#pragma once
#include "scene_geo.hpp"
#include <vector>
#include <algorithm>
#include <cstdint>

struct BVHNode {
    Vec3 minb, maxb;
    int left, right; // indices (if leaf, left = first triangle index, right = tri count)
    bool leaf;
    int tri_idx;
    void init(){ left=right=-1; leaf=false; tri_idx=-1; }
};

struct BVH {
    std::vector<BVHNode> nodes;
    std::vector<Triangle> *tris;
    void build(std::vector<Triangle>& triangles){
        tris = &triangles;
        nodes.clear();
        std::vector<int> ids(triangles.size());
        for(size_t i=0;i<ids.size();++i) ids[i]=int(i);
        build_node(ids,0,ids.size());
    }
    int build_node(std::vector<int>& ids,int start,int end){
        BVHNode node; node.init();
        Vec3 minb(1e30f,1e30f,1e30f), maxb(-1e30f,-1e30f,-1e30f);
        for(int i=start;i<end;i++){
            Triangle &t = (*tris)[ids[i]];
            minb.x = std::min(minb.x, t.minb.x); minb.y = std::min(minb.y, t.minb.y); minb.z = std::min(minb.z, t.minb.z);
            maxb.x = std::max(maxb.x, t.maxb.x); maxb.y = std::max(maxb.y, t.maxb.y); maxb.z = std::max(maxb.z, t.maxb.z);
        }
        node.minb = minb; node.maxb = maxb;
        int myIndex = nodes.size();
        nodes.push_back(node);
        int count = end-start;
        if(count<=4){
            nodes[myIndex].leaf = true;
            nodes[myIndex].left = start;
            nodes[myIndex].right = count;
            return myIndex;
        }
        // split by longest axis
        Vec3 ext = subv(maxb,minb);
        int axis = (ext.x>ext.y && ext.x>ext.z)?0: (ext.y>ext.z?1:2);
        std::sort(ids.begin()+start, ids.begin()+end, [&](int a,int b){
            float ca = ( (&(*tris)[a].minb.x)[axis] + (&(*tris)[a].maxb.x)[axis]) * 0.5f;
            float cb = ( (&(*tris)[b].minb.x)[axis] + (&(*tris)[b].maxb.x)[axis]) * 0.5f;
            return ca < cb;
        });
        int mid = start + count/2;
        int leftChild = build_node(ids,start,mid);
        int rightChild = build_node(ids,mid,end);
        nodes[myIndex].left = leftChild;
        nodes[myIndex].right = rightChild;
        nodes[myIndex].leaf = false;
        return myIndex;
    }
};
