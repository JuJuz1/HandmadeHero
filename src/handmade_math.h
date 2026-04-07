#ifndef HANDMADE_MATH_H
#define HANDMADE_MATH_H

#include "handmade_vec2.h"
#include "handmade_vec3.h"

NODISCARD
INTERNAL inline f32
SquareF32(f32 value) {
    const f32 result{ value * value };
    return result;
}

#endif // HANDMADE_MATH_H
