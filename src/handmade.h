#ifndef HANDMADE_H
#define HANDMADE_H

#include <cmath> // TODO: write own sinf
#include <cstdint>

// Ensure we are compiling as 64-bit for now...
// NOTE: is this a good way?
static_assert(sizeof(void*) == 8, "Size of pointer is not 8!");

/*
HANDMADE_INTERNAL:
    0: release builds
    1: developer builds

HANDMADE_DEBUG:
    0: enables assertions
    1: disables assertions
*/

// TODO: use these ASSERT(s) vs assert from <cassert>?
// probably just make this better to include more information (error messages and variadic arguments
// of values?)
#ifdef HANDMADE_DEBUG
// clang-format off
#define ASSERT(expr) if (!(expr)) { *(static_cast<int*>(0)) = 0; }
// clang-format on
#else
#define ASSERT(expr)
#endif

// Defines for static
#define INTERNAL static
#define GLOBAL static
#define LOCAL_PERSIST static

#define ARRAY_COUNT(arr) (sizeof(arr) / sizeof((arr)[0]))

// Integral promotion, otherwise the value might wrap to 0 because we hit u32 max...

#define KILOBYTES(count) ((count) * 1024LL)
#define MEGABYTES(count) (KILOBYTES(count) * 1024LL)
#define GIGABYTES(count) (MEGABYTES(count) * 1024LL)
#define TERABYTES(count) (GIGABYTES(count) * 1024LL)

// Typedefs for common types
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef i32 bool32; // We never use bool here :)

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

#if HANDMADE_INTERNAL

/// Services that the platform layer provides to the game ///

namespace platform {

INTERNAL void DEBUGPrintInt(const char* name, i32 value);
INTERNAL void DEBUGPrintFloat(const char* name, f32 value);

struct DEBUGFileReadResult {
    void* content;
    u32 contentSize;
};

INTERNAL DEBUGFileReadResult DEBUGPlatformReadFile(const char* filename);
INTERNAL void DEBUGPlatformFreeFileMemory(void* memory);
// TODO: make this safer i.e. protect against lost data e.g. if the write succeeds only partially
INTERNAL bool32 DEBUGPlatformWriteFile(const char* filename, void* memory, u32 fileSize);

} //namespace platform

#endif // HANDMADE_INTERNAL

GLOBAL constexpr f32 PI32{ 3.14159265359f };

INTERNAL inline i32
SafeTruncateI64toI32(i64 value) {
    ASSERT(value <= 2147483647LL);
    ASSERT(value >= -2147483648LL);
    return static_cast<i32>(value);
}

INTERNAL inline u32
SafeTrunateU64toU32(u64 value) {
    // TODO: U32_MAX and such...
    ASSERT(value <= 0xFFFFFFFFULL);
    return static_cast<u32>(value);
}

INTERNAL inline f32
SafeTruncateF64toF32(f64 value) {
    ASSERT(value <= 3.402823466e+38);
    ASSERT(value >= -3.402823466e+38);
    return static_cast<f32>(value);
}

namespace game {

// TODO: experiment with more than 1
GLOBAL constexpr u8 playerCount{ 1 };

// All the memory the game needs
struct GameMemory {
    void* permanentStorage;
    u64 permanentStorageSize;

    void* transientStorage;
    u64 transientStorageSize;

    bool32 isInitialized;
};

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

// Keyboard button states
struct Button {
    bool32 endedDown; // If the button ended down during the frame
    // The amount of times the state changed during the frame, with this we can deduce
    // if the button was just pressed, pressed continuosly or just released
    u32 halfTransitionCount;
};

GLOBAL constexpr u8 buttonCount{ 6 };

struct InputButtons {
    // A union allows us to do:
    // InputButtons b;
    // b[0] is the same as b.up;
    union {
        Button buttons[buttonCount];

        struct {
            Button up;
            Button down;
            Button left;
            Button right;

            Button Q;
            Button E;
        };
    };
};

struct Input {
    // TODO: insert frame statistics
    InputButtons playerInputs[playerCount];
};

static_assert(sizeof(InputButtons) == sizeof(Button) * buttonCount,
              "Inputbuttons count doesn't match the amount of buttons declared!");

namespace input {

INTERNAL bool32
ActionJustPressed(const Button* button) {
    return button->endedDown && button->halfTransitionCount > 0;
}

INTERNAL bool32
ActionPressed(const Button* button) {
    return button->endedDown;
}

INTERNAL bool32
ActionReleased(const Button* button) {
    return !button->endedDown && button->halfTransitionCount > 0;
}

} //namespace input

// We use the style 2 (Game as a service to the OS) described in the series

/// Services that the game provides to the platform layer ///

// Game's "main loop"
INTERNAL void UpdateAndRender(GameMemory* memory, const OffScreenBuffer* buff,
                              const SoundOutputBuffer* soundBuff, const Input* input);

struct GameState {
    u32 xOffset;
    u32 yOffset;
    u32 toneHz;
};

} //namespace game

#endif // HANDMADE_H
