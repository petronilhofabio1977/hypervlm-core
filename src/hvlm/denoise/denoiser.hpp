#pragma once
#include <vector>
#include <cmath>
#include "../render_pt_full/scene_geo.hpp"

// simple A-Trous with color buffer (no normal buffer for brevity) - 3 iterations
static void atrous_denoise(unsigned char* img, int W, int H){
    std::vector<float> buf(W*H*3);
    for(int i=0;i<W*H*3;i++) buf[i] = img[i] / 255.0f;
    std::vector<float> tmp(buf.size());
    const int iterations = 3;
    const int kernel[5] = {1,4,6,4,1};
    for(int iter=0; iter<iterations; ++iter){
        int step = 1<<iter;
        for(int y=0;y<H;y++){
            for(int x=0;x<W;x++){
                float sum[3]={0,0,0}; float wsum=0;
                for(int ky=-2; ky<=2; ++ky){
                    for(int kx=-2; kx<=2; ++kx){
                        int sx = x + kx*step; int sy = y + ky*step;
                        if(sx<0||sy<0||sx>=W||sy>=H) continue;
                        int k = (abs(kx)+abs(ky));
                        int kval = kernel[ std::min(4, k) ];
                        int idx = (sy*W + sx)*3;
                        sum[0] += buf[idx+0]*kval;
                        sum[1] += buf[idx+1]*kval;
                        sum[2] += buf[idx+2]*kval;
                        wsum += kval;
                    }
                }
                size_t oidx = (y*W + x)*3;
                tmp[oidx+0] = sum[0]/wsum;
                tmp[oidx+1] = sum[1]/wsum;
                tmp[oidx+2] = sum[2]/wsum;
            }
        }
        buf.swap(tmp);
    }
    for(int i=0;i<W*H*3;i++) img[i] = (unsigned char)(std::min(1.0f, buf[i]) * 255.0f);
}
