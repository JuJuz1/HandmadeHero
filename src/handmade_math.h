#ifndef HANDMADE_MATH_H
#define HANDMADE_MATH_H

#include "handmade_vec2.h"
#include "handmade_vec3.h"

GLOBAL constexpr f32 PI32f{ 3.14159265359f };

NODISCARD
INTERNAL inline f32
SquareF32(f32 value) {

    const f32 result{ value * value };
    return result;
}

struct Rect {
    Vec2 min, max;
};

NODISCARD
INTERNAL inline Rect
RectMinMax(Vec2 min, Vec2 max) {
    const Rect result{ min, max };
    return result;
}

NODISCARD
INTERNAL inline Rect
RectMinDim(Vec2 min, Vec2 dim) {
    const Rect result{ min, min + dim };
    return result;
}

NODISCARD
INTERNAL inline Rect
RectCenterHalfDim(Vec2 center, Vec2 halfDim) {
    const Rect result{ center - halfDim, center + halfDim };
    return result;
}

NODISCARD
INTERNAL inline Rect
RectCenterDim(Vec2 center, Vec2 dim) {
    const Rect result{ RectCenterHalfDim(center, dim * 0.5f) };
    return result;
}

NODISCARD
INTERNAL inline bool32
IsInsideRectangle(Rect rect, Vec2 test) {
    // NOTE: non-inclusive at the max corner
    const bool32 result{ test.x >= rect.min.x && test.y >= rect.min.y && test.x < rect.max.x &&
                         test.y < rect.max.y };
    return result;
}

#endif // HANDMADE_MATH_H
