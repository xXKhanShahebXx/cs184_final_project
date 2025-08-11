#pragma once
#include <cmath>

struct Vec3 { float x, y, z; Vec3():x(0),y(0),z(0){} Vec3(float x,float y,float z):x(x),y(y),z(z){} Vec3(float v):x(v),y(v),z(v){}
    Vec3 operator+(const Vec3& o) const { return Vec3(x+o.x,y+o.y,z+o.z); }
    Vec3 operator-(const Vec3& o) const { return Vec3(x-o.x,y-o.y,z-o.z); }
    Vec3 operator*(float s) const { return Vec3(x*s,y*s,z*s); }
    Vec3 operator/(float s) const { return Vec3(x/s,y/s,z/s); }
    Vec3& operator+=(const Vec3& o){ x+=o.x; y+=o.y; z+=o.z; return *this; }
    Vec3& operator-=(const Vec3& o){ x-=o.x; y-=o.y; z-=o.z; return *this; }
    Vec3 operator-() const { return Vec3(-x,-y,-z); }
    float length() const { return std::sqrt(x*x+y*y+z*z); }
    Vec3 normalize() const { float l=length(); return l>0?(*this)/l:Vec3(); }
    float dot(const Vec3& o) const { return x*o.x+y*o.y+z*o.z; }
};
inline Vec3 operator*(float s,const Vec3& v){ return v*s; }
inline float length(const Vec3& v){ return v.length(); }
inline Vec3 normalize(const Vec3& v){ return v.normalize(); }
inline float dot(const Vec3& a,const Vec3& b){ return a.dot(b); }