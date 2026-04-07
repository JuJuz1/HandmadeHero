#ifndef HANDMADE_VEC3_H
#define HANDMADE_VEC3_H

struct Vec3 {
    union {
        struct {
            f32 x;
            f32 y;
            f32 z;
        };

        f32 e[3];
    };

    inline f32& operator[](i32 i);

    inline Vec3& operator+=(Vec3 a);
    inline Vec3& operator-=(Vec3 a);
    inline Vec3& operator*=(f32 scalar);
    inline Vec3& operator/=(f32 scalar);
};

NODISCARD
INTERNAL inline Vec3
operator-(Vec3 a) {
    Vec3 result{ -a.x, -a.y, -a.z };
    return result;
}

NODISCARD
INTERNAL inline Vec3
operator+(Vec3 a, Vec3 b) {
    Vec3 result{ a.x + b.x, a.y + b.y, a.z + b.z };
    return result;
}

NODISCARD
INTERNAL inline Vec3
operator-(Vec3 a, Vec3 b) {
    Vec3 result{ a.x - b.x, a.y - b.y, a.z - b.z };
    return result;
}

NODISCARD
INTERNAL inline Vec3
operator*(Vec3 a, f32 scalar) {
    Vec3 result{ a.x * scalar, a.y * scalar, a.z * scalar };
    return result;
}

NODISCARD
INTERNAL inline Vec3
operator*(f32 scalar, Vec3 a) {
    Vec3 result{ a * scalar };
    return result;
}

NODISCARD
INTERNAL inline Vec3
operator/(Vec3 a, f32 scalar) {
    Vec3 result{ a.x / scalar, a.y / scalar, a.z / scalar };
    return result;
}

/// Member functions

NODISCARD
inline f32&
Vec3::operator[](i32 i) {
    ASSERT(0 <= i && i < 3);
    f32& result{ this->e[i] };
    return result;
}

inline Vec3&
Vec3::operator+=(Vec3 a) {
    *this = *this + a;
    return *this;
}

inline Vec3&
Vec3::operator-=(Vec3 a) {
    *this = *this - a;
    return *this;
}

inline Vec3&
Vec3::operator*=(f32 scalar) {
    *this = *this * scalar;
    return *this;
}

inline Vec3&
Vec3::operator/=(f32 scalar) {
    *this = *this * (1.0f / scalar);
    return *this;
}

#endif // HANDMADE_VEC3_H
