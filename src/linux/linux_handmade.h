#ifndef LINUX_HANDMADE_H
#define LINUX_HANDMADE_H

#if HANDMADE_LINUX

namespace sdl {

struct OffScreenBuffer {
    SDL_Texture* texture;
    void* memory;
    i32 width;
    i32 height;
    i32 bytesPerPixel;
    i32 pitch;
};

struct WindowDimension {
    i32 width;
    i32 height;
};

struct GameCode {
    void* dll;
    time_t lastWritetime;

    update_and_render* updateAndRender;
    get_sound_samples* getSoundSamples;

    bool32 isValid;
};

GLOBAL i32 constexpr all_State_File_Name_Count{ 4096 };

struct ReplayBuffer {
    i32 fileHandle;
    void* memoryMap;
    void* memoryBlock;
    char replayFilePath[all_State_File_Name_Count];

    bool32 isRecordedAtLeastOnce;
};

GLOBAL i32 constexpr replay_Buffer_Count{ 4 };
GLOBAL i32 constexpr replay_Buffer_Not_Recording{ -1 };
GLOBAL i32 constexpr replay_Buffer_Not_Playing{ -1 };

struct AllState {
    void* gameMemory;
    u64 memorySize;
    ReplayBuffer replayBuffers[replay_Buffer_Count];

    char exePath[all_State_File_Name_Count];
    char* exeFilename;

    i32 selectedIndex;
    i32 recordingIndex;
    i32 playingIndex;

    i32 recordingHandle;
    i32 playingHandle;

    bool32 isReplayLooping;
};

} //namespace sdl

#endif // HANDMADE_LINUX
#endif // LINUX_HANDMADE_H
