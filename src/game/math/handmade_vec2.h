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
};

NODISCARD
INTERNAL inline Vec2 operator-(Vec2 a);

NODISCARD
INTERNAL inline Vec2 operator+(Vec2 a, Vec2 b);

NODISCARD
INTERNAL inline Vec2 operator-(Vec2 a, Vec2 b);

NODISCARD
INTERNAL inline Vec2 operator*(Vec2 a, f32 scalar);

NODISCARD
INTERNAL inline Vec2 operator*(f32 scalar, Vec2 a);

NODISCARD
INTERNAL inline Vec2 operator/(Vec2 a, f32 scalar);

// Hadamard product
NODISCARD
INTERNAL inline Vec2 operator*(Vec2 a, Vec2 b);

NODISCARD
INTERNAL inline f32 Dot(Vec2 a, Vec2 b);

NODISCARD
INTERNAL inline f32 Length(Vec2 a);

NODISCARD
INTERNAL inline f32 LengthSquared(Vec2 a);

NODISCARD
INTERNAL inline bool32 IsNormalized(Vec2 a);

NODISCARD
INTERNAL inline Vec2 Normalize(Vec2 a);

NODISCARD
INTERNAL inline Vec2 Reflect(Vec2 v, Vec2 n);

#endif // HANDMADE_VEC2_H
