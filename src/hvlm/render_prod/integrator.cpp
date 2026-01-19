#include "hvlm/core/triangle.hpp"
int main_prod(int W, int H);
#include "util.hpp"
#include "geometry.hpp"
#include "bvh.hpp"
#include "../common/material.hpp"
#include "denoise.hpp"
#include <vector>
#include <iostream>
#include <fstream>
#include <atomic>
#include <thread>

static inline float balance_weight(float pdf_a, float pdf_b){ float a = pdf_a*pdf_a; float b = pdf_b*pdf_b; return a/(a+b+1e-12f); }

struct Camera {
    Vec3 pos, look, up; float fov, aspect, aperture, focal;
    Camera(){ pos=Vec3(0,0,-4); look=Vec3(0,0,1); up=Vec3(0,1,0); fov=45.0f; aspect=2.0f; aperture=0.0f; focal=4.0f; }
    Ray generate(float px,float py,int W,int H){
        float u = (2.0f*(px+rand01f())/W - 1.0f) * tanf(fov * 0.5f * 3.14159265f/180.0f) * aspect;
        float v = (1.0f - 2.0f*(py+rand01f())/H) * tanf(fov * 0.5f * 3.14159265f/180.0f);
        Vec3 dir = normalize(Vec3(u,v,1.0f));
        Ray r(pos, dir);
        if(aperture > 1e-6f){
            Vec3 focalp = add(pos, mul(dir, focal));
            float rth = sqrtf(rand01f()); float theta = 2.0f*3.14159265f*rand01f();
            float rx = rth * cosf(theta) * aperture * 0.5f;
            float ry = rth * sinf(theta) * aperture * 0.5f;
            Vec3 offset = add(mul(normalize(crossp(up, dir)), rx), mul(up, ry));
            r.o = add(r.o, offset);
            r.d = normalize(subv(focalp, r.o));
        }
        return r;
    }
};

static Vec3 eval_environment(const Ray &r){
    float t = 0.5f * (r.d.y + 1.0f);
    return add(mul(Vec3(0.7f,0.8f,1.0f), t), mul(Vec3(0.2f,0.2f,0.25f), 1.0f-t));
}

// scene globals (simple)
static std::vector<Triangle> g_tris;
static std::vector<Material> g_mats;
static BVH g_bvh;
static std::vector<Sphere> g_spheres;

static Vec3 shade_path(const Ray &init, int maxbounces=6){
    Vec3 throughput(1,1,1), accum(0,0,0);
    Ray ray = init;
    for(int bounce=0;bounce<maxbounces;++bounce){
        // first check spheres
        float tSphere=1e30f; Vec3 nSphere; int sphMat=-1; bool sphHit=false;
        for(size_t i=0;i<g_spheres.size();++i){
            float tS; Vec3 nS;
                if(tS < tSphere){ tSphere = tS; nSphere = nS; sphMat = g_spheres[i].material_id; sphHit = true; }
            }
        }
        // then BVH
        float tTri; Vec3 nTri; int triMat=-1;
        bool triHit = g_bvh.traverse_hit(ray, tTri, nTri, triMat);
        float tHit = 1e30f; Vec3 N; int matId=-1;
        if(sphHit && (!triHit || tSphere < tTri)){ tHit = tSphere; N = nSphere; matId = sphMat; }
        else if(triHit){ tHit = tTri; N = nTri; matId = triMat; }
        if(matId == -1){
            accum = add(accum, mulv(throughput, eval_environment(ray)));
            break;
        }
        // fetch material
        Material M = g_mats[matId];
        // emission
        if(M.emission.x>0.0f || M.emission.y>0.0f || M.emission.z>0.0f){
            accum = add(accum, mulv(throughput, M.emission));
            break;
        }
        Vec3 P = add(ray.o, mul(ray.d, tHit));
        Vec3 V = normalize(mul(ray.d, -1.0f));
        // sample hemisphere cosine (BSDF sampling)
        float r1 = rand01f(), r2 = rand01f();
        float phi = 2.0f*3.14159265f*r1;
        float cosTheta = sqrtf(1.0f - r2);
        float sinTheta = sqrtf(1.0f - cosTheta*cosTheta);
        Vec3 tangent = fabs(N.x) > 0.1f ? Vec3(0,1,0) : Vec3(1,0,0);
        Vec3 bitangent = normalize(crossp(N,tangent));
        Vec3 dir = normalize(add(add(mul(tangent, cosf(phi)*sinTheta), mul(bitangent, sinf(phi)*sinTheta)), mul(N, cosTheta)));
        Ray newRay(add(P, mul(dir, 1e-4f)), dir);
        Vec3 brdf = eval_brdf(M, N, V, dir);
        float pdf = cosTheta / 3.14159265f;
        throughput = mulv(throughput, mul(brdf, 1.0f / (pdf + 1e-12f)));
        ray = newRay;
        // Russian roulette
        if(bounce > 3){
            float pcont = std::min(0.95f, std::max({throughput.x, throughput.y, throughput.z}));
            if(rand01f() > pcont) break;
            throughput = mul(throughput, 1.0f / pcont);
        }
    }
    return accum;
}

