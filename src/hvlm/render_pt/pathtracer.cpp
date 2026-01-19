#include "bvh.hpp"
#include "../common/material.hpp"
#include "../common/brdf.hpp"
#include "../common/sampling.hpp"

Vec3 trace_path(const BVH& bvh,
                const std::vector<Material>& mats,
                Ray ray)
{
    Vec3 radiance(0,0,0);
    Vec3 throughput(1,1,1);

    for(int bounce = 0; bounce < 8; ++bounce) {
        Hit hit;
        if(!traverse_bvh_hit(bvh, ray, hit))
            break;

        const Material& m = mats[hit.mat_id];

        radiance += throughput * m.emission;

        Vec3 dir = cosine_hemisphere(hit.n);
        ray = Ray(hit.p + hit.n * 1e-4f, dir);

        throughput *= eval_brdf(m);

        if(bounce > 3) {
            float p = fmax(throughput.x,
                      fmax(throughput.y, throughput.z));
            if(rand01() > p) break;
            throughput /= p;
        }
    }

    return radiance;
}
