#ifndef HANDMADE_MATH_H
#define HANDMADE_MATH_H

#include "handmade.h"

#include "handmade_vec2.h"

NODISCARD
INTERNAL inline f32 SquareF32(f32 value);

struct Rect {
    Vec2 min, max;
};

NODISCARD
INTERNAL inline Rect RectMinMax(Vec2 min, Vec2 max);

NODISCARD
INTERNAL inline Rect RectMinDim(Vec2 min, Vec2 dim);

NODISCARD
INTERNAL inline Rect RectCenterHalfDim(Vec2 center, Vec2 halfDim);

NODISCARD
INTERNAL inline Rect RectCenterDim(Vec2 center, Vec2 dim);

NODISCARD
INTERNAL inline bool32 IsInsideRectangle(Rect rect, Vec2 test);

#endif // HANDMADE_MATH_H
