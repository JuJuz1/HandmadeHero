#ifndef HANDMADE_INTRINSICS_H
#define HANDMADE_INTRINSICS_H

#include <cmath>

/*
TODO: implement these functions ourself using platform-efficient versions and cmath include
*/

INTERNAL inline i32
TruncateI64toI32(i64 value) {
    ASSERT(value <= 2147483647LL);
    ASSERT(value >= -2147483648LL);
    return static_cast<i32>(value);
}

INTERNAL inline u32
TruncateU64toU32(u64 value) {
    // TODO: U32_MAX and such...
    ASSERT(value <= 0xFFFFFFFFULL);
    return static_cast<u32>(value);
}

INTERNAL inline f32
TruncateF64toF32(f64 value) {
    ASSERT(value <= 3.402823466e+38);
    ASSERT(value >= -3.402823466e+38);
    return static_cast<f32>(value);
}

INTERNAL inline i32
TruncateF32ToI32(f32 value) {
    // NOTE: truncate always truncates towards 0, even when value is negative
    return static_cast<i32>(value);
}

INTERNAL inline u32
TruncateF32ToU32(f32 value) {
    ASSERT(value >= 0);
    return static_cast<u32>(value);
}

INTERNAL inline i32
FloorF32ToI32(f32 value) {
    return static_cast<i32>(floorf(value));
}

INTERNAL inline u32
FloorF32ToU32(f32 value) {
    ASSERT(value >= 0);
    return static_cast<u32>(floorf(value));
}

INTERNAL inline u32
RoundF32ToU32(f32 value) {
    ASSERT(value >= 0);
    return static_cast<u32>(value + 0.5f);
}

INTERNAL inline i32
RoundF32ToI32(f32 value) {
    return static_cast<i32>(value + 0.5f);
}

INTERNAL inline f32
Sin(f32 angle) {
    const f32 result{ sinf(angle) };
    return result;
}

INTERNAL inline f32
Cos(f32 angle) {
    const f32 result{ cosf(angle) };
    return result;
}

INTERNAL inline f32
ATan2(f32 y, f32 x) {
    const f32 result{ atan2f(y, x) };
    return result;
}

#endif
