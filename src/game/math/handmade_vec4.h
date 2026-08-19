#ifndef HANDMADE_VEC4_H
#define HANDMADE_VEC4_H

#include "game/handmade.h"

struct Vec4 {
    // Very cumbersome and bug-attracting stuff...

    // TODO: these could be same as Vec3's:
    // Vec2 xy;
    // f32 ignored0;
    // Instead of this madness
    union {
        struct {
            union {
                Vec3 rgb;

                struct {
                    f32 r, g, b;
                };
            };

            f32 a;
        };

        struct {
            union {
                Vec3 xyz;

                struct {
                    f32 x, y, z;
                };
            };

            f32 w;
        };

        struct {
            union {
                Vec2 xy;

                struct {
                    f32 x, y;
                };
            };

            f32 z, w;
        };

        struct {
            f32 x;

            union {
                Vec2 yz;

                struct {
                    f32 y, z;
                };
            };

            f32 w;
        };

        struct {
            f32 x, y;

            union {
                Vec2 zw;

                struct {
                    f32 z, w;
                };
            };
        };

        f32 e[4];
    };

    Vec4() = default;

    // TODO: Better ways?
    template <typename T>
    Vec4(T x_) : x{ static_cast<f32>(x_) }, y{}, z{}, w{} {}

    template <typename T, typename U>
    Vec4(T x_, U y_) : x{ static_cast<f32>(x_) }, y{ static_cast<f32>(y_) }, z{}, w{} {}

    template <typename T, typename U, typename V>
    Vec4(T x_, U y_, V z_)
        : x{ static_cast<f32>(x_) }, y{ static_cast<f32>(y_) }, z{ static_cast<f32>(z_) }, w{} {}

    template <typename T, typename U, typename V, typename W>
    Vec4(T x_, U y_, V z_, W w_)
        : x{ static_cast<f32>(x_) }, y{ static_cast<f32>(y_) }, z{ static_cast<f32>(z_) },
          w{ static_cast<f32>(w_) } {}

    Vec4(Vec3 vec, f32 w_)
        : x{ static_cast<f32>(vec.x) }, y{ static_cast<f32>(vec.y) }, z{ static_cast<f32>(vec.z) },
          w{ static_cast<f32>(w_) } {}

    NODISCARD
    inline f32& operator[](i32 i);

    inline Vec4& operator+=(Vec4 rhs);
    inline Vec4& operator-=(Vec4 rhs);
    inline Vec4& operator*=(f32 scalar);
    inline Vec4& operator/=(f32 scalar);
    // Hadamard
    inline Vec4& operator*=(Vec4 vec);

    NOT_BOUND const Vec4 ZERO;
    NOT_BOUND const Vec4 ONE;
};

inline const Vec4 Vec4::ZERO{};
inline const Vec4 Vec4::ONE{ 1, 1, 1, 1 };

//NODISCARD
//INTERNAL inline Vec4 operator-(Vec4 rhs);

//NODISCARD
//INTERNAL inline Vec4 operator+(Vec4 lhs, Vec4 rhs);

//NODISCARD
//INTERNAL inline Vec4 operator-(Vec4 lhs, Vec4 rhs);

//NODISCARD
//INTERNAL inline Vec4 operator*(Vec4 lhs, f32 scalar);

//NODISCARD
//INTERNAL inline Vec4 operator*(f32 scalar, Vec4 rhs);

//NODISCARD
//INTERNAL inline Vec4 operator/(Vec4 lhs, f32 scalar);

#endif // HANDMADE_VEC4_H
