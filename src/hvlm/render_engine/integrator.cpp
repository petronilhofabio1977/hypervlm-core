#include "hvlm/core/triangle.hpp"
int main_engine(int W, int H);
#include "util.hpp"
#include "geometry.hpp"
#include "sah_bvh.hpp"
#include "../common/material.hpp"
#include "denoise.hpp"
#include "atm_integration.hpp"
#include <vector>
#include <iostream>
#include <fstream>
#include <atomic>
#include <thread>
#include <chrono>

static std::vector<Triangle> g_tris;
static std::vector<Material> g_mats;
static std::vector<Sphere> g_spheres;
static BVH g_bvh;

struct Camera {
    Vec3 pos, look, up; float fov, aspect, aperture, focal;
    Camera(){ pos = Vec3(0,0,-4); look = Vec3(0,0,1); up=Vec3(0,1,0); fov=45.0f; aspect=2.0f; aperture=0.0f; focal=4.0f; }
    Ray generate(float px, float py, int W, int H){
        float u = (2.0f*(px + rand01f())/W - 1.0f) * tanf(fov*0.5f * PI_F/180.0f) * aspect;
        float v = (1.0f - 2.0f*(py + rand01f())/H) * tanf(fov*0.5f * PI_F/180.0f);
        Vec3 dir = normalize(Vec3(u, v, 1.0f));
        Ray r(pos, dir);
        if(aperture > 1e-6f){
            Vec3 focalp = add(pos, mul(dir, focal));
            float rth = sqrtf(rand01f()); float theta = 2.0f*PI_F*rand01f();
            float rx = rth * cosf(theta) * aperture * 0.5f;
            float ry = rth * sinf(theta) * aperture * 0.5f;
            Vec3 offset = add(mul(normalize(crossp(up, dir)), rx), mul(up, ry));
            r.o = add(r.o, offset);
            r.d = normalize(subv(focalp, r.o));
        }
        return r;
    }
};

static Vec3 eval_env(const Ray &r){
    float t = 0.5f * (r.d.y + 1.0f);
    return add(mul(Vec3(0.7f,0.8f,1.0f), t), mul(Vec3(0.2f,0.2f,0.25f), 1.0f - t));
}

// balance heuristic
static inline float balance_weight(float a, float b){ float aa = a*a; float bb = b*b; return aa/(aa+bb+1e-12f); }

// sample point on triangle uniformly (returns position and pdf per area)
static void sample_triangle_uniform(const Triangle &tri, Vec3 &pos, Vec3 &nOut, float &pdf){
    float r1 = rand01f(), r2 = rand01f();
    float su = 1.0f - sqrtf(r1);
    float sv = sqrtf(r1)*(1.0f - r2);
    float sw = sqrtf(r1)*r2;
    pos = add(add(mul(tri.v0, su), mul(tri.v1, sv)), mul(tri.v2, sw));
    nOut = normalize(crossp(subv(tri.v1, tri.v0), subv(tri.v2, tri.v0)));
    float area = len(crossp(subv(tri.v1, tri.v0), subv(tri.v2, tri.v0))) * 0.5f;
    pdf = 1.0f / (area + 1e-12f);
}

