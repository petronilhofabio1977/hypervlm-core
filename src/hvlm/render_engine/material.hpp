#pragma once
#include "util.hpp"
#include <cmath>

// Schlick Fresnel
inline Vec3 fresnel_schlick(const Vec3 &F0, float cosTheta){
    float f = powf(1.0f - cosTheta, 5.0f);
    return add(mul(F0, 1.0f - f), mul(Vec3(1,1,1), f));
}

    float a2 = a*a;
    float denom = (NdotH*NdotH)*(a2 - 1.0f) + 1.0f;
    return a2 / (PI_F * denom * denom + 1e-12f);
}

// Smith geometry (approx)
    float gv = NdotV/(NdotV*(1.0f - k) + k + 1e-12f);
    return gv * gl;
}

inline Vec3 eval_brdf(const Material &m, const Vec3 &N, const Vec3 &V, const Vec3 &L){
    Vec3 H = normalize(add(V,L));
    float NdotV = std::max(0.0f, dot(N,V));
    float NdotH = std::max(0.0f, dot(N,H));
    float VdotH = std::max(0.0f, dot(V,H));
    // diffuse (Burley)
    Vec3 diffuse = mulv(m.albedo, Vec3(1.0f/PI_F,1.0f/PI_F,1.0f/PI_F));
    Vec3 F = fresnel_schlick(F0, VdotH);
    return out;
}
