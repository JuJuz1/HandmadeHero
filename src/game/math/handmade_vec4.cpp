#include "handmade_vec4.h"

NODISCARD
INTERNAL inline Vec4
operator-(Vec4 rhs) {
    const Vec4 result{ -rhs.x, -rhs.y, -rhs.z, -rhs.w };
    return result;
}

NODISCARD
INTERNAL inline Vec4
operator+(Vec4 lhs, Vec4 rhs) {
    const Vec4 result{ lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w };
    return result;
}

NODISCARD
INTERNAL inline Vec4
operator-(Vec4 lhs, Vec4 rhs) {
    const Vec4 result{ lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w };
    return result;
}

NODISCARD
INTERNAL inline Vec4
operator*(Vec4 lhs, f32 scalar) {
    const Vec4 result{ lhs.x * scalar, lhs.y * scalar, lhs.z * scalar, lhs.w * scalar };
    return result;
}

NODISCARD
INTERNAL inline Vec4
operator*(f32 scalar, Vec4 rhs) {
    const Vec4 result{ rhs * scalar };
    return result;
}

NODISCARD
INTERNAL inline Vec4
operator/(Vec4 rhs, f32 scalar) {
    const Vec4 result{ rhs.x / scalar, rhs.y / scalar, rhs.z / scalar, rhs.w / scalar };
    return result;
}

/// Member functions ///

NODISCARD
inline f32&
Vec4::operator[](i32 i) {
    ASSERT(0 <= i && i < 4);
    f32& result{ this->e[i] };
    return result;
}

inline Vec4&
Vec4::operator+=(Vec4 rhs) {
    *this = *this + rhs;
    return *this;
}

inline Vec4&
Vec4::operator-=(Vec4 rhs) {
    *this = *this - rhs;
    return *this;
}

inline Vec4&
Vec4::operator*=(f32 scalar) {
    *this = *this * scalar;
    return *this;
}

inline Vec4&
Vec4::operator/=(f32 scalar) {
    *this = *this * (1.0f / scalar);
    return *this;
}
