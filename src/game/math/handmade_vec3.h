#ifndef HANDMADE_VEC3_H
#define HANDMADE_VEC3_H

#include "game/handmade.h"

struct Vec3 {
    union {
        struct {
            f32 x;
            f32 y;
            f32 z;
        };

        f32 e[3];
    };

    NODISCARD
    inline f32& operator[](i32 i);

    inline Vec3& operator+=(Vec3 a);
    inline Vec3& operator-=(Vec3 a);
    inline Vec3& operator*=(f32 scalar);
    inline Vec3& operator/=(f32 scalar);
};

NODISCARD
INTERNAL inline Vec3 operator-(Vec3 a);

NODISCARD
INTERNAL inline Vec3 operator+(Vec3 a, Vec3 b);

NODISCARD
INTERNAL inline Vec3 operator-(Vec3 a, Vec3 b);

NODISCARD
INTERNAL inline Vec3 operator*(Vec3 a, f32 scalar);

NODISCARD
INTERNAL inline Vec3 operator*(f32 scalar, Vec3 a);

NODISCARD
INTERNAL inline Vec3 operator/(Vec3 a, f32 scalar);

#endif // HANDMADE_VEC3_H
