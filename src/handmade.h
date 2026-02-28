#ifndef HANDMADE_H
#define HANDMADE_H

#include <cstdint>

// Ensure we are compiling as 64-bit for now...
// NOTE: is this a good way?
static_assert(sizeof(void*) == 8, "Size of pointer is not 8!");

// Defines for static
#define INTERNAL static
#define GLOBAL static
#define LOCAL_PERSIST static

// Typedefs for common types
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef i32 bool32;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

GLOBAL constexpr f32 PI32{ 3.14159265359f };

// Struct to hold buffer info
struct OffScreenBuffer {
    void* memory;
    i32 width;
    i32 height;
    u32 bytesPerPixel;
    u32 pitch;
};

// We use the style 2 (Game as a service to the OS) described in the series

/// Services that the platform layer provides to the game ///

/// ----------------------------------------------------- ///

/// Services that the game provides to the platform layer ///

// Input, bitmap buffer, sound buffer and timing
void GameUpdateAndRender(OffScreenBuffer* buff, u32 xOffset, u32 yOffset);

#endif // HANDMADE_H
