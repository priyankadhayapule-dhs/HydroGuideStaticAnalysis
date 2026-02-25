//
// Copyright 2019 EchoNous Inc.
//

// Basic geometry structs (vec2-4) and functions
// Could be extended with matrices, or replaced completely with glm

#pragma once

#include <utility>
#include <cmath>
#include <ViewMatrix3x3.h>

// Clamp a value between two extremes
template <typename T>
T clamp(T val, T low, T high);

// Generic vectors of 2-4 elements
template <typename T>
struct tvec2
{
	T x,y;

	tvec2() = default;
	tvec2(T x, T y) : x(x),y(y) {}

	// vector add/subtract
	tvec2<T>& operator+=(const tvec2<T>&);
	tvec2<T>& operator-=(const tvec2<T>&);
	// scalar multiply/divide
	tvec2<T>& operator*=(const T&);
	tvec2<T>& operator/=(const T&);
};

template <typename T>
struct tvec3
{
	T x,y,z;

	tvec3() = default;
	tvec3(T x, T y, T z) : x(x), y(y), z(z) {}

	// vector add/subtract
	tvec3<T>& operator+=(const tvec3<T>&);
	tvec3<T>& operator-=(const tvec3<T>&);
	// scalar multiply/divide
	tvec3<T>& operator*=(const T&);
	tvec3<T>& operator/=(const T&);
};

template <typename T>
struct tvec4
{
	T x,y,z,w;

	tvec4() = default;
	tvec4(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) {}

	// vector add/subtract
	tvec4<T>& operator+=(const tvec4<T>&);
	tvec4<T>& operator-=(const tvec4<T>&);
	// scalar multiply/divide
	tvec4<T>& operator*=(const T&);
	tvec4<T>& operator/=(const T&);
};

// typedefs for common types
typedef tvec2<float> vec2;
typedef tvec3<float> vec3;
typedef tvec4<float> vec4;

typedef tvec2<int> ivec2;
typedef tvec3<int> ivec3;
typedef tvec4<int> ivec4;

// external operations
template <typename T>
tvec2<T> operator-(const tvec2<T>&);
template <typename T>
tvec2<T> operator+(const tvec2<T>&, const tvec2<T>&);
template <typename T>
tvec2<T> operator-(const tvec2<T>&, const tvec2<T>&);
// multiply assumes commutative
template <typename T>
tvec2<T> operator*(const tvec2<T>&, const T&);
template <typename T>
tvec2<T> operator*(const T&, const tvec2<T>&);
template <typename T>
tvec2<T> operator/(const tvec2<T>&, const T&);
// other geometry functions
template <typename T>
T dot(const tvec2<T>&, const tvec2<T>&);
// length and distance always returned as float
template <typename T>
float length(const tvec2<T>&);
template <typename T>
float distance(const tvec2<T>&, const tvec2<T>&);
template <typename T>
tvec2<T> normalize(const tvec2<T>&);

template <typename T>
tvec3<T> operator-(const tvec3<T>&);
template <typename T>
tvec3<T> operator+(const tvec3<T>&, const tvec3<T>&);
template <typename T>
tvec3<T> operator-(const tvec3<T>&, const tvec3<T>&);
// multiply assumes commutative
template <typename T>
tvec3<T> operator*(const tvec3<T>&, const T&);
template <typename T>
tvec3<T> operator*(const T&, const tvec3<T>&);
template <typename T>
tvec3<T> operator/(const tvec3<T>&, const T&);
// other geometry functions
template <typename T>
T dot(const tvec3<T>&, const tvec3<T>&);
// length and distance always returned as float
template <typename T>
float length(const tvec3<T>&);
template <typename T>
float distance(const tvec3<T>&, const tvec3<T>&);
template <typename T>
tvec3<T> normalize(const tvec3<T>&);
// cross product only valid for vec3
template <typename T>
tvec3<T> cross(const tvec3<T>&, const tvec3<T>&);

template <typename T>
tvec4<T> operator-(const tvec4<T>&);
template <typename T>
tvec4<T> operator+(const tvec4<T>&, const tvec4<T>&);
template <typename T>
tvec4<T> operator-(const tvec4<T>&, const tvec4<T>&);
// multiply assumes commutative
template <typename T>
tvec4<T> operator*(const tvec4<T>&, const T&);
template <typename T>
tvec4<T> operator*(const T&, const tvec4<T>&);
template <typename T>
tvec4<T> operator/(const tvec4<T>&, const T&);
// other geometry functions
template <typename T>
T dot(const tvec4<T>&, const tvec4<T>&);
// length and distance always returned as float
template <typename T>
float length(const tvec4<T>&);
template <typename T>
float distance(const tvec4<T>&, const tvec4<T>&);
template <typename T>
tvec4<T> normalize(const tvec4<T>&);

// Column major 3x3 matrix
struct Matrix3 {
    float m[9];

    Matrix3() : m() {
        // set to identity
        m[0] = 1.0f;
        m[1] = 0.0f;
        m[2] = 0.0f;

        m[3] = 0.0f;
        m[4] = 1.0f;
        m[5] = 0.0f;

        m[6] = 0.0f;
        m[7] = 0.0f;
        m[8] = 1.0f;
    }

    static Matrix3 Scale(float scale) { return Scale(scale, scale); }
    static Matrix3 Scale(float xScale, float yScale) {
        Matrix3 mtx;
        mtx.at(0,0) = xScale;
        mtx.at(1,1) = yScale;
        return mtx;
    }
    static Matrix3 Translate(float dx, float dy) {
        Matrix3 mtx;
        mtx.at(0,2) = dx;
        mtx.at(1,2) = dy;
        return mtx;
    }

    float* ptr() { return m; }
    const float *ptr() const { return m; }

    float& at(int row, int col) { return m[3*col + row]; }
    float at(int row, int col) const { return m[3*col + row]; }

    // assumes homogeneous coords for third vector component
    vec2 operator*(const vec2& v) const {
        return vec2{
            at(0,0)*v.x + at(0,1)*v.y + at(0,2),
            at(1,0)*v.x + at(1,1)*v.y + at(1,2),
        };
    }

    vec3 operator*(const vec3& v) const {
        return vec3{
            at(0,0)*v.x + at(0,1)*v.y + at(0,2)*v.z,
            at(1,0)*v.x + at(1,1)*v.y + at(1,2)*v.z,
            at(2,0)*v.x + at(2,1)*v.y + at(2,2)*v.z,
        };
    }

    Matrix3 operator*(const Matrix3& rhs) const {
        Matrix3 result;
        ViewMatrix3x3::multiplyMM(result.m, this->m, rhs.m);
        return result;
    }

    Matrix3 inverse() const {
        Matrix3 result;
        ViewMatrix3x3::inverseM(result.m, m);
        return result;
    }
};

#include "geometry.inl"