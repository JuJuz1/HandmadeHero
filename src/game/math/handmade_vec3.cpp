#include "handmade_vec3.h"

NODISCARD
INTERNAL inline Vec3
operator-(Vec3 v) {
    const Vec3 result{ -v.x, -v.y, -v.z };
    return result;
}

NODISCARD
INTERNAL inline Vec3
operator+(Vec3 lhs, Vec3 rhs) {
    const Vec3 result{ lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z };
    return result;
}

NODISCARD
INTERNAL inline Vec3
operator-(Vec3 lhs, Vec3 rhs) {
    const Vec3 result{ lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z };
    return result;
}

NODISCARD
INTERNAL inline Vec3
operator*(Vec3 lhs, f32 scalar) {
    const Vec3 result{ lhs.x * scalar, lhs.y * scalar, lhs.z * scalar };
    return result;
}

NODISCARD
INTERNAL inline Vec3
operator*(f32 scalar, Vec3 rhs) {
    const Vec3 result{ rhs * scalar };
    return result;
}

NODISCARD
INTERNAL inline Vec3
operator/(Vec3 lhs, f32 scalar) {
    const Vec3 result{ lhs.x / scalar, lhs.y / scalar, lhs.z / scalar };
    return result;
}

// Hadamard
NODISCARD
INTERNAL inline Vec3
operator*(Vec3 lhs, Vec3 rhs) {
    const Vec3 result{ lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z };
    return result;
}

/// Member functions ///

NODISCARD
inline f32&
Vec3::operator[](i32 i) {
    ASSERT(0 <= i && i < 3);
    f32& result{ this->e[i] };
    return result;
}

inline Vec3&
Vec3::operator+=(Vec3 rhs) {
    *this = *this + rhs;
    return *this;
}

inline Vec3&
Vec3::operator-=(Vec3 rhs) {
    *this = *this - rhs;
    return *this;
}

inline Vec3&
Vec3::operator*=(f32 scalar) {
    *this = *this * scalar;
    return *this;
}

inline Vec3&
Vec3::operator/=(f32 scalar) {
    *this = *this * (1.0f / scalar);
    return *this;
}

NODISCARD
INTERNAL inline bool32
operator==(Vec3 lhs, Vec3 rhs) {
    const bool32 result{ lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z };
    return result;
}

NODISCARD
INTERNAL inline bool32
operator!=(Vec3 lhs, Vec3 rhs) {
    const bool32 result{ !(lhs == rhs) };
    return result;
}

NODISCARD
INTERNAL inline f32
Dot(Vec3 lhs, Vec3 rhs) {
    const f32 result{ (lhs.x * rhs.x) + (lhs.y * rhs.y) + (lhs.z * rhs.z) };
    return result;
}

// This is cheaper to compute than Length, so use if we really don't need the actual length
NODISCARD
INTERNAL inline f32
LengthSq(Vec3 v) {
    const f32 result{ Dot(v, v) };
    ASSERT(result >= 0.0f);
    return result;
}

NODISCARD
INTERNAL inline f32
Length(Vec3 v) {
    const f32 result{ Sqrt(LengthSq(v)) };
    ASSERT(result >= 0.0f);
    return result;
}
