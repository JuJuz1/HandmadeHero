#ifndef HANDMADE_H
#define HANDMADE_H

#include <cstdint>

/*
This file acts as the shared interface between the platform layer and the game code

- It declares the functions the platform calls into the game (UpdateAndRender, etc.)
- It declares the services the game can call back into the platform (file I/O, debug, etc.)
- It defines shared data structures (GameMemory, Input, etc.) that both sides must agree on

This allows the game code to be compiled separately (e.g. as a DLL) and reloaded without
restarting the platform layer
*/

// Ensure we are compiling as 64-bit for now...
// NOTE: is this a good way?
static_assert(sizeof(void*) == 8, "Size of pointer is not 8!");

// Compilers

#ifndef COMPILER_MSVC
#    define COMPILER_MSVC 0
#endif

#ifndef COMPILER_LLVM
#    define COMPILER_LLVM 0
#endif

// Determine the compiler if none set
// TODO: more compilers
#if !COMPILER_MSVC && !COMPILER_LLVM
#    if _MSC_VER
#        undef COMPILER_MSVC
#        define COMPILER_MSVC 1
#    else
#        undef COMPILER_LLVM
#        define COMPILER_LLVM 1
#    endif
#endif

/*
HANDMADE_INTERNAL:
    0: release builds
    1: developer builds

HANDMADE_DEBUG:
    0: enables assertions
    1: disables assertions
*/

// TODO: use these ASSERT(s) vs assert from <cassert>?
// probably just make this better to include more information (error messages and variadic
// arguments of values?)
#if HANDMADE_DEBUG
// clang-format off
    #define ASSERT(expr) if (!(expr)) { *(static_cast<int*>(0)) = 0; } // clang-tidy NOLINT
// clang-format on
#else
#    define ASSERT(expr)
#endif

// Defines for different meanings of static

#define INTERNAL static
#define GLOBAL static
#define LOCAL_PERSIST static

#define ARRAY_COUNT(arr) (sizeof(arr) / sizeof((arr)[0]))

// Integral promotion, otherwise the value might wrap to 0 because we hit u32 max...
#define KILOBYTES(count) ((count) * 1024LL)
#define MEGABYTES(count) (KILOBYTES(count) * 1024LL)
#define GIGABYTES(count) (MEGABYTES(count) * 1024LL)
#define TERABYTES(count) (GIGABYTES(count) * 1024LL)

#define UNUSED_PARAMS(...) (void)(__VA_ARGS__)

// c++17 required
// Can be opted out of easily by just checking c++ standard version when compiling
// Although on MSVC at least when compiling with an older standard like c++14, it still
// works and produces the warning
#define NODISCARD [[nodiscard]]

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

typedef size_t memory_index;

/// Services that the platform layer provides to the game ///

// A thread context passed to game code and is used when calling back to platform-specific code
struct ThreadContext {
    i32 placeHolder;
};

#if HANDMADE_INTERNAL

namespace platform_export {

struct DEBUGFileReadResult {
    void* content;
    i32 contentSize;
};

// clang-format off
#define DEBUG_PRINT(name) void name(ThreadContext* threadContext, const char* message)
typedef DEBUG_PRINT(debug_print);

#define DEBUG_PRINT_INT(name) void name(ThreadContext* threadContext, const char* valueName, i32 value)
typedef DEBUG_PRINT_INT(debug_print_int);

#define DEBUG_PRINT_FLOAT(name) void name(ThreadContext* threadContext, const char* valueName, f32 value)
typedef DEBUG_PRINT_FLOAT(debug_print_float);

#define DEBUG_FREE_FILE_MEMORY(name) void name(ThreadContext* threadContext, void* memory)
typedef DEBUG_FREE_FILE_MEMORY(debug_free_file_memory);

#define DEBUG_READ_FILE(name) DEBUGFileReadResult name(ThreadContext* threadContext, const char* filename)
typedef DEBUG_READ_FILE(debug_read_file);

#define DEBUG_WRITE_FILE(name) bool32 name(ThreadContext* threadContext, const char* filename, void* memory, u32 fileSize)
typedef DEBUG_WRITE_FILE(debug_write_file);
// clang-format on

// Exported functions for the game
struct PlatformExports {
    platform_export::debug_print* DEBUGPrint;
    platform_export::debug_print_int* DEBUGPrintInt;
    platform_export::debug_print_float* DEBUGPrintFloat;

    platform_export::debug_free_file_memory* DEBUGFreeFileMemory;
    platform_export::debug_read_file* DEBUGReadFile;
    platform_export::debug_write_file* DEBUGWriteFile;
};

} //namespace platform_export

#endif // HANDMADE_INTERNAL

GLOBAL constexpr f32 PI32f{ 3.14159265359f };

