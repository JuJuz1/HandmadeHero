#ifndef HANDMADE_MATH_H
#define HANDMADE_MATH_H

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

inline f32&
Vec2::operator[](i32 i) {
    ASSERT(0 <= i && i < 2);
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

#endif // HANDMADE_MATH_H
