#include "hvlm/core/triangle.hpp"
#pragma once
#include <vector>
#include <array>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>
#include <limits>
#include <algorithm>
#include <random>

struct Vec3 { float x,y,z; Vec3():x(0),y(0),z(0){} Vec3(float a,float b,float c):x(a),y(b),z(c){} };
static inline Vec3 add(const Vec3&a,const Vec3&b){return Vec3(a.x+b.x,a.y+b.y,a.z+b.z);}
static inline Vec3 subv(const Vec3&a,const Vec3&b){return Vec3(a.x-b.x,a.y-b.y,a.z-b.z);}
static inline Vec3 mul(const Vec3&a,float s){return Vec3(a.x*s,a.y*s,a.z*s);}
static inline Vec3 mulv(const Vec3&a,const Vec3&b){return Vec3(a.x*b.x,a.y*b.y,a.z*b.z);}
static inline float dot(const Vec3&a,const Vec3&b){return a.x*b.x + a.y*b.y + a.z*b.z;}
static inline Vec3 crossp(const Vec3&a,const Vec3&b){return Vec3(a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x);}
static inline float len(const Vec3&a){return std::sqrt(dot(a,a));}
static inline Vec3 normalize(const Vec3&a){ float L=len(a); return L>0?mul(a,1.0f/L):a; }

struct Ray { Vec3 o,d; float tmin,tmax; Ray(const Vec3&o_,const Vec3&d_):o(o_),d(d_){tmin=1e-4f; tmax=1e30f;} };

    Vec3 v0,v1,v2;
    Vec3 n0;
    int material_id;
    // bounding box
    Vec3 minb, maxb;
    void compute_bounds(){
        minb = Vec3(std::min({v0.x,v1.x,v2.x}), std::min({v0.y,v1.y,v2.y}), std::min({v0.z,v1.z,v2.z}));
        maxb = Vec3(std::max({v0.x,v1.x,v2.x}), std::max({v0.y,v1.y,v2.y}), std::max({v0.z,v1.z,v2.z}));
    }
};

struct Mesh {
    std::vector<Vec3> positions;
    std::vector<Triangle> tris;
    std::vector<Material> materials;
    bool load_obj(const std::string&path){
        std::ifstream f(path);
        if(!f) { std::cerr<<"OBJ open fail: "<<path<<"\n"; return false; }
        std::string ln;
        std::vector<Vec3> tempv;
        while(std::getline(f,ln)){
            if(ln.size()<2) continue;
            std::istringstream ss(ln);
            std::string tag; ss>>tag;
            if(tag=="v"){ float x,y,z; ss>>x>>y>>z; tempv.emplace_back(x,y,z); }
            else if(tag=="f"){
                int a,b,c; char ch;
                ss >> a >> ch;
                if(ss.fail()){ // simple tri indices
                    ss.clear(); ss.str(ln); ss>>tag>>a>>b>>c;
                } else {
                    // possible v/vt/vn, handle first int
                    // fallback: parse line tokens to pick vertex indices
                    ss.clear(); ss.str(ln); std::vector<int> idx;
                    std::string tok;
                    ss>>tok; // skip 'f'
                    std::istringstream ls(ln.substr(2));
                    while(ls>>tok){
                        size_t p = tok.find('/');
                        int vi = std::stoi(p==std::string::npos?tok:tok.substr(0,p));
                        idx.push_back(vi);
                    }
                    if(idx.size()>=3){ a=idx[0]; b=idx[1]; c=idx[2]; }
                    else continue;
                }
                // OBJ indices are 1-based
                Triangle T;
                T.v0 = tempv[a-1]; T.v1 = tempv[b-1]; T.v2 = tempv[c-1];
                T.material_id = 0;
                T.compute_bounds();
                tris.push_back(T);
            }
        }
        positions = tempv;
        return true;
    }
};