INTERNAL void
CatStrings(const char* srcA, i64 srcASize, const char* srcB, i64 srcBSize, char* dest,
           i64 destSize) {
    // Space for null terminator
    i64 total{ srcASize + srcBSize };
    if (total >= destSize) {
        total = destSize - 1;
    }

    for (i32 i{}; i < srcASize && i < total; ++i) {
        *dest++ = *srcA++;
    }

    for (i32 i{}; i < srcBSize && i < total; ++i) {
        *dest++ = *srcB++;
    }

    *dest++ = '\0';
}

NODISCARD
INTERNAL i32
StrLength(const char* str) {
    ASSERT(str);

    i32 len{};
    while (*str++) {
        len++;
    }

    return len;
}

/// Game specific code ///

//namespace game {

// TODO: experiment with more than 1
GLOBAL constexpr i32 player_Count{ 1 };

// All the memory the game needs
struct GameMemory {
    void* permanentStorage;
    u64 permanentStorageSize;

    void* transientStorage;
    u64 transientStorageSize;

    bool32 isInitialized;

#if HANDMADE_INTERNAL
    platform_export::PlatformExports exports;
#endif
};

// Struct to hold screen buffer info
struct OffScreenBuffer {
    void* memory;
    i32 width;
    i32 height;
    i32 bytesPerPixel;
    i32 pitch;
};

struct SoundOutputBuffer {
    i16* samples;
    i32 samplesPerSecond;
    i32 sampleCount;
};

// Keyboard button states
struct Button {
    bool32 endedDown; // If the button ended down during the frame
    // The amount of times the state changed during the frame, with this we can deduce
    // if the button was just pressed, pressed continuously or just released
    i32 halfTransitionCount;
};

GLOBAL constexpr i32 button_Count{ 8 };
GLOBAL constexpr i32 mouse_Button_Count{ 5 };

struct InputButtons {
    // A union allows us to do:
    // InputButtons b;
    // b[0] is the same as b.up;
    union {
        Button buttons[button_Count];

        struct {
            Button up;
            Button down;
            Button left;
            Button right;

            Button shift;

            Button Q;
            Button E;

            Button Z;
        };
    };
};

struct MouseButtons {
    union {
        Button buttons[mouse_Button_Count];

        struct {
            Button left;
            Button middle;
            Button right;

            // Side buttons
            Button x1; // Closer
            Button x2; // Further
        };
    };
};

struct Input {
    InputButtons playerInputs[player_Count];

    MouseButtons mouseButtons;
    i32 mouseX, mouseY, mouseZ; // mouseZ is scroll

    f32 frameDeltaTime;
};

static_assert(sizeof(InputButtons) == sizeof(Button) * button_Count,
              "Inputbuttons count doesn't match the amount of buttons declared!");
static_assert(sizeof(MouseButtons) == sizeof(Button) * mouse_Button_Count,
              "MouseButtons count doesn't match the amount of buttons declared!");

#include "handmade_intrinsics.h"
#include "handmade_tile.h"

struct MemoryArena {
    u8* base;
    memory_index size;
    memory_index used;
};

INTERNAL void
InitializeArena(MemoryArena* arena, u8* base, memory_index size) {
    arena->base = base;
    arena->size = size;
    arena->used = 0;
}

#define PushSize(arena, type) (type*)PushSize_(arena, sizeof(type))
#define PushArray(arena, count, type) (type*)PushSize_(arena, (count) * sizeof(type))

NODISCARD
INTERNAL void*
PushSize_(MemoryArena* arena, memory_index size) {
    ASSERT(arena->used + size <= arena->size);
    void* result{ arena->base + arena->used };
    arena->used += size;

    return result;
}

struct World {
    Tilemap* tileMap;
};

struct LoadedBitmapInfo {
    u32* pixels;
    i32 width;
    i32 height;
};

// The game state
struct GameState {
    MemoryArena worldArena;
    World* world;

    TilemapPosition playerPos;

    LoadedBitmapInfo background;
    LoadedBitmapInfo playerHead;
};

// We use the style 2 (Game as a service to the OS) described in the series

/// Services that the game provides to the platform layer ///

//namespace dll {

// clang-format off
#define GET_SOUND_SAMPLES(name) void name(ThreadContext* threadContext, GameMemory* memory, const SoundOutputBuffer* soundBuff)
typedef GET_SOUND_SAMPLES(get_sound_samples);

#define UPDATE_AND_RENDER(name) void name(ThreadContext* threadContext, GameMemory* memory, const OffScreenBuffer* screenBuff, const Input* input)
typedef UPDATE_AND_RENDER(update_and_render);
// clang-format on

//} //namespace dll

//} //namespace game

#endif // HANDMADE_H
