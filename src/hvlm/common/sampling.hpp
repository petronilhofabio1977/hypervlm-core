#pragma once
#include <cmath>
#include <cstdlib>
#include "../core/vec3.hpp"

inline float rand01() {
    return rand() / (float)RAND_MAX;
}

inline Vec3 cosine_hemisphere(const Vec3& n) {
    float r1 = rand01();
    float r2 = rand01();
    float phi = 2.0f * 3.14159265f * r1;

    float x = cosf(phi) * sqrtf(r2);
    float y = sinf(phi) * sqrtf(r2);
    float z = sqrtf(1.0f - r2);

    Vec3 T = fabs(n.x) > 0.1f ? Vec3(0,1,0) : Vec3(1,0,0);
    T = normalize(crossp(T, n));
    Vec3 B = crossp(n, T);

    return normalize(T*x + B*y + n*z);
}
