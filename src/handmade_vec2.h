#ifndef HANDMADE_VEC2_H
#define HANDMADE_VEC2_H

struct Vec2 {
    union {
        struct {
            f32 x;
            f32 y;
        };

        f32 e[2];
    };

    // NOTE: These could also be outside the struct by taking a reference as the first parameter
    //inline Vec2& operator+=(Vec2& a, Vec2 b);

    inline f32& operator[](i32 i);

    inline Vec2& operator+=(Vec2 a);
    inline Vec2& operator-=(Vec2 a);
    inline Vec2& operator*=(f32 scalar);
    inline Vec2& operator/=(f32 scalar);

    // Hadamard product
    inline Vec2& operator*=(Vec2 a);
};

NODISCARD
INTERNAL inline Vec2
operator-(Vec2 a) {
    Vec2 result{ -a.x, -a.y };
    return result;
}

NODISCARD
INTERNAL inline Vec2
operator+(Vec2 a, Vec2 b) {
    Vec2 result{ a.x + b.x, a.y + b.y };
    return result;
}

NODISCARD
INTERNAL inline Vec2
operator-(Vec2 a, Vec2 b) {
    Vec2 result{ a.x - b.x, a.y - b.y };
    return result;
}

NODISCARD
INTERNAL inline Vec2
operator*(Vec2 a, f32 scalar) {
    Vec2 result{ a.x * scalar, a.y * scalar };
    return result;
}

NODISCARD
INTERNAL inline Vec2
operator*(f32 scalar, Vec2 a) {
    Vec2 result{ a * scalar };
    return result;
}

NODISCARD
INTERNAL inline Vec2
operator/(Vec2 a, f32 scalar) {
    Vec2 result{ a.x / scalar, a.y / scalar };
    return result;
}

// Hadamard product
NODISCARD
INTERNAL inline Vec2
operator*(Vec2 a, Vec2 b) {
    Vec2 result{ a.x * b.x, a.y * b.y };
    return result;
}

/// Member functions

NODISCARD
inline f32&
Vec2::operator[](i32 i) {
    ASSERT(0 <= i && i < *this->e);
    f32& result{ this->e[i] };
    return result;
}

inline Vec2&
Vec2::operator+=(Vec2 a) {
    *this = *this + a;
    return *this;
}

inline Vec2&
Vec2::operator-=(Vec2 a) {
    *this = *this - a;
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
Vec2::operator*=(Vec2 a) {
    *this = *this * a;
    return *this;
}

/// Utility functions

NODISCARD
INTERNAL inline f32
Dot(Vec2 a, Vec2 b) {
    const f32 result{ a.x * b.x + a.y * b.y };
    return result;
}

NODISCARD
INTERNAL inline f32
Length(Vec2 a) {
    const f32 result{ Sqrt(Dot(a, a)) };
    return result;
}

// This is cheaper to compute than Length, so use if we really don't need the actual length
NODISCARD
INTERNAL inline f32
LengthSquared(Vec2 a) {
    const f32 result{ Dot(a, a) };
    return result;
}

NODISCARD
INTERNAL inline bool32
IsNormalized(Vec2 a) {
    constexpr f32 eps{ 1e-3f };
    const bool32 result{ AbsF32(LengthSquared(a) - 1.0f) < eps };
    return result;
}

// Doubt this will be hardly ever used
NODISCARD
INTERNAL inline Vec2
Normalize(Vec2 a) {
    if (IsNormalized(a)) {
        return a;
    }

    Vec2 result{ a };
    const f32 lengthSq{ LengthSquared(a) };
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

#endif // HANDMADE_VEC2_H
