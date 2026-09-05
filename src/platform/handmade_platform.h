#ifndef HANDMADE_PLATFORM_H
#define HANDMADE_PLATFORM_H

/*
    Platform specific code aimed for C-compatibility that is seperated from everything else
    TODO: This seperation needs to be refined, though, if we want the platform file to only include
    this file!
*/

#ifdef __cplusplus
extern "C" {
#endif

#include <float.h>  // various float definitions
#include <stddef.h> // size_t
#include <stdint.h> // common types

/// Compilers

// These can be specified from the command line also if one wants to, we do it here
// If specifying from the command line they have to be defined as equal to other than 0 to work
// -DCOMPILER_MSVC=1

#ifndef COMPILER_MSVC
#    define COMPILER_MSVC 0
#endif

#ifndef COMPILER_LLVM
#    define COMPILER_LLVM 0
#endif

// TODO: more compilers, GCC at least
// Determine the compiler if none set
#if !COMPILER_MSVC && !COMPILER_LLVM
#    if defined(__clang__)
#        undef COMPILER_LLVM
#        define COMPILER_LLVM 1
// Clang on windows also defines _MSC_VER this... it's a mess
// On the other hand, from the source code's point of view it should be the same if the code is
// compiled using MSVC or clang. clang pretends to be fully MSVC compatible so it makes sense that
// way. However if we really want to fully exclusively know which compiler we are using this just
// complicates things
#    elif defined(_MSC_VER)
#        undef COMPILER_MSVC
#        define COMPILER_MSVC 1
#    endif
#endif

// @Hack TODO: figure out a better way to exclude debug cycle reads and inclusion of intrin.h
#if HANDMADE_WEB
#    undef COMPILER_LLVM
#    define COMPILER_LLVM 0
#endif

/// SIMD

// TODO: ARM64?
// TODO: make SIMD a switch for the build script?
#if defined(_M_X64) || defined(_M_IX86) || defined(__x86_64__) || defined(__i386__)
#    define HANDMADE_SIMD 1
#    define HANDMADE_WASM_SIMD 0
#elif defined(__EMSCRIPTEN__)
#    define HANDMADE_SIMD 0
#    define HANDMADE_WASM_SIMD 1
#else
#    define HANDMADE_SIMD 0
#    define HANDMADE_WASM_SIMD 0
#endif

// This iffing requires explicit knowledge of the compiler, not some compiler conforming to one
// anothers' features
#if HANDMADE_SIMD
#    if COMPILER_MSVC
#        include <intrin.h>
#    elif COMPILER_LLVM // || COMPILER_GCC
// Clang (Windows, Linux, Mac), GCC (Linux)
#        include <x86intrin.h>
#    endif
#elif HANDMADE_WASM_SIMD
// TODO:
//#    include <wasm_simd128.h>
#endif

/// Typedefs for common types

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

#ifdef FLT_MAX
#    define F32_MAX FLT_MAX
#else
// TODO: error for now? or just define ourselves?
#    error FLT_MAX not defined!
#endif

// TODO: Wrap SIMD types???
// typedef f32x4 __mm128;

// A thread context passed to game code and is used when calling back to platform-specific code
typedef struct ThreadContext {
    i32 placeHolder;
} ThreadContext;

/// Services that the platform layer provides to the game ///

//#if HANDMADE_INTERNAL

typedef struct DEBUGFileReadResult {
    void* content;
    u32 contentSize;
} DEBUGFileReadResult;

enum {
    DEBUGCycleCounter_UpdateAndRender = 0,
    DEBUGCycleCounter_RenderGroupToOutput,

    DEBUGCycleCounter_DrawRectSlowly,
    DEBUGCycleCounter_DrawRectQuickly,
    DEBUGCycleCounter_TestPixel,
    DEBUGCycleCounter_FillPixel,

    DEBUGCycleCounter_Count
};

typedef struct DEBUGCycleCounter {
    u64 cycleCount;
    u32 hitCount; // How many times the function is called
} DEBUGCycleCounter;

#if COMPILER_MSVC || COMPILER_LLVM
#    define BEGIN_TIMED_BLOCK(id) u64 startCycleCount##id{ __rdtsc() };
#    define END_TIMED_BLOCK(id)                                                                    \
        gDebugMemory->counters[DEBUGCycleCounter_##id].cycleCount +=                               \
            __rdtsc() - startCycleCount##id;                                                       \
        ++gDebugMemory->counters[DEBUGCycleCounter_##id].hitCount;
//#elif COMPILER_GCC
//#    // TODO: make these work
//#    define BEGIN_TIMED_BLOCK(id) u64 startCycleCount##id{ __rdtsc() };
//#    define END_TIMED_BLOCK(id) \
//        gDebugMemory->counters[DEBUGCycleCounter_##id].cycleCount += \
//            _rdtsc() - startCycleCount##id; \
//        ++gDebugMemory->counters[DEBUGCycleCounter_##id].hitCount;
#else // TODO: web, let's see sometime
#    define BEGIN_TIMED_BLOCK(id)
#    define END_TIMED_BLOCK(id)
#endif

//#endif

// clang-format off
// TODO: our own versions?
#define DEBUG_PRINT(name) void name(ThreadContext* threadContext, const char* format, ...)
typedef DEBUG_PRINT(debug_print);

#define DEBUG_FREE_FILE_MEMORY(name) void name(ThreadContext* threadContext, void* memory)
typedef DEBUG_FREE_FILE_MEMORY(debug_free_file_memory);

#define DEBUG_READ_FILE(name) DEBUGFileReadResult name(ThreadContext* threadContext, const char* filename)
typedef DEBUG_READ_FILE(debug_read_file);

#define DEBUG_WRITE_FILE(name) bool32 name(ThreadContext* threadContext, const char* filename, void* memory, u32 fileSize)
typedef DEBUG_WRITE_FILE(debug_write_file);
// clang-format on

// Exported functions for the game
typedef struct PlatformExports {
    debug_print* DEBUGPrint;

    debug_free_file_memory* DEBUGFreeFileMemory;
    debug_read_file* DEBUGReadFile;
    debug_write_file* DEBUGWriteFile;
} PlatformExports;

// All the memory the game needs
typedef struct GameMemory {
    void* permanentStorage;
    // TODO: because we target wasm32, better ways to do this? JUST TARGET 64???
    // Although most browser already support wasm64 at the time 26/6/2026
    // As an exercise it would be best to keep it 32-bit just to see how the code needs to change
#if 0 // HANDMADE_WEB
    u32 permanentStorageSize;
#else
    u64 permanentStorageSize;
#endif

    void* transientStorage;
#if 0 // HANDMADE_WEB
    u32 transientStorageSize;
#else
    u64 transientStorageSize;
#endif

    bool32 isInitialized;

    PlatformExports exports;

#if HANDMADE_INTERNAL
    DEBUGCycleCounter counters[DEBUGCycleCounter_Count];
#endif
} GameMemory;

// Struct to hold screen buffer info
typedef struct OffScreenBuffer {
    void* memory;
    i32 width;
    i32 height;
    i32 pitch;
} OffScreenBuffer;

typedef struct SoundOutputBuffer {
    i16* samples;
    i32 samplesPerSecond;
    i32 sampleCount;
} SoundOutputBuffer;

// Keyboard button states
typedef struct Button {
    bool32 endedDown; // If the button ended down during the frame
    // The amount of times the state changed during the frame, with this we can deduce
    // if the button was just pressed, pressed continuously or just released
    i32 halfTransitionCount;
} Button;

typedef struct InputButtons {
    // A union allows us to do:
    // InputButtons b;
    // b[0] is the same as b.up;
    union {
        Button buttons[28];

        struct {
            Button up;
            Button down;
            Button left;
            Button right;

            Button actionUp;
            Button actionDown;
            Button actionLeft;
            Button actionRight;

            Button shift; // Right and left combined
            Button ctrl;  // -||-

            Button space;

            Button Q;
            Button E;

            Button R;
            Button F;
            Button Z;

            Button enter;

            Button F1;
            Button F2;
            Button F3;
            Button F4;
            Button F5;
            Button F6;
            Button F7;
            Button F8;
            Button F9;
            Button F10;

            // All new buttons must be above this
            Button terminator;
        };
    };
} InputButtons;

typedef struct MouseButtons {
    union {
        Button buttons[6];

        struct {
            Button left;
            Button middle;
            Button right;

            // Side buttons
            Button x1; // Closer
            Button x2; // Further

            Button terminator;
        };
    };
} MouseButtons;

typedef struct Input {
    InputButtons playerInputs[2]; // Essentially the player count

    MouseButtons mouseButtons;
    i32 mouseX, mouseY, mouseZ; // mouseZ is scroll

    f32 frameDeltaTime;

    bool32 executableReloaded;
} Input;

// We use the style 2 (Game as a service to the OS) described in the series

/// Services that the game provides to the platform layer ///

// clang-format off
#define GET_SOUND_SAMPLES(name) void name(ThreadContext* threadContext, GameMemory* memory, SoundOutputBuffer* soundBuff)
typedef GET_SOUND_SAMPLES(get_sound_samples);

#define UPDATE_AND_RENDER(name) void name(ThreadContext* threadContext, GameMemory* memory, OffScreenBuffer* screenBuff, Input* input)
typedef UPDATE_AND_RENDER(update_and_render);
// clang-format on

#ifdef __cplusplus
}
#endif

#endif // HANDMADE_PLATFORM_H