static Vec3 trace_path(const Ray &rinit, int max_bounces){
    Vec3 throughput(1,1,1), radiance(0,0,0);
    Ray ray = rinit;
    for(int bounce=0;bounce<max_bounces;++bounce){
        // test spheres
        float tSphere = 1e30f; Vec3 nSphere; int sphMat=-1;
        for(size_t i=0;i<g_spheres.size();++i){
                if(t < tSphere){ tSphere = t; nSphere = n; sphMat = g_spheres[i].material_id; }
            }
        }
        // test triangles BVH
        float tTri; Vec3 nTri; int triMat=-1;
        bool triHit = g_bvh.intersect(ray, tTri, nTri, triMat);
        bool hitSomething = false;
        float tHit = 1e30f; Vec3 N; int matId=-1;
        if(sphMat >=0 && (!triHit || tSphere < tTri)){ hitSomething=true; tHit = tSphere; N = nSphere; matId = sphMat; }
        else if(triHit){ hitSomething=true; tHit = tTri; N = nTri; matId = triMat; }
        if(!hitSomething){
            radiance = add(radiance, mulv(throughput, eval_env(ray)));
            break;
        }
        Vec3 P = add(ray.o, mul(ray.d, tHit));
        Material M = g_mats[matId];
        if(M.emission.x > 0.0f || M.emission.y > 0.0f || M.emission.z > 0.0f){
            radiance = add(radiance, mulv(throughput, M.emission));
            break;
        }
        Vec3 V = normalize(mul(ray.d, -1.0f));
        // Direct light sampling (pick one emissive triangle if exists)
        Vec3 Ld(0,0,0);
        int lightTriIdx=-1;
        for(size_t i=0;i<g_tris.size();++i) if(g_mats[g_tris[i].material_id].emission.x>0.0f) { lightTriIdx = int(i); break; }
        if(lightTriIdx>=0){
            Vec3 lightPos, lightN; float pdfLight;
            sample_triangle_uniform(g_tris[lightTriIdx], lightPos, lightN, pdfLight);
            Vec3 wi = normalize(subv(lightPos, P));
            // if not occluded
// REMOVED_UNUSED:             float tS; Vec3 nS; int mDummy;
            bool occ = false;
            // sphere occlusion
            // triangle occlusion
            float dummyT; Vec3 dummyN; int dummyM;
            if(!occ){
                float dist2 = dot(subv(lightPos,P), subv(lightPos,P));
                float cosL = std::max(0.0f, dot(lightN, mul(wi,-1.0f)));
                float pdf_area = pdfLight;
                Vec3 brdf = eval_brdf(M, N, V, wi);
                Vec3 Le = g_mats[g_tris[lightTriIdx].material_id].emission;
                float pdf_bsdf = std::max(1e-6f, dot(N,wi)/PI_F);
                Ld = mul(Ld, w);
            }
        }
        Vec3 Ls;
        float pdf_bsdf;
        if(rand01f() < 0.5f){
            // cosine hemisphere
            float r1 = rand01f(), r2 = rand01f();
            float phi = 2.0f*PI_F*r1;
            float cosTheta = sqrtf(1.0f - r2);
            float sinTheta = sqrtf(1.0f - cosTheta*cosTheta);
            Vec3 tangent = fabs(N.x) > 0.1f ? Vec3(0,1,0) : Vec3(1,0,0);
            Vec3 bitangent = normalize(crossp(N, tangent));
            Vec3 dir = normalize(add(add(mul(tangent, cosf(phi)*sinTheta), mul(bitangent, sinf(phi)*sinTheta)), mul(N, cosTheta)));
            Ls = dir;
            pdf_bsdf = cosTheta / PI_F * 0.5f;
        } else {
            Vec3 R = subv(mul(N, 2.0f*dot(N,V)), V);
            float u1 = rand01f(), u2 = rand01f();
            float sinTheta = sqrtf(1.0f - cosTheta*cosTheta);
            float phi = 2.0f*PI_F*u2;
            Vec3 T = fabs(R.x) > 0.1f ? Vec3(0,1,0) : Vec3(1,0,0);
            Vec3 B = normalize(crossp(R, T));
            Ls = normalize(add(add(mul(T, cosf(phi)*sinTheta), mul(B, sinf(phi)*sinTheta)), mul(R, cosTheta)));
        }
        Vec3 brdf_val = eval_brdf(M, N, V, Ls);
        float cosNLs = std::max(0.0f, dot(N, Ls));
        // throughput update
        throughput = mulv(throughput, mul(brdf_val, cosNLs / (pdf_bsdf + 1e-12f)));
        // accumulate direct light from light sampling
        radiance = add(radiance, mulv(throughput, Ld));
        ray = Ray(add(P, mul(Ls, 1e-4f)), Ls);
        // Russian roulette
        if(bounce > 3){
            float pcont = std::min(0.95f, std::max({throughput.x, throughput.y, throughput.z}));
            if(rand01f() > pcont) break;
            throughput = mul(throughput, 1.0f / pcont);
        }
    }
    return radiance;
}

