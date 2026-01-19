#pragma once
#include <cmath>

struct Vec3 {
    float x, y, z;

    Vec3() : x(0), y(0), z(0) {}
    Vec3(float v) : x(v), y(v), z(v) {}
    Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
};

/* operadores básicos */
inline Vec3 operator+(const Vec3& a, const Vec3& b) {
    return Vec3(a.x + b.x, a.y + b.y, a.z + b.z);
}
inline Vec3 operator-(const Vec3& a, const Vec3& b) {
    return Vec3(a.x - b.x, a.y - b.y, a.z - b.z);
}
inline Vec3 operator*(const Vec3& a, float s) {
    return Vec3(a.x * s, a.y * s, a.z * s);
}
inline Vec3 operator/(const Vec3& a, float s) {
    return Vec3(a.x / s, a.y / s, a.z / s);
}

/* utilitários */
inline float dot(const Vec3& a, const Vec3& b) {
    return a.x*b.x + a.y*b.y + a.z*b.z;
}
inline Vec3 crossp(const Vec3& a, const Vec3& b) {
    return Vec3(
        a.y*b.z - a.z*b.y,
        a.z*b.x - a.x*b.z,
        a.x*b.y - a.y*b.x
    );
}
inline float len(const Vec3& v) {
    return std::sqrt(dot(v,v));
}
inline Vec3 normalize(const Vec3& v) {
    float l = len(v);
    return (l > 0.0f) ? v / l : Vec3(0);
}
