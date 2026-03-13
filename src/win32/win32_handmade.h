#ifndef WIN32_HANDMADE_H
#define WIN32_HANDMADE_H

// Compiler switch
#if HANDMADE_WIN32

namespace win32 {

struct OffScreenBuffer {
    BITMAPINFO info;
    void* memory;
    i32 width;
    i32 height;
    u32 bytesPerPixel;
    i32 pitch;
};

// "Secondary" buffer values
struct SoundOutput {
    u32 samplesPerSecond;
    u32 bytesPerSample;
    u32 buffSize;
    u32 latencySampleCount;
    u32 runningSampleIndex;
};

struct WindowDimension {
    i32 width;
    i32 height;
};

} //namespace win32

#endif // HANDMADE_WIN32
#endif // WIN32_HANDMADE_H