// simple write ppm / png helper (requires ImageMagick 'magick' present)
static void write_image(const std::string &name, int W, int H, const std::vector<unsigned char>& img){
    std::ofstream f(name + ".ppm", std::ios::binary);
    f<<"P6\n"<<W<<" "<<H<<"\n255\n";
    f.write((char*)img.data(), img.size());
    f.close();
    // png if magick present
    if(system("which magick > /dev/null 2>&1")==0){
        std::string cmd = std::string("magick ") + name + ".ppm " + name + ".png";
        system(cmd.c_str());
    }
}

int main_prod(int W=800,int H=400){
    // build a minimal demonstrative scene:
    g_tris.clear(); g_mats.clear(); g_spheres.clear();

    // ground (two tris)
    Triangle g1; g1.v0=Vec3(-10,-1,-10); g1.v1=Vec3(10,-1,-10); g1.v2=Vec3(10,-1,10); g1.material_id=0; g1.compute_bounds();
    Triangle g2; g2.v0=Vec3(-10,-1,-10); g2.v1=Vec3(10,-1,10); g2.v2=Vec3(-10,-1,10); g2.material_id=0; g2.compute_bounds();
    g_tris.push_back(g1); g_tris.push_back(g2);

    // light (emissive triangle)
    Triangle lt; lt.v0=Vec3(-1.2f,1.8f,0.0f); lt.v1=Vec3(1.2f,1.8f,0.0f); lt.v2=Vec3(0.0f,1.8f,1.8f); lt.material_id=1; lt.compute_bounds();
    g_tris.push_back(lt);

    Triangle t1; t1.v0=Vec3(-0.7f,-1,0.3f); t1.v1=Vec3(-0.2f,0.2f,0.5f); t1.v2=Vec3(-1.2f,0.2f,0.5f); t1.material_id=2; t1.compute_bounds();
    Triangle t2; t2.v0=Vec3(0.7f,-1,0.3f); t2.v1=Vec3(1.2f,0.2f,0.5f); t2.v2=Vec3(0.2f,0.2f,0.5f); t2.material_id=3; t2.compute_bounds();
    g_tris.push_back(t1); g_tris.push_back(t2);

    Sphere sph; sph.center = Vec3(0.0f, -0.3f, 0.8f); sph.radius = 0.35f; sph.material_id = 4;
    g_spheres.push_back(sph);

    // materials
    g_mats.push_back(m0); g_mats.push_back(mlight); g_mats.push_back(mr); g_mats.push_back(mm); g_mats.push_back(mm);

    // build BVH
    g_bvh.build(g_tris);

    // camera
    Camera cam; cam.aspect = float(W)/float(H); cam.pos = Vec3(0,0,-4); cam.aperture = 0.03f; cam.focal = 4.0f;

    // render params
    int spp = 256;
    std::vector<float> accum(W*H*3, 0.0f);

    int tile = 32;
    std::atomic<int> tasks_done{0};
    int total_tiles = ((W+tile-1)/tile)*((H+tile-1)/tile);

    // simple thread pool using std::thread
    int nthreads = std::max(1u, std::thread::hardware_concurrency() ? std::thread::hardware_concurrency() : 4u);
    std::vector<std::thread> workers;
    std::atomic<int> next_tile{0};
    auto worker = [&](void){
        int tid;
        while((tid = next_tile.fetch_add(1)) < total_tiles){
            int tx = tid % ((W+tile-1)/tile);
            int ty = tid / ((W+tile-1)/tile);
            int x0 = tx*tile; int y0 = ty*tile;
            for(int y=y0;y<std::min(H,y0+tile);++y){
                for(int x=x0;x<std::min(W,x0+tile);++x){
                    Vec3 csum(0,0,0);
                    for(int s=0;s<spp;++s){
                        Ray r = cam.generate((float)x, (float)y, W, H);
                        Vec3 col = shade_path(r, 8);
                        csum = add(csum, col);
                    }
                    csum = mul(csum, 1.0f / spp);
                    int idx = (y*W + x)*3;
                    accum[idx+0] += csum.x;
                    accum[idx+1] += csum.y;
                    accum[idx+2] += csum.z;
                }
            }
            tasks_done++;
        }
    };

    for(int i=0;i<nthreads;i++) workers.emplace_back(worker);
    for(auto &t : workers) t.join();

    // convert & tonemap + write
    std::vector<unsigned char> img(W*H*3);
    for(int y=0;y<H;y++){
        for(int x=0;x<W;x++){
            int idx=(y*W+x)*3;
            Vec3 c(accum[idx+0], accum[idx+1], accum[idx+2]);
            // filmic-like tonemap
            auto tm=[&](float v){ return v/(1.0f+v); };
            Vec3 tcol = Vec3(tm(c.x), tm(c.y), tm(c.z));
            img[idx+0] = (unsigned char)(255.0f * std::min(1.0f, tcol.x));
            img[idx+1] = (unsigned char)(255.0f * std::min(1.0f, tcol.y));
            img[idx+2] = (unsigned char)(255.0f * std::min(1.0f, tcol.z));
        }
    }

    write_image("render_prod", W, H, img);

    // denoise
    atrous_denoise(img.data(), W, H);
    write_image("render_prod_denoised", W, H, img);
    std::cout << "[PROD] render_prod.png and render_prod_denoised.png written\n";
    return 0;
}
