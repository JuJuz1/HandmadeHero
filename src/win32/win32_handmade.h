#ifndef WIN32_HANDMADE_H
#define WIN32_HANDMADE_H

struct Win32OffScreenBuffer {
    BITMAPINFO info;
    void* memory;
    i32 width;
    i32 height;
    u32 bytesPerPixel;
    u32 pitch;
};

struct WindowDimension {
    i32 width;
    i32 height;
};

// Secondary buffer values
struct Win32SoundOutput {
    u32 samplesPerSecond;
    u32 bytesPerSample;
    u32 buffSize;
    u32 latencySampleCount;
    u32 runningSampleIndex;
};

#endif // WIN32_HANDMADE_H
