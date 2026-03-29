#ifndef HANDMADE_H
#define HANDMADE_H

/*
    This file acts as the shared interface between the platform layer and the game code

    - It declares the functions the platform calls into the game (UpdateAndRender, etc.)
    - It declares the services the game can call back into the platform (file I/O, debug, etc.)
    - It defines shared data structures (GameMemory, Input, etc.) that both sides must agree on

    This allows the game code to be compiled separately (e.g. as a DLL) and reloaded without
    restarting the platform layer

    Split into handmade_platform.h which is designed to be C-compatible
*/

#include "handmade_platform.h"

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

/// Services that the platform layer provides to the game ///

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

#include "handmade_intrinsics.h"
#include "handmade_math.h"
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

#define PushSize(arena, type) (type*)_PushSize(arena, sizeof(type))
#define PushArray(arena, count, type) (type*)_PushSize(arena, (count) * sizeof(type))

NODISCARD
INTERNAL void*
_PushSize(MemoryArena* arena, memory_index size) {
    ASSERT(arena->used + size <= arena->size);
    void* result{ arena->base + arena->used };
    arena->used += size;

    return result;
}

struct World {
    Tilemap* tilemap;
};

struct LoadedBitmapInfo {
    u32* pixels;
    i32 width;
    i32 height;
};

struct HeroBitmaps {
    LoadedBitmapInfo head;
    LoadedBitmapInfo torso;
    Vec2 align;
};

struct Entity {
    TilemapPosition pos;
    Vec2 velocity;

    Vec2 dimensions;
    i32 facingDir;

    bool32 exists;
};

// The game state
struct GameState {
    MemoryArena worldArena;
    World* world;

    TilemapPosition cameraPos;
    i32 cameraFollowingEntityIndex; // By default the first player (index 1)

    Entity entities[256];
    i32 entityCount;
    // Cursed... probably reconsider your use free will
    i32 playerIndexForController[ARRAY_COUNT((static_cast<Input*>(0))->playerInputs)];

    LoadedBitmapInfo background;
    HeroBitmaps heroBitmaps[4];
};

#endif // HANDMADE_H
