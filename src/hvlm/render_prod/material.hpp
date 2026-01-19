#pragma once
#include "util.hpp"
#include <algorithm>

// Schlick Fresnel
inline Vec3 fresnel_schlick(const Vec3 &F0, float cosTheta){
    float f = powf(1.0f - cosTheta, 5.0f);
    return add(mul(F0, 1.0f - f), mul(Vec3(1,1,1), f));
}

inline Vec3 eval_brdf(const Material &m, const Vec3 &N, const Vec3 &V, const Vec3 &L){
// REMOVED_UNUSED:     float NdotV = std::max(0.0f, dot(N,V));
    Vec3 diffuse = mulv(m.albedo, Vec3(1.0f/3.14159265f,1.0f/3.14159265f,1.0f/3.14159265f));
    Vec3 F = fresnel_schlick(F0, std::max(0.0f, dot(normalize(add(V,L)), V)));
    float NdotH = std::max(0.0f, dot(N, normalize(add(V,L))));
    Vec3 spec = mul(F, powf(NdotH, specPower));
    // mix
    return out;
}
