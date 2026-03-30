#ifndef HANDMADE_INTRINSICS_H
#define HANDMADE_INTRINSICS_H

/*
    TODO: implement these functions ourself using platform-efficient versions and remove cmath
   include. As a result this file's contents can access the platform layer!

   https://learn.microsoft.com/en-us/cpp/intrinsics/compiler-intrinsics?view=msvc-170
   https://www.intel.com/content/www/us/en/docs/intrinsics-guide/index.html
*/

#include <cmath>

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
INTERNAL inline i32
RoundF32ToI32(f32 value) {
    const i32 result{ static_cast<i32>(roundf(value)) };
    return result;
}

NODISCARD
INTERNAL inline u32
RoundF32ToU32(f32 value) {
    ASSERT(value >= 0);
    const u32 result{ static_cast<u32>(roundf(value)) };
    return result;
}

NODISCARD
INTERNAL inline i32
CeilF32ToI32(f32 value) {
    const i32 result{ static_cast<i32>(ceilf(value)) };
    return result;
}

NODISCARD
INTERNAL inline u32
CeilF32ToU32(f32 value) {
    ASSERT(value >= 0);
    const u32 result{ static_cast<u32>(ceilf(value)) };
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
INTERNAL inline f32
Sqrt(f32 value) {
    const f32 result{ sqrtf(value) };
    return result;
}

NODISCARD
INTERNAL inline u32
AbsI32ToU32(i32 value) {
    const u32 result{ static_cast<u32>(abs(value)) };
    return result;
}

NODISCARD
INTERNAL inline f32
AbsF32(f32 value) {
    const f32 result{ fabs(value) };
    return result;
}

struct BitscanResult {
    bool32 found;
    u32 index;
};

NODISCARD
INTERNAL inline BitscanResult
FindLeastSignificantBitSet(u32 value) {
    BitscanResult result{};

#if COMPILER_MSVC
    // Why is long different from int... (...but not on MSVC! C++...)
    result.found = _BitScanForward(reinterpret_cast<unsigned long*>(&result.index), value);
#else
    for (u32 i{}; i < 32; ++i) {
        if (value & (1 << i)) {
            result.found = true;
            result.index = i;
            break;
        }
    }
#endif

    return result;
}

#endif // HANDMADE_INTRINSICS_H
