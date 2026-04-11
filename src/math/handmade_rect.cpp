#include "handmade_rect.h"

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

// Non-inclusive at the max corner
NODISCARD
INTERNAL inline bool32
IsInsideRectangle(Rect rect, Vec2 test) {
    const bool32 result{ test.x >= rect.min.x && test.y >= rect.min.y && test.x < rect.max.x &&
                         test.y < rect.max.y };
    return result;
}
