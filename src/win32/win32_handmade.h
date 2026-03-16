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

struct DSoundParams {
    DWORD byteToLock;
    DWORD targetCursor;
    DWORD bytesToWrite;
};

struct WindowDimension {
    i32 width;
    i32 height;
};

struct GameCode {
    HMODULE dll;
    FILETIME lastWritetime;

    // IMPORTANT: these can be 0 as we removed the stubs so we need to check before calling
    game::dll::update_and_render* updateAndRender;
    game::dll::get_sound_samples* getSoundSamples;

    bool32 isValid;
};

// NOTE: don't use MAX_PATH in user code as it can be dangerous
GLOBAL u32 constexpr allStateFileNameCount{ MAX_PATH };

// Buffers to store the game memory in memory instead of disk
struct ReplayBuffer {
    HANDLE fileHandle;
    HANDLE memoryMap;
    void* memoryBlock;
    char replayFilePath[allStateFileNameCount];
};

GLOBAL u8 constexpr replayBufferCount{ 4 };

// NOTE: not really all state (yet?)
struct AllState {
    //game::Input* input;
    //u32 inputCount;

    void* gameMemory;
    u64 memorySize;
    ReplayBuffer replayBuffers[replayBufferCount];

    char exePath[allStateFileNameCount];
    char* exeFilename;

    u32 recordingIndex;
    u32 playingIndex;
    HANDLE recordingHandle;
    HANDLE playingHandle;
};

} //namespace win32

#endif // HANDMADE_WIN32
#endif // WIN32_HANDMADE_H
