#ifndef WIN32_HANDMADE_H
#define WIN32_HANDMADE_H

// Compiler switch
#if HANDMADE_WIN32

namespace hm_win32 {

struct OffScreenBuffer {
    BITMAPINFO info;
    void* memory;
    i32 width;
    i32 height;
    i32 bytesPerPixel;
    i32 pitch;
};

// "Secondary" buffer values
struct SoundOutput {
    i32 samplesPerSecond;
    i32 bytesPerSample;
    i32 buffSize;
    i32 latencySampleCount;
    i32 runningSampleIndex;
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
    update_and_render* updateAndRender;
    get_sound_samples* getSoundSamples;

    bool32 isValid;
};

// NOTE: don't use MAX_PATH in user code as it can be dangerous
GLOBAL constexpr i32 file_Name_Count{ MAX_PATH };

// Buffers to store the game memory in memory instead of disk
struct ReplayBuffer {
    HANDLE fileHandle;
    HANDLE memoryMap;
    void* memoryBlock;
    Array<char, file_Name_Count> replayFilePath[file_Name_Count];

    bool32 isRecordedAtLeastOnce;
};

GLOBAL constexpr i32 replay_Buffer_Count{ 4 };
GLOBAL constexpr i32 replay_Buffer_Not_Recording{ -1 };
GLOBAL constexpr i32 replay_Buffer_Not_Playing{ -1 };

// NOTE: not really all state (yet?)
struct AllState {
    void* gameMemory;
    u64 memorySize;
    Array<ReplayBuffer, replay_Buffer_Count> replayBuffers;

    Array<char, file_Name_Count> exePath;
    char* exeFilename;

    i32 selectedIndex; // Modifiable index to switch selected replay buffer
    i32 recordingIndex;
    i32 playingIndex;
    HANDLE recordingHandle;
    HANDLE playingHandle;

    bool32 isReplayLooping;
};

} //namespace hm_win32

#endif // HANDMADE_WIN32
#endif // WIN32_HANDMADE_H