// simple write helpers
static void write_ppm(const std::string &name, int W, int H, const std::vector<unsigned char> &img){
    std::ofstream f(name + ".ppm", std::ios::binary);
    f<<"P6\n"<<W<<" "<<H<<"\n255\n";
    f.write((char*)img.data(), img.size());
    f.close();
    if(system("which magick > /dev/null 2>&1")==0){
        std::string cmd = std::string("magick ") + name + ".ppm " + name + ".png";
        system(cmd.c_str());
    }
}

// main entry

// =========================
//  MAIN ENGINE ENTRY POINT
// =========================
int main_engine(int W, int H) {

    // scene: clear
    g_tris.clear();
    g_mats.clear();
    g_spheres.clear();

    // ground
    Triangle g1; g1.v0 = Vec3(-10,-1,-10); g1.v1 = Vec3(10,-1,-10); g1.v2 = Vec3(10,-1,10); g1.material_id = 0; g1.compute_bounds();
    Triangle g2; g2.v0 = Vec3(-10,-1,-10); g2.v1 = Vec3(10,-1,10); g2.v2 = Vec3(-10,-1,10); g2.material_id = 0; g2.compute_bounds();
    g_tris.push_back(g1); g_tris.push_back(g2);

    // emissive light
    Triangle lt; lt.v0 = Vec3(-1.2f,1.8f,0.0f); lt.v1 = Vec3(1.2f,1.8f,0.0f); lt.v2 = Vec3(0.0f,1.8f,1.8f); lt.material_id = 1; lt.compute_bounds();
    g_tris.push_back(lt);

    // sample triangles
    Triangle t1; t1.v0 = Vec3(-0.7f,-1,0.3f); t1.v1 = Vec3(-0.2f,0.2f,0.5f); t1.v2 = Vec3(-1.2f,0.2f,0.5f); t1.material_id = 2; t1.compute_bounds();
    Triangle t2; t2.v0 = Vec3(0.7f,-1,0.3f); t2.v1 = Vec3(1.2f,0.2f,0.5f); t2.v2 = Vec3(0.2f,0.2f,0.5f); t2.material_id = 3; t2.compute_bounds();
    g_tris.push_back(t1); g_tris.push_back(t2);

    Sphere sph; sph.center = Vec3(0.0f, -0.3f, 0.8f); sph.radius = 0.35f; sph.material_id = 4;
    g_spheres.push_back(sph);

    // materials

    g_mats.push_back(m0);      // 0
    g_mats.push_back(mlight);  // 1
    g_mats.push_back(mRed);    // 2
    g_mats.push_back(mMetal);  // 3
    g_mats.push_back(mMetal);  // 4 (sphere)

    // BVH
    g_bvh.build(g_tris);

    // camera
    Camera cam;
    cam.aspect = float(W)/float(H);

    // rendering
    const int spp = 64;
    std::vector<float> accum(W*H*3, 0.0f);

    // render loop (single thread for now)
    for(int y=0;y<H;y++){
        for(int x=0;x<W;x++){
            Vec3 sum(0,0,0);
            for(int s=0;s<spp;s++){
                Ray r = cam.generate(x, y, W, H);
                Vec3 c = trace_path(r, 8);
                sum = add(sum, c);
            }
            sum = mul(sum, 1.0f / float(spp));
            int idx = (y*W+x)*3;
            accum[idx] = sum.x;
            accum[idx+1] = sum.y;
            accum[idx+2] = sum.z;
        }
    }

    // convert to 8-bit
    std::vector<unsigned char> img(W*H*3);
    for(int i=0;i<W*H;i++){
        img[i*3+0] = (unsigned char)(255.0f * accum[i*3+0]/(1.0f + accum[i*3+0]));
        img[i*3+1] = (unsigned char)(255.0f * accum[i*3+1]/(1.0f + accum[i*3+1]));
        img[i*3+2] = (unsigned char)(255.0f * accum[i*3+2]/(1.0f + accum[i*3+2]));
    }

    std::ofstream f("render_engine.ppm", std::ios::binary);
    f << "P6\n" << W << " " << H << "\n255\n";
    f.write((char*)img.data(), img.size());
    f.close();

    std::cout << "[ENGINE] render_engine.ppm written\n";
    return 0;
}

