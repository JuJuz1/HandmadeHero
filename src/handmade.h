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

/// Other includes for the platform and the game ///

// TODO: or just include explicitly?
// Also make sure templated code still works

#include "handmade_array.h"

/// Other includes for the platform and the game ///

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

#endif // HANDMADE_H
