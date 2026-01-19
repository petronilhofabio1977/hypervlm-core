#pragma once
#include <cmath>
#include <random>
#include <chrono>
#include <algorithm>
#include <cstdint>
struct Vec3 { float x,y,z; Vec3():x(0),y(0),z(0){} Vec3(float a,float b,float c):x(a),y(b),z(c){} };
inline Vec3 add(const Vec3&a,const Vec3&b){return Vec3(a.x+b.x,a.y+b.y,a.z+b.z);}

struct Ray { Vec3 o,d; float tmin,tmax; Ray():o(),d(){ tmin=1e-4f; tmax=1e30f; } Ray(const Vec3&o_,const Vec3&d_):o(o_),d(d_){ tmin=1e-4f; tmax=1e30f; } };
inline Vec3 subv(const Vec3&a,const Vec3&b){return Vec3(a.x-b.x,a.y-b.y,a.z-b.z);}
inline Vec3 mul(const Vec3&a,float s){return Vec3(a.x*s,a.y*s,a.z*s);}
inline Vec3 mulv(const Vec3&a,const Vec3&b){return Vec3(a.x*b.x,a.y*b.y,a.z*b.z);}
inline float dot(const Vec3&a,const Vec3&b){return a.x*b.x + a.y*b.y + a.z*b.z;}
inline Vec3 crossp(const Vec3&a,const Vec3&b){ return Vec3(a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x); }
inline float len(const Vec3&a){return std::sqrt(dot(a,a));}
inline Vec3 normalize(const Vec3&a){ float L=len(a); return L>0 ? mul(a,1.0f/L) : a; }
inline Vec3 clamp01(const Vec3&a){ return Vec3(std::fmin(1.0f,std::fmax(0.0f,a.x)), std::fmin(1.0f,std::fmax(0.0f,a.y)), std::fmin(1.0f,std::fmax(0.0f,a.z))); }
static inline float rand01f() {
    static thread_local std::mt19937 gen((unsigned)std::chrono::high_resolution_clock::now().time_since_epoch().count());
    return std::uniform_real_distribution<float>(0.0f,1.0f)(gen);
}
constexpr float PI_F = 3.14159265358979323846f;
inline float clampf(float v,float a,float b){ return v<a? a: (v>b? b: v); }
