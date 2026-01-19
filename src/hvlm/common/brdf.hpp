#pragma once
#include "../core/vec3.hpp"
#include "../common/material.hpp"

inline Vec3 eval_brdf(const Material& m) {
    return m.albedo * (1.0f / 3.14159265f);
}
