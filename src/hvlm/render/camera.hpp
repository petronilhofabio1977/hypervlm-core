#pragma once
#include "ray.hpp"
#include <cmath>
struct Camera {
    std::array<float,3> pos{0,0,-3};
    Ray generate(float x,float y,int W,int H){
        float u=(2*(x/W)-1), v=(1-2*(y/H));
        return Ray{{pos[0],pos[1],pos[2]}, {u,v,1}};
    }
};
