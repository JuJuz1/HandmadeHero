#ifndef HANDMADE_INTRINSICS_H
#define HANDMADE_INTRINSICS_H

#include <cmath>

/*
    TODO: implement these functions ourself using platform-efficient versions and cmath include
*/

NODISCARD
INTERNAL inline i32
TruncateI64toI32(i64 value) {
    ASSERT(value <= 2147483647LL);
    ASSERT(value >= -2147483648LL);
    const i32 result{ static_cast<i32>(value) };
    return result;
}

NODISCARD
INTERNAL inline u32
TruncateU64toU32(u64 value) {
    // TODO: U32_MAX and such...
    ASSERT(value <= 0xFFFFFFFFULL);
    const u32 result{ static_cast<u32>(value) };
    return result;
}

NODISCARD
INTERNAL inline f32
TruncateF64toF32(f64 value) {
    ASSERT(value <= 3.402823466e+38);
    ASSERT(value >= -3.402823466e+38);
    const f32 result{ static_cast<f32>(value) };
    return result;
}

NODISCARD
INTERNAL inline i32
TruncateF32ToI32(f32 value) {
    // NOTE: truncate always truncates towards 0, even when value is negative
    const i32 result{ static_cast<i32>(value) };
    return result;
}

NODISCARD
INTERNAL inline u32
TruncateF32ToU32(f32 value) {
    ASSERT(value >= 0);
    const u32 result{ static_cast<u32>(value) };
    return result;
}

NODISCARD
INTERNAL inline i32
FloorF32ToI32(f32 value) {
    const i32 result{ static_cast<i32>(floorf(value)) };
    return result;
}

NODISCARD
INTERNAL inline u32
FloorF32ToU32(f32 value) {
    ASSERT(value >= 0);
    const u32 result{ static_cast<u32>(floorf(value)) };
    return result;
}

NODISCARD
INTERNAL inline u32
RoundF32ToU32(f32 value) {
    ASSERT(value >= 0);
    const u32 result{ static_cast<u32>(value + 0.5f) };
    return result;
}

NODISCARD
INTERNAL inline i32
RoundF32ToI32(f32 value) {
    const i32 result{ static_cast<i32>(value + 0.5f) };
    return result;
}

NODISCARD
INTERNAL inline f32
Sin(f32 angle) {
    const f32 result{ sinf(angle) };
    return result;
}

NODISCARD
INTERNAL inline f32
Cos(f32 angle) {
    const f32 result{ cosf(angle) };
    return result;
}

NODISCARD
INTERNAL inline f32
ATan2(f32 y, f32 x) {
    const f32 result{ atan2f(y, x) };
    return result;
}

NODISCARD
INTERNAL inline u32
AbsI32ToU32(i32 value) {
    const u32 result{ static_cast<u32>(abs(value)) };
    return result;
}

#endif // HANDMADE_INTRINSICS_H
