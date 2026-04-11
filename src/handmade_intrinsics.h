#ifndef HANDMADE_INTRINSICS_H
#define HANDMADE_INTRINSICS_H

#include "handmade.h"

NODISCARD INTERNAL inline i32 TruncateI64toI32(i64 value);
NODISCARD INTERNAL inline u32 TruncateU64toU32(u64 value);
NODISCARD INTERNAL inline f32 TruncateF64toF32(f64 value);
NODISCARD INTERNAL inline i32 TruncateF32ToI32(f32 value);
NODISCARD INTERNAL inline u32 TruncateF32ToU32(f32 value);

NODISCARD INTERNAL inline i32 FloorF32ToI32(f32 value);
NODISCARD INTERNAL inline u32 FloorF32ToU32(f32 value);

NODISCARD INTERNAL inline i32 RoundF32ToI32(f32 value);
NODISCARD INTERNAL inline u32 RoundF32ToU32(f32 value);

NODISCARD INTERNAL inline i32 CeilF32ToI32(f32 value);
NODISCARD INTERNAL inline u32 CeilF32ToU32(f32 value);

NODISCARD INTERNAL inline f32 Sin(f32 angle);
NODISCARD INTERNAL inline f32 Cos(f32 angle);
NODISCARD INTERNAL inline f32 ATan2(f32 y, f32 x);

NODISCARD INTERNAL inline f32 Sqrt(f32 value);

NODISCARD INTERNAL inline u32 AbsI32(i32 value);
NODISCARD INTERNAL inline f32 AbsF32(f32 value);

NODISCARD INTERNAL inline f32 ExpF32(f32 value);

struct BitscanResult {
    bool32 found;
    u32 index;
};

NODISCARD INTERNAL inline BitscanResult FindLeastSignificantBitSet(u32 value);

NODISCARD INTERNAL inline i32 SignOf(i32 value);

#endif // HANDMADE_INTRINSICS_H
