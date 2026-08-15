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

    Vec3() = default;

    template <typename T, typename U, typename V>
    Vec3(T x_, U y_, V z_)
        : x{ static_cast<f32>(x_) }, y{ static_cast<f32>(y_) }, z{ static_cast<f32>(z_) } {}

    NODISCARD
    inline f32& operator[](i32 i);

    inline Vec3& operator+=(Vec3 rhs);
    inline Vec3& operator-=(Vec3 rhs);
    inline Vec3& operator*=(f32 scalar);
    inline Vec3& operator/=(f32 scalar);
    // Hadamard
    inline Vec3& operator*=(Vec3 scalar);

    NOT_BOUND const Vec3 ZERO;
    NOT_BOUND const Vec3 ONE;
};

inline const Vec3 Vec3::ZERO{};
inline const Vec3 Vec3::ONE{ 1, 1, 1 };

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
