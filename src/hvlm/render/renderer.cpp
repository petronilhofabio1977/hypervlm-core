#include "camera.hpp"
#include "integrator_path.hpp"
#include "../common/material.hpp"
#include "../../../atm_bridge/include/ATMBridge.hpp"
#include <vector>
#include <fstream>
#include <iostream>
#include <atomic>

int main(){
    int W=512,H=256;
    Camera cam; Material mat;
    std::vector<unsigned char> img(W*H*3);
    ATMBridge atm(8);
    std::atomic<int> done{0};

    int tile=32;
    for(int ty=0;ty<H;ty+=tile){
        for(int tx=0;tx<W;tx+=tile){
            atm.submit([&,tx,ty](){
                for(int y=ty;y<std::min(H,ty+tile);y++){
                    for(int x=tx;x<std::min(W,tx+tile);x++){
                        Ray r=cam.generate(x,y,W,H);
                        auto px=shade(r,mat);
                        size_t idx=(y*W+x)*3;
                        img[idx]=px[0]; img[idx+1]=px[1]; img[idx+2]=px[2];
                    }
                }
                done++;
            });
        }
    }

    int total=((H+tile-1)/tile)*((W+tile-1)/tile);
    while(done<total) std::this_thread::sleep_for(std::chrono::milliseconds(10));
    atm.shutdown();

    std::ofstream f("render.ppm",std::ios::binary);
    f<<"P6\n"<<W<<" "<<H<<"\n255\n";
    f.write((char*)img.data(),img.size());
    f.close();
    std::cout<<"Imagem gerada: render.ppm\n";
    return 0;
}
