#ifndef HANDMADE_VEC4_H
#define HANDMADE_VEC4_H

#include "game/handmade.h"

struct Vec4 {
    union {
        struct {
            f32 x, y, z, w;
        };

        // As color
        struct {
            f32 r, g, b, a;
        };

        f32 e[4];
    };

    NODISCARD
    inline f32& operator[](i32 i);

    inline Vec4& operator+=(Vec4 rhs);
    inline Vec4& operator-=(Vec4 rhs);
    inline Vec4& operator*=(f32 scalar);
    inline Vec4& operator/=(f32 scalar);
};

NODISCARD
INTERNAL inline Vec4 operator-(Vec4 rhs);

NODISCARD
INTERNAL inline Vec4 operator+(Vec4 lhs, Vec4 b);

NODISCARD
INTERNAL inline Vec4 operator-(Vec4 lhs, Vec4 b);

NODISCARD
INTERNAL inline Vec4 operator*(Vec4 lhs, f32 scalar);

NODISCARD
INTERNAL inline Vec4 operator*(f32 scalar, Vec4 rhs);

NODISCARD
INTERNAL inline Vec4 operator/(Vec4 lhs, f32 scalar);

#endif // HANDMADE_VEC4_H
