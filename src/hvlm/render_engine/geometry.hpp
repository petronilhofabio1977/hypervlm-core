#include "hvlm/core/triangle.hpp"
#pragma once
#include "util.hpp"
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>


    Vec3 v0,v1,v2;
    Vec3 n;
    int material_id;
    Vec3 minb,maxb;
    void compute_bounds(){
        minb = Vec3(std::fmin(std::fmin(v0.x,v1.x),v2.x), std::fmin(std::fmin(v0.y,v1.y),v2.y), std::fmin(std::fmin(v0.z,v1.z),v2.z));
        maxb = Vec3(std::fmax(std::fmax(v0.x,v1.x),v2.x), std::fmax(std::fmax(v0.y,v1.y),v2.y), std::fmax(std::fmax(v0.z,v1.z),v2.z));
        n = normalize(crossp(subv(v1,v0), subv(v2,v0)));
    }
};

struct Mesh {
    std::vector<Triangle> tris;
    bool load_obj(const std::string &path){
        std::ifstream f(path);
        if(!f){ std::cerr<<"OBJ open fail: "<<path<<"\n"; return false; }
        std::string ln;
        std::vector<Vec3> pos;
        while(std::getline(f,ln)){
            if(ln.size()<2) continue;
            std::istringstream ss(ln);
            std::string tag; ss>>tag;
            if(tag=="v"){ float x,y,z; ss>>x>>y>>z; pos.emplace_back(x,y,z); }
            else if(tag=="f"){
                std::vector<int> idx; std::string tok;
                while(ss>>tok){
                    size_t p=tok.find('/');
                    int vi = std::stoi(p==std::string::npos? tok : tok.substr(0,p));
                    idx.push_back(vi-1);
                }
                if(idx.size()<3) continue;
                for(size_t i=1;i+1<idx.size();++i){
                    Triangle t; t.v0=pos[idx[0]]; t.v1=pos[idx[i]]; t.v2=pos[idx[i+1]]; t.material_id=0; t.compute_bounds(); tris.push_back(t);
                }
            }
        }
        return true;
    }
};

struct Sphere { Vec3 center; float radius; int material_id; };

    Vec3 oc = subv(r.o, s.center);
    float a = dot(r.d, r.d);
    float b = 2.0f * dot(oc, r.d);
    float c = dot(oc, oc) - s.radius * s.radius;
    float disc = b*b - 4*a*c;
    if(disc < 0.0f) return false;
    float sq = sqrtf(disc);
    float t0 = (-b - sq) / (2*a);
    float t1 = (-b + sq) / (2*a);
    float t = t0;
    if(t < r.tmin || t > r.tmax) t = t1;
    if(t < r.tmin || t > r.tmax) return false;
    tOut = t;
    Vec3 p = add(r.o, mul(r.d, t));
    nOut = normalize(subv(p, s.center));
    return true;
}
