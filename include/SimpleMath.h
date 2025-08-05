#pragma once
#include <cmath>

struct Vec3 {
    float x, y, z;
    
    Vec3() : x(0.0f), y(0.0f), z(0.0f) {}
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}
    Vec3(float value) : x(value), y(value), z(value) {}
    
    Vec3 operator+(const Vec3& other) const {
        return Vec3(x + other.x, y + other.y, z + other.z);
    }
    
    Vec3 operator-(const Vec3& other) const {
        return Vec3(x - other.x, y - other.y, z - other.z);
    }
    
    Vec3 operator*(float scalar) const {
        return Vec3(x * scalar, y * scalar, z * scalar);
    }
    
    Vec3 operator/(float scalar) const {
        return Vec3(x / scalar, y / scalar, z / scalar);
    }
    
    Vec3& operator+=(const Vec3& other) {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }
    
    Vec3& operator-=(const Vec3& other) {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }
    
    Vec3 operator-() const {
        return Vec3(-x, -y, -z);
    }
    
    float length() const {
        return std::sqrt(x * x + y * y + z * z);
    }
    
    Vec3 normalize() const {
        float len = length();
        if (len > 0.0f) {
            return *this / len;
        }
        return Vec3();
    }
    
    float dot(const Vec3& other) const {
        return x * other.x + y * other.y + z * other.z;
    }
};

inline Vec3 operator*(float scalar, const Vec3& vec) {
    return vec * scalar;
}

inline float length(const Vec3& vec) {
    return vec.length();
}

inline Vec3 normalize(const Vec3& vec) {
    return vec.normalize();
}

inline float dot(const Vec3& vec1, const Vec3& vec2) {
    return vec1.dot(vec2);
} 