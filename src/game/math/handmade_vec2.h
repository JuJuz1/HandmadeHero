#ifndef HANDMADE_VEC2_H
#define HANDMADE_VEC2_H

#include "game/handmade.h"

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

    NODISCARD
    inline f32& operator[](i32 i);

    inline Vec2& operator+=(Vec2 a);
    inline Vec2& operator-=(Vec2 a);
    inline Vec2& operator*=(f32 scalar);
    inline Vec2& operator/=(f32 scalar);

    // Hadamard product
    inline Vec2& operator*=(Vec2 a);

    NOT_BOUND const Vec2 LEFT;
    NOT_BOUND const Vec2 RIGHT;
    NOT_BOUND const Vec2 UP;
    NOT_BOUND const Vec2 DOWN;
    NOT_BOUND const Vec2 ZERO;
};

inline constexpr Vec2 Vec2::LEFT{ -1.0f, 0.0f };
inline constexpr Vec2 Vec2::RIGHT{ 1.0f, 0.0f };
inline constexpr Vec2 Vec2::UP{ 0.0f, 1.0f };
inline constexpr Vec2 Vec2::DOWN{ 0.0f, -1.0f };
inline constexpr Vec2 Vec2::ZERO{};

NODISCARD
INTERNAL inline Vec2 operator-(Vec2 rhs);

NODISCARD
INTERNAL inline Vec2 operator+(Vec2 lhs, Vec2 rhs);

NODISCARD
INTERNAL inline Vec2 operator-(Vec2 lhs, Vec2 rhs);

NODISCARD
INTERNAL inline Vec2 operator*(Vec2 lhs, f32 scalar);

NODISCARD
INTERNAL inline Vec2 operator*(f32 scalar, Vec2 rhs);

NODISCARD
INTERNAL inline Vec2 operator/(Vec2 lhs, f32 scalar);

NODISCARD
INTERNAL inline bool32 operator==(Vec2 lhs, Vec2 rhs);

NODISCARD
INTERNAL inline bool32 operator!=(Vec2 lhs, Vec2 rhs);

// Hadamard product
NODISCARD
INTERNAL inline Vec2 operator*(Vec2 lhs, Vec2 rhs);

NODISCARD
INTERNAL inline f32 Dot(Vec2 lhs, Vec2 rhs);

NODISCARD
INTERNAL inline f32 Length(Vec2 v);

NODISCARD
INTERNAL inline f32 LengthSquared(Vec2 v);

NODISCARD
INTERNAL inline bool32 IsNormalized(Vec2 v);

NODISCARD
INTERNAL inline Vec2 Normalize(Vec2 v);

NODISCARD
INTERNAL inline Vec2 Reflect(Vec2 v, Vec2 n);

#endif // HANDMADE_VEC2_H
