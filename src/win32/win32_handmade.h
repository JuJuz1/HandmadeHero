#ifndef WIN32_HANDMADE_H
#define WIN32_HANDMADE_H

// Compiler switch
#if HANDMADE_WIN32

// NOTE: don't use MAX_PATH in user code as it can be dangerous
#define WIN32_ALL_STATE_FILE_NAME_COUNT MAX_PATH

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

struct GameCode {
    HMODULE dll;
    FILETIME lastWritetime;

    // IMPORTANT: these can be 0 as we removed the stubs so we need to check before calling
    game::dll::update_and_render* updateAndRender;
    game::dll::get_sound_samples* getSoundSamples;

    bool32 isValid;
};

struct ReplayBuffer {
    HANDLE fileHandle;
    HANDLE memoryMap;
    void* memoryBlock;
    char replayFilePath[WIN32_ALL_STATE_FILE_NAME_COUNT];
};

// NOTE: not really all state (yet?)
struct AllState {
    //game::Input* input;
    //u32 inputCount;

    void* gameMemory;
    u64 memorySize;
    ReplayBuffer replayBuffers[4];

    char exePath[WIN32_ALL_STATE_FILE_NAME_COUNT];
    char* exeFilename;

    u32 recordingIndex;
    u32 playingIndex;
    HANDLE recordingHandle;
    HANDLE playingHandle;
};

} //namespace win32

#endif // HANDMADE_WIN32
#endif // WIN32_HANDMADE_H
