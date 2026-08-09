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

    Vec2() = default;

    // Yucky :(
    // TODO: it would be cumbersome to make overloads for all different combinations
    // i32, f32, f32
    // f32, f32, i32
    // ...
    template <typename T, typename U>
    Vec2(T x_, U y_) : x{ static_cast<f32>(x_) }, y{ static_cast<f32>(y_) } {}

    // NOTE: These could also be outside the struct by taking a reference as the first parameter
    //inline Vec2& operator+=(Vec2& a, Vec2 b);

    NODISCARD inline f32& operator[](i32 i);

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

    NOT_BOUND const Vec2 ONE;
};

inline const Vec2 Vec2::LEFT{ -1.0f, 0.0f };
inline const Vec2 Vec2::RIGHT{ 1.0f, 0.0f };
inline const Vec2 Vec2::UP{ 0.0f, 1.0f };
inline const Vec2 Vec2::DOWN{ 0.0f, -1.0f };
inline const Vec2 Vec2::ZERO{};

inline const Vec2 Vec2::ONE{ 1.0f, 1.0f };

//NODISCARD
//INTERNAL inline Vec2 operator-(Vec2 rhs);

//NODISCARD
//INTERNAL inline Vec2 operator+(Vec2 lhs, Vec2 rhs);

//NODISCARD
//INTERNAL inline Vec2 operator-(Vec2 lhs, Vec2 rhs);

//NODISCARD
//INTERNAL inline Vec2 operator*(Vec2 lhs, f32 scalar);

//NODISCARD
//INTERNAL inline Vec2 operator*(f32 scalar, Vec2 rhs);

//NODISCARD
//INTERNAL inline Vec2 operator/(Vec2 lhs, f32 scalar);

//NODISCARD
//INTERNAL inline bool32 operator==(Vec2 lhs, Vec2 rhs);

//NODISCARD
//INTERNAL inline bool32 operator!=(Vec2 lhs, Vec2 rhs);

//// Hadamard product
//NODISCARD
//INTERNAL inline Vec2 operator*(Vec2 lhs, Vec2 rhs);

//NODISCARD
//INTERNAL inline f32 Dot(Vec2 lhs, Vec2 rhs);

//NODISCARD
//INTERNAL inline f32 LengthSq(Vec2 v);

//NODISCARD
//INTERNAL inline f32 Length(Vec2 v);

//NODISCARD
//INTERNAL inline bool32 IsNormalized(Vec2 v);

//NODISCARD
//INTERNAL inline Vec2 Normalize(Vec2 v);

//NODISCARD
//INTERNAL inline Vec2 Reflect(Vec2 v, Vec2 n);

#endif // HANDMADE_VEC2_H
