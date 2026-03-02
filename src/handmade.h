#ifndef HANDMADE_H
#define HANDMADE_H

#include <cmath> // TODO: write own sinf
#include <cstdint>

// Ensure we are compiling as 64-bit for now...
// NOTE: is this a good way?
static_assert(sizeof(void*) == 8, "Size of pointer is not 8!");

// Defines for static
#define INTERNAL static
#define GLOBAL static
#define LOCAL_PERSIST static

#define ARRAY_COUNT(arr) (sizeof(arr) / sizeof((arr)[0]))

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

namespace game {

// Struct to hold buffer info
struct OffScreenBuffer {
    void* memory;
    i32 width;
    i32 height;
    u32 pitch;
};

struct SoundOutputBuffer {
    u32 samplesPerSecond;
    u32 sampleCount;
    i16* samples;
};

struct ButtonState {
    //u32 halfTransitionCount; // For controllers
    bool32 endedDown; // If the button was down at the end of the frame
};

struct InputButtons {
    ButtonState up;
    ButtonState down;
    ButtonState left;
    ButtonState right;
};

GLOBAL constexpr u8 playerCount{ 2 };

struct Input {
    InputButtons playerInputs[playerCount];
};

// We use the style 2 (Game as a service to the OS) described in the series

/// Services that the platform layer provides to the game ///

/// ----------------------------------------------------- ///

/// Services that the game provides to the platform layer ///

// Input, bitmap buffer, sound buffer and timing
INTERNAL void UpdateAndRender(const OffScreenBuffer* buff, const SoundOutputBuffer* soundBuff,
                              const Input* input);

} //namespace game

#endif // HANDMADE_H
