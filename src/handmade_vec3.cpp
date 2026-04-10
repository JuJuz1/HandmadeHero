#include "handmade_vec3.h"

NODISCARD
INTERNAL inline Vec3
operator-(Vec3 a) {
    const Vec3 result{ -a.x, -a.y, -a.z };
    return result;
}

NODISCARD
INTERNAL inline Vec3
operator+(Vec3 a, Vec3 b) {
    const Vec3 result{ a.x + b.x, a.y + b.y, a.z + b.z };
    return result;
}

NODISCARD
INTERNAL inline Vec3
operator-(Vec3 a, Vec3 b) {
    const Vec3 result{ a.x - b.x, a.y - b.y, a.z - b.z };
    return result;
}

NODISCARD
INTERNAL inline Vec3
operator*(Vec3 a, f32 scalar) {
    const Vec3 result{ a.x * scalar, a.y * scalar, a.z * scalar };
    return result;
}

NODISCARD
INTERNAL inline Vec3
operator*(f32 scalar, Vec3 a) {
    const Vec3 result{ a * scalar };
    return result;
}

NODISCARD
INTERNAL inline Vec3
operator/(Vec3 a, f32 scalar) {
    const Vec3 result{ a.x / scalar, a.y / scalar, a.z / scalar };
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
Vec3::operator+=(Vec3 a) {
    *this = *this + a;
    return *this;
}

inline Vec3&
Vec3::operator-=(Vec3 a) {
    *this = *this - a;
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
