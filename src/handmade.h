#ifndef HANDMADE_H
#define HANDMADE_H

/*
    This file acts as the shared interface between the platform layer and the game code

    - It declares the functions the platform calls into the game (UpdateAndRender, etc.)
    - It declares the services the game can call back into the platform (file I/O, debug, etc.)
    - It defines shared data structures (GameMemory, Input, etc.) that both sides must agree on

    This allows the game code to be compiled separately (as a DLL) and hotreloaded without
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
    0: disables assertions
    1: enables assertions
*/

// TODO: use these ASSERT(s) vs assert from <cassert>?
// probably just make this better to include more information (error messages and variadic
// arguments of values?)
#if HANDMADE_DEBUG
// clang-format off
// clang-tidy NOLINTBEGIN
    #define ASSERT(expr) if (!(expr)) { *(static_cast<int*>(nullptr)) = 0; }
    // This is to detect invalid paths that we should never enter, BUT shouldn't cause a crash
    // TODO: probably log also
    #define INVALID_CODE_PATH ASSERT(!"INVALID CODE PATH")
// clang-format on
// clang-tidy NOLINTEND
#else
#    define ASSERT(expr)
#endif

// Defines for different meanings of static

#define INTERNAL static
#define GLOBAL static
#define LOCAL_PERSIST static
#define NOT_BOUND static // The member variable is not bound to the struct, used by Array

#define ARRAY_COUNT(arr) (sizeof(arr) / sizeof((arr)[0]))

// Integral promotion, otherwise the value might wrap to 0 because we hit u32 max...
#define KILOBYTES(count) ((count) * 1024LL)
#define MEGABYTES(count) (KILOBYTES(count) * 1024LL)
#define GIGABYTES(count) (MEGABYTES(count) * 1024LL)
#define TERABYTES(count) (GIGABYTES(count) * 1024LL)

#define UNUSED_PARAMS(...) (void)(__VA_ARGS__)

// Returns the first if equal, same for MAX
#define MIN(a, b) (((a) <= (b)) ? (a) : (b))
#define MAX(a, b) (((a) >= (b)) ? (a) : (b))

// C++17 required
// Although on MSVC at least when compiling with an older standard like C++14, it still
// works and produces the warnings correctly. Clang warns when using an older standard!
#if __cplusplus >= 201703L
#    define NODISCARD [[nodiscard]]
#else
#    define NODISCARD
#endif

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

#include "handmade_array.h"
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

/// Entities ///

enum class EntityType {
    NON_EXISTENT = 0,
    HERO,
    WALL,
};

/**
 * Low frequency entity meant to be "ticked" at a slower rate compared to high frequency
 */
struct LowEntity {
    EntityType type;

    TilemapPosition pos;
    f32 width, height;

    bool32 collides;
    i32 dAbsTileZ; // Stairs

    i32 highEntityIndex;
};

/**
 * High frequency
 */
struct HighEntity {
    Vec2 pos; // NOTE: This is now already relative to the camera center
    u32 absTileZ;
    Vec2 velocity;

    i32 facingDir;

    i32 lowEntityIndex;
};

struct Entity {
    LowEntity* low;
    HighEntity* high;
    i32 lowIndex;
};

/**
 * The game state!
 */
struct GameState {
    MemoryArena worldArena;
    World* world;

    TilemapPosition cameraPos;
    i32 cameraFollowingEntityIndex; // By default the first player (index 1)

    Array<LowEntity, 4096> lowEntities;   // Holds all entities
    Array<HighEntity, 256> highEntities_; // Holds the entities marked as high frequency
    i32 lowEntityCount;
    i32 highEntityCount;

    Array<i32, ARRAY_COUNT(Input::playerInputs)> playerIndexFromController;

    LoadedBitmapInfo background;
    Array<HeroBitmaps, 4> heroBitmaps;

    bool32 startWithAPlayer;
};

#endif // HANDMADE_H
