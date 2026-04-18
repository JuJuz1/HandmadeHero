#include "handmade_vec2.h"

#include "game/handmade_intrinsics.h"

NODISCARD
INTERNAL inline Vec2
operator-(Vec2 rhs) {
    const Vec2 result{ -rhs.x, -rhs.y };
    return result;
}

NODISCARD
INTERNAL inline Vec2
operator+(Vec2 lhs, Vec2 rhs) {
    const Vec2 result{ lhs.x + rhs.x, lhs.y + rhs.y };
    return result;
}

NODISCARD
INTERNAL inline Vec2
operator-(Vec2 lhs, Vec2 rhs) {
    const Vec2 result{ lhs.x - rhs.x, lhs.y - rhs.y };
    return result;
}

NODISCARD
INTERNAL inline Vec2
operator*(Vec2 lhs, f32 scalar) {
    const Vec2 result{ lhs.x * scalar, lhs.y * scalar };
    return result;
}

NODISCARD
INTERNAL inline Vec2
operator*(f32 scalar, Vec2 rhs) {
    const Vec2 result{ rhs * scalar };
    return result;
}

NODISCARD
INTERNAL inline Vec2
operator/(Vec2 lhs, f32 scalar) {
    const Vec2 result{ lhs.x / scalar, lhs.y / scalar };
    return result;
}

// Hadamard product
NODISCARD
INTERNAL inline Vec2
operator*(Vec2 lhs, Vec2 rhs) {
    const Vec2 result{ lhs.x * rhs.x, lhs.y * rhs.y };
    return result;
}

/// Member functions ///

NODISCARD
inline f32&
Vec2::operator[](i32 i) {
    ASSERT(0 <= i && i < 2);
    f32& result{ this->e[i] };
    return result;
}

inline Vec2&
Vec2::operator+=(Vec2 rhs) {
    *this = *this + rhs;
    return *this;
}

inline Vec2&
Vec2::operator-=(Vec2 rhs) {
    *this = *this - rhs;
    return *this;
}

inline Vec2&
Vec2::operator*=(f32 scalar) {
    *this = *this * scalar;
    return *this;
}

inline Vec2&
Vec2::operator/=(f32 scalar) {
    *this = *this * (1.0f / scalar);
    return *this;
}

inline Vec2&
Vec2::operator*=(Vec2 rhs) {
    *this = *this * rhs;
    return *this;
}

NODISCARD
INTERNAL inline f32
Dot(Vec2 lhs, Vec2 rhs) {
    const f32 result{ (lhs.x * rhs.x) + (lhs.y * rhs.y) };
    return result;
}

NODISCARD
INTERNAL inline f32
Length(Vec2 v) {
    const f32 result{ Sqrt(Dot(v, v)) };
    return result;
}

// This is cheaper to compute than Length, so use if we really don't need the actual length
NODISCARD
INTERNAL inline f32
LengthSquared(Vec2 v) {
    const f32 result{ Dot(v, v) };
    return result;
}

NODISCARD
INTERNAL inline bool32
IsNormalized(Vec2 v) {
    constexpr f32 eps{ 0.001f };
    const bool32 result{ AbsF32(LengthSquared(v) - 1.0f) < eps };
    return result;
}

// Doubt this will be hardly ever used
NODISCARD
INTERNAL inline Vec2
Normalize(Vec2 v) {
    if (IsNormalized(v)) {
        return v;
    }

    Vec2 result{ v };
    const f32 lengthSq{ LengthSquared(v) };
    if (lengthSq > 0.0f) {
        result /= Sqrt(lengthSq);
    }

    return result;
}

NODISCARD
INTERNAL inline Vec2
Reflect(Vec2 v, Vec2 n) {
    // NOTE: Assert n is normalized? otherwise the formula breaks
    ASSERT(IsNormalized(n));

    const Vec2 result{ v - (2.0f * Dot(v, n) * n) };
    return result;
}
