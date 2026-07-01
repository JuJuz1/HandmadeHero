#include "handmade_math.h"

#include "game/math/handmade_vec2.cpp"
#include "game/math/handmade_vec3.cpp"
#include "game/math/handmade_vec4.cpp"

#include "game/math/handmade_rect.cpp"

/*
   IMPORTANT: Only include this! not the above .cpp files
*/

// TODO: move these to somewhere better place

NODISCARD
INTERNAL f32
SafeRatioN(f32 numerator, f32 divisor, f32 N) {
    f32 result{ N };

    if (divisor != 0.0f) {
        result = numerator / divisor;
    }

    return result;
}

NODISCARD
INTERNAL f32
SafeRatio0(f32 numerator, f32 divisor) {
    const f32 result{ SafeRatioN(numerator, divisor, 0.0f) };

    return result;
}

NODISCARD
INTERNAL f32
SafeRatio1(f32 numerator, f32 divisor) {
    const f32 result{ SafeRatioN(numerator, divisor, 1.0f) };

    return result;
}

NODISCARD
INTERNAL Vec3
GetBarycentric(Rect3 rect, Vec3 p) {
    Vec3 result;

    result.x = SafeRatio0(p.x - rect.min.x, rect.max.x - rect.min.x);
    result.y = SafeRatio0(p.y - rect.min.y, rect.max.y - rect.min.y);
    result.z = SafeRatio0(p.z - rect.min.z, rect.max.z - rect.min.z);

    return result;
}

NODISCARD
INTERNAL f32
Lerp(f32 a, f32 b, f32 t) {
    const f32 result{ ((1 - t) * a) + (t * b) };
    return result;
}

NODISCARD
INTERNAL f32
Clamp(f32 min, f32 max, f32 value) {
    f32 result{ value };

    if (value < min) {
        result = min;
    } else if (value > max) {
        result = max;
    }

    return result;
}

NODISCARD
INTERNAL f32
Clamp01(f32 value) {
    const f32 result{ Clamp(0.0f, value, 1.0f) };
    return result;
}

NODISCARD
INTERNAL inline Vec3
Clamp01(Vec3 value) {
    Vec3 result;

    result.x = Clamp01(value.x);
    result.y = Clamp01(value.y);
    result.z = Clamp01(value.z);

    return result;
}
