#include "handmade_rect.h"

NODISCARD
INTERNAL inline Rect2
RectMinMax(Vec2 min, Vec2 max) {
    const Rect2 result{ min, max };
    return result;
}

NODISCARD
INTERNAL inline Rect2
RectMinDim(Vec2 min, Vec2 dim) {
    const Rect2 result{ min, min + dim };
    return result;
}

NODISCARD
INTERNAL inline Rect2
RectCenterHalfDim(Vec2 center, Vec2 halfDim) {
    const Rect2 result{ center - halfDim, center + halfDim };
    return result;
}

NODISCARD
INTERNAL inline Rect2
RectCenterDim(Vec2 center, Vec2 dim) {
    const Rect2 result{ RectCenterHalfDim(center, dim * 0.5f) };
    return result;
}

// Non-inclusive at the max corner
NODISCARD
INTERNAL inline bool32
IsInsideRectangle(Rect2 rect, Vec2 test) {
    const bool32 result{ test.x >= rect.min.x && test.y >= rect.min.y && test.x < rect.max.x &&
                         test.y < rect.max.y };
    return result;
}

INTERNAL inline Rect2
AddRadiusTo(Rect2 rect, Vec2 radius) {
    Rect2 result;

    result.min = rect.min - radius;
    result.max = rect.max + radius;

    return result;
}

///
/// Rect3
///

NODISCARD
INTERNAL inline Rect3
RectMinMax(Vec3 min, Vec3 max) {
    const Rect3 result{ min, max };
    return result;
}

NODISCARD
INTERNAL inline Rect3
RectMinDim(Vec3 min, Vec3 dim) {
    const Rect3 result{ min, min + dim };
    return result;
}

NODISCARD
INTERNAL inline Rect3
RectCenterHalfDim(Vec3 center, Vec3 halfDim) {
    const Rect3 result{ center - halfDim, center + halfDim };
    return result;
}

NODISCARD
INTERNAL inline Rect3
RectCenterDim(Vec3 center, Vec3 dim) {
    const Rect3 result{ RectCenterHalfDim(center, dim * 0.5f) };
    return result;
}

// Non-inclusive at the max corner
NODISCARD
INTERNAL inline bool32
IsInsideRectangle(Rect3 rect, Vec3 test) {
    const bool32 result{ test.x >= rect.min.x && test.y >= rect.min.y && test.z >= rect.min.z &&
                         test.x < rect.max.x && test.y < rect.max.y && test.z < rect.max.z };
    return result;
}

INTERNAL inline Rect3
AddRadiusTo(Rect3 rect, Vec3 radius) {
    Rect3 result;

    result.min = rect.min - radius;
    result.max = rect.max + radius;

    return result;
}

// Doesn't work for rotated rects!
NODISCARD
INTERNAL inline bool32
RectsIntersect(Rect3 a, Rect3 b) {
    const bool32 result{ !((b.max.x <= a.min.x || b.min.x >= a.max.x) ||
                           (b.max.y <= a.min.y || b.min.y >= a.max.y) ||
                           (b.max.z <= a.min.z || b.min.z >= a.max.z)) };
    return result;
}
