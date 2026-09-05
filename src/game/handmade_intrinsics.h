#ifndef HANDMADE_INTRINSICS_H
#define HANDMADE_INTRINSICS_H

//#include "game/handmade.h"

//NODISCARD
//INTERNAL inline i32 Trunc(i64 value);
//NODISCARD
//INTERNAL inline u32 Trunc(u64 value);
//NODISCARD
//INTERNAL inline f32 Trunc(f64 value);
//NODISCARD
//INTERNAL inline i32 Trunc(f32 value);
//NODISCARD
//INTERNAL inline u32 Trunc(f32 value);

//NODISCARD
//INTERNAL inline i32 FloorToI32(f32 value);
//NODISCARD
//INTERNAL inline u32 FloorToU32(f32 value);

//NODISCARD
//INTERNAL inline i32 RoundToI32(f32 value);
//NODISCARD
//INTERNAL inline u32 RoundToU32(f32 value);

//NODISCARD
//INTERNAL inline i32 CeilToI32(f32 value);
//NODISCARD
//INTERNAL inline u32 CeilToU32(f32 value);

//NODISCARD
//INTERNAL inline f32 Sin(f32 angle);
//NODISCARD
//INTERNAL inline f32 Cos(f32 angle);
//NODISCARD
//INTERNAL inline f32 ATan2(f32 y, f32 x);

//NODISCARD
//INTERNAL inline f32 Sqrt(f32 value);

//NODISCARD
//INTERNAL inline u32 Abs(i32 value);
//NODISCARD
//INTERNAL inline f32 Abs(f32 value);

//NODISCARD
//INTERNAL inline f32 Exp(f32 value);

struct BitscanResult {
    bool32 found;
    u32 index;
};

//NODISCARD
//INTERNAL inline BitscanResult FindLeastSignificantBitSet(u32 value);

//NODISCARD
//INTERNAL inline i32 SignOf(i32 value);

#endif // HANDMADE_INTRINSICS_H
