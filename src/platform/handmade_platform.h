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

#include <stddef.h> // size_t
#include <stdint.h> // common types

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

#if COMPILER_MSVC
#    include <intrin.h>
#endif

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

// A thread context passed to game code and is used when calling back to platform-specific code
typedef struct ThreadContext {
    i32 placeHolder;
} ThreadContext;

#if HANDMADE_INTERNAL

/// Services that the platform layer provides to the game ///

typedef struct DEBUGFileReadResult {
    void* content;
    u32 contentSize;
} DEBUGFileReadResult;

// clang-format off
#define DEBUG_PRINT(name) void name(ThreadContext* threadContext, const char* message)
typedef DEBUG_PRINT(debug_print);

#define DEBUG_PRINT_I32(name) void name(ThreadContext* threadContext, const char* valueName, i32 value)
typedef DEBUG_PRINT_I32(debug_print_i32);

#define DEBUG_PRINT_U32(name) void name(ThreadContext* threadContext, const char* valueName, u32 value)
typedef DEBUG_PRINT_U32(debug_print_u32);

#define DEBUG_PRINT_F32(name) void name(ThreadContext* threadContext, const char* valueName, f32 value)
typedef DEBUG_PRINT_F32(debug_print_f32);

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
    debug_print_i32* DEBUGPrintInt;
    debug_print_u32* DEBUGPrintUInt;
    debug_print_f32* DEBUGPrintFloat;

    debug_free_file_memory* DEBUGFreeFileMemory;
    debug_read_file* DEBUGReadFile;
    debug_write_file* DEBUGWriteFile;
} PlatformExports;

#endif // HANDMADE_INTERNAL

// All the memory the game needs
typedef struct GameMemory {
    void* permanentStorage;
    u64 permanentStorageSize;

    void* transientStorage;
    u64 transientStorageSize;

    bool32 isInitialized;

#if HANDMADE_INTERNAL
    PlatformExports exports;
#endif
} GameMemory;

// Struct to hold screen buffer info
typedef struct OffScreenBuffer {
    void* memory;
    i32 width;
    i32 height;
    i32 bytesPerPixel;
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
        Button buttons[10];

        struct {
            Button up;
            Button down;
            Button left;
            Button right;

            Button shift;

            Button space;

            Button Q;
            Button E;

            Button enter;

            // All new buttons have to be above this
            Button Z;
        };
    };
} InputButtons;

typedef struct MouseButtons {
    union {
        Button buttons[5];

        struct {
            Button left;
            Button middle;
            Button right;

            // Side buttons
            Button x1; // Closer
            Button x2; // Further
        };
    };
} MouseButtons;

typedef struct Input {
    InputButtons playerInputs[2];

    MouseButtons mouseButtons;
    i32 mouseX, mouseY, mouseZ; // mouseZ is scroll

    f32 frameDeltaTime;
} Input;

// We use the style 2 (Game as a service to the OS) described in the series

/// Services that the game provides to the platform layer ///

// clang-format off
#define GET_SOUND_SAMPLES(name) void name(ThreadContext* threadContext, GameMemory* memory, const SoundOutputBuffer* soundBuff)
typedef GET_SOUND_SAMPLES(get_sound_samples);

#define UPDATE_AND_RENDER(name) void name(ThreadContext* threadContext, GameMemory* memory, const OffScreenBuffer* screenBuff, const Input* input)
typedef UPDATE_AND_RENDER(update_and_render);
// clang-format on

#ifdef __cplusplus
}
#endif

#endif // HANDMADE_PLATFORM_H
