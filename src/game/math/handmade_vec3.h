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

        struct {
            f32 r;
            f32 g;
            f32 b;
        };

        struct {
            Vec2 xy;
            f32 ignored0;
        };

        struct {
            Vec2 yz;
            f32 ignored1;
        };

        f32 e[3];
    };

    NODISCARD
    inline f32& operator[](i32 i);

    inline Vec3& operator+=(Vec3 rhs);
    inline Vec3& operator-=(Vec3 rhs);
    inline Vec3& operator*=(f32 scalar);
    inline Vec3& operator/=(f32 scalar);

    NOT_BOUND const Vec3 ZERO;
    NOT_BOUND const Vec3 ONE;
};

inline constexpr Vec3 Vec3::ZERO{};
inline constexpr Vec3 Vec3::ONE{ 1, 1, 1 };

//NODISCARD
//INTERNAL inline Vec3 operator-(Vec3 rhs);

//NODISCARD
//INTERNAL inline Vec3 operator+(Vec3 lhs, Vec3 rhs);

//NODISCARD
//INTERNAL inline Vec3 operator-(Vec3 lhs, Vec3 rhs);

//NODISCARD
//INTERNAL inline Vec3 operator*(Vec3 lhs, f32 scalar);

//NODISCARD
//INTERNAL inline Vec3 operator*(f32 scalar, Vec3 rhs);

//NODISCARD
//INTERNAL inline Vec3 operator/(Vec3 lhs, f32 scalar);

//NODISCARD
//INTERNAL inline bool32 operator==(Vec3 lhs, Vec3 rhs);

//NODISCARD
//INTERNAL inline bool32 operator!=(Vec3 lhs, Vec3 rhs);

//// Hadamard product
//NODISCARD
//INTERNAL inline Vec3 operator*(Vec3 lhs, Vec3 rhs);

#endif // HANDMADE_VEC3_H
