#include "hvlm/core/triangle.hpp"
#pragma once

#include "hvlm/core/vec3.hpp"
#include <algorithm>

struct Triangle {
    Vec3 v0;
    Vec3 v1;
    Vec3 v2;

    Vec3 minb;
    Vec3 maxb;

    int material_id = 0;

    inline void compute_bounds() {
        minb.x = std::min({v0.x, v1.x, v2.x});
        minb.y = std::min({v0.y, v1.y, v2.y});
        minb.z = std::min({v0.z, v1.z, v2.z});

        maxb.x = std::max({v0.x, v1.x, v2.x});
        maxb.y = std::max({v0.y, v1.y, v2.y});
        maxb.z = std::max({v0.z, v1.z, v2.z});
    }
};
