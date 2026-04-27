#ifndef SDL_HANDMADE_H
#define SDL_HANDMADE_H

#if defined(HANDMADE_LINUX) || defined(HANDMADE_MACOS)

#    include "game/handmade_array.h"

namespace hm_sdl {

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

// IMPORTANT: Take care of stack size limit
GLOBAL constexpr i32 file_Name_Count{ 1024 };

struct ReplayBuffer {
    i32 fileHandle;
    void* memoryBlock;
    Array<char, file_Name_Count> replayFilePath;

    bool32 isRecordedAtLeastOnce;
};

GLOBAL constexpr i32 replay_Buffer_Not_Recording{ -1 };
GLOBAL constexpr i32 replay_Buffer_Not_Playing{ -1 };

struct AllState {
    void* gameMemory;
    u64 memorySize;
    Array<ReplayBuffer, 4> replayBuffers;

    Array<char, file_Name_Count> exePath;
    char* exeFilename;

    i32 selectedIndex;
    i32 recordingIndex;
    i32 playingIndex;

    i32 recordingHandle;
    i32 playingHandle;

    bool32 isReplayLooping;
};

} //namespace hm_sdl

#endif // HANDMADE_LINUX || HANDMADE_MACOS
#endif // SDL_HANDMADE_H
