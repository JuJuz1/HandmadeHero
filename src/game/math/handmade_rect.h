#ifndef HANDMADE_RECT_H
#define HANDMADE_RECT_H

#include "game/handmade.h"

#include "game/math/handmade_vec2.h"

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

#endif // HANDMADE_RECT_H
