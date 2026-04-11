#include "handmade_math.h"

#include "handmade_rect.cpp"
#include "handmade_vec2.cpp"
#include "handmade_vec3.cpp"

/*
   IMPORTANT: Only include this! not the above .cpp files
*/

NODISCARD
INTERNAL inline f32
SquareF32(f32 value) {
    const f32 result{ value * value };
    return result;
}
