#pragma once
#include "ray.hpp"
#include "../common/material.hpp"
#include <array>

static inline std::array<unsigned char,3> shade(const Ray&r,const Material&m){
    float t = 0.5f*(r.d[1]+1);
    std::array<unsigned char,3> o;
    o[0] = (unsigned char)(255*(m.albedo[0]*(0.3+0.7*t)));
    o[1] = (unsigned char)(255*(m.albedo[1]*(0.3+0.7*t)));
    o[2] = (unsigned char)(255*(m.albedo[2]*(0.3+0.7*t)));
    return o;
}
