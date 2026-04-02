/*
    https://github.com/KimJorgensen/sdl_handmade/blob/master/code/sdl_handmade.cpp

    A heavily modified version of the SDL Handmade Linux platform layer using SDL2
*/

#include <SDL2/SDL.h>

#include <cstdio>
#include <dlfcn.h>
#include <fcntl.h>
#include <glob.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <x86intrin.h>

#include "handmade.h"

#include "linux_handmade.h"

// NOTE: MAP_ANONYMOUS is not defined on macOS X and some other UNIX systems
// On the vast majority of those systems, one can use MAP_ANON instead
// Huge thanks to Adam Rosenfield for investigating this, and suggesting this workaround:
#ifndef MAP_ANONYMOUS
#    define MAP_ANONYMOUS MAP_ANON
#endif

GLOBAL bool32 gIsGameRunning;
GLOBAL bool32 gIsGamePaused;

GLOBAL sdl::OffScreenBuffer gScreenBuff;
GLOBAL i64 gPerfCounterFreq;

#if HANDMADE_INTERNAL

namespace platform_export {

INTERNAL
DEBUG_PRINT(DEBUGPrint) {
    UNUSED_PARAMS(threadContext);

    printf("%s", message);
}

INTERNAL
DEBUG_PRINT_INT(DEBUGPrintInt) {
    UNUSED_PARAMS(threadContext);

    printf("%s: %d\n", valueName, value);
}

INTERNAL
DEBUG_PRINT_FLOAT(DEBUGPrintFloat) {
    UNUSED_PARAMS(threadContext);

    printf("%s: %f\n", valueName, value);
}

DEBUG_FREE_FILE_MEMORY(DEBUGFreeFileMemory) {
    if (memory) {
        free(memory);
    }
}

DEBUG_READ_FILE(DEBUGReadFile) {
    DEBUGFileReadResult result{};

    const i32 fileHandle{ open(filename, O_RDONLY) };
    if (fileHandle == -1) {
        return result;
    }

    struct stat fileStatus;
    if (fstat(fileHandle, &fileStatus) == -1) {
        close(fileHandle);
        return result;
    }

    result.contentSize = static_cast<u32>(fileStatus.st_size);

    result.content = malloc(result.contentSize);
    if (!result.content) {
        close(fileHandle);
        result.contentSize = 0;
        return result;
    }

    u32 bytesToRead{ result.contentSize };
    u8* nextByteLocation{ static_cast<u8*>(result.content) };
    while (bytesToRead) {
        ssize_t bytesRead{ read(fileHandle, nextByteLocation, bytesToRead) };
        if (bytesRead == -1) {
            free(result.content);
            result.content = 0;
            result.contentSize = 0;
            close(fileHandle);
            return result;
        }

        bytesToRead -= bytesRead;
        nextByteLocation += bytesRead;
    }

    close(fileHandle);

    return result;
}

DEBUG_WRITE_FILE(DEBUGWriteFile) {
    const i32 fileHandle{ open(filename, O_WRONLY | O_CREAT | O_TRUNC,
                               S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) };
    if (!fileHandle) {
        return false;
    }

    u32 bytesToWrite{ fileSize };
    u8* nextByteLocation{ static_cast<u8*>(memory) };
    while (bytesToWrite) {
        ssize_t bytesWritten{ write(fileHandle, nextByteLocation, bytesToWrite) };
        if (bytesWritten == -1) {
            close(fileHandle);
            return false;
        }

        bytesToWrite -= bytesWritten;
        nextByteLocation += bytesWritten;
    }

    close(fileHandle);

    return true;
}

} //namespace platform_export

#endif // HANDMADE_INTERNAL

namespace sdl {

INTERNAL void
ToggleFullscreen(SDL_Window* window) {
    const u32 flags{ SDL_GetWindowFlags(window) };
    if (flags & SDL_WINDOW_FULLSCREEN_DESKTOP) {
        SDL_SetWindowFullscreen(window, 0);
    } else {
        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    }
}

NODISCARD
INTERNAL WindowDimension
GetWindowDimensions(SDL_Window* window) {
    WindowDimension result;
    SDL_GetWindowSize(window, &result.width, &result.height);

    return result;
}

INTERNAL void
ResizeTexture(OffScreenBuffer* screenBuff, SDL_Renderer* renderer, i32 w, i32 h) {
    if (screenBuff->memory) {
        munmap(screenBuff->memory,
               screenBuff->width * screenBuff->height * screenBuff->bytesPerPixel);
    }

    screenBuff->width = w;
    screenBuff->height = h;
    screenBuff->bytesPerPixel = 4;

    if (screenBuff->texture) {
        SDL_DestroyTexture(screenBuff->texture);
    }

    screenBuff->texture =
        SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
                          screenBuff->width, screenBuff->height);

    const i32 bmMemorySize{ screenBuff->width * screenBuff->height * screenBuff->bytesPerPixel };
    screenBuff->memory =
        mmap(0, bmMemorySize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    screenBuff->pitch = screenBuff->width * screenBuff->bytesPerPixel;
}

INTERNAL void
DisplayBufferWindow(SDL_Renderer* renderer, const OffScreenBuffer* screenBuff, i32 wndWidth,
                    i32 wndHeight) {
    SDL_UpdateTexture(screenBuff->texture, 0, screenBuff->memory, screenBuff->pitch);

    const SDL_Rect srcRect{ 0, 0, screenBuff->width, screenBuff->height };
    SDL_Rect destRect;

    if ((wndWidth >= screenBuff->width * 2) && (wndHeight >= screenBuff->height * 2)) {
        destRect = SDL_Rect{ 0, 0, 2 * screenBuff->width, 2 * screenBuff->height };
        SDL_RenderCopy(renderer, screenBuff->texture, &srcRect, &destRect);
    } else {
        constexpr i32 offsetX{ 50 };
        constexpr i32 offsetY{ 50 };

        destRect = SDL_Rect{ offsetX, offsetY, screenBuff->width, screenBuff->height };
        SDL_RenderCopy(renderer, screenBuff->texture, &srcRect, &destRect);
    }

    SDL_RenderPresent(renderer);
}

INTERNAL void
GetExePathAndFilename(AllState* allState) {
    // TODO: Is this needed?
    memset(allState->exePath, 0, sizeof(allState->exePath));
    // Also this?
    ssize_t charactersRead{ readlink("/proc/self/exe", allState->exePath,
                                     sizeof(allState->exePath) - 1) };

    allState->exeFilename = allState->exePath;

    for (char* scan{ allState->exePath }; *scan; ++scan) {
        if (*scan == '/') {
            allState->exeFilename = scan + 1;
        }
    }
}

INTERNAL void
BuildGamePathFilename(const AllState* allState, const char* filename, char* dest, i32 destCount) {
    CatStrings(allState->exePath, allState->exeFilename - allState->exePath, filename,
               StrLength(filename), dest, destCount);
}

INTERNAL void
GetInputFilePath(const AllState* allState, bool32 inputStream, i32 slotIndex, char* dest,
                 i32 destCount) {
    char temp[32];
    sprintf(temp, "loop_edit%d_%s.hmi", slotIndex, inputStream ? "input" : "state");
    BuildGamePathFilename(allState, temp, dest, destCount);
}

NODISCARD
INTERNAL ReplayBuffer*
GetReplayBuffer(AllState* allState, i32 index) {
    ASSERT(index < ARRAY_COUNT(allState->replayBuffers));
    ReplayBuffer* replayBuffer{ &allState->replayBuffers[index] };
    return replayBuffer;
}

INTERNAL void
ProcessPendingSDLEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT: {
            printf("SDL_QUIT\n");
            gIsGameRunning = false;
        } break;

        // Window events
        case SDL_WINDOWEVENT: {
            switch (event.window.event) {
            case SDL_WINDOWEVENT_SIZE_CHANGED: {
                SDL_Window* window{ SDL_GetWindowFromID(event.window.windowID) };
                SDL_Renderer* renderer{ SDL_GetRenderer(window) };
                printf("SDL_WINDOWEVENT_SIZE_CHANGED (%d, %d)\n", event.window.data1,
                       event.window.data2);
            } break;

            case SDL_WINDOWEVENT_FOCUS_GAINED: {
                printf("SDL_WINDOWEVENT_FOCUS_GAINED\n");
            } break;

            // Similar to WM_PAINT on Windows
            case SDL_WINDOWEVENT_EXPOSED: {
                printf("SDL_WINDOWEVENT_EXPOSED\n");

                SDL_Window* window{ SDL_GetWindowFromID(event.window.windowID) };
                SDL_Renderer* renderer{ SDL_GetRenderer(window) };
                const auto wndDimension{ GetWindowDimensions(window) };
                DisplayBufferWindow(renderer, &gScreenBuff, wndDimension.width,
                                    wndDimension.height);

            } break;
            }
        } break;

        default: {
            //printf("Default event\n");
        } break;
        }
    }
}

NODISCARD
INTERNAL u64
GetWallClock() {
    const u64 result{ SDL_GetPerformanceCounter() };
    return result;
}

NODISCARD
INTERNAL f32
GetSecondsElapsed(u64 Start, u64 End) {
    const f32 result{ static_cast<f32>(End - Start) / static_cast<f32>(gPerfCounterFreq) };
    return result;
}

NODISCARD
INTERNAL time_t
GetLastWriteTime(const char* filename) {
    time_t lastWriteTime{};

    struct stat fileStatus;
    if (stat(filename, &fileStatus) == 0) {
        lastWriteTime = fileStatus.st_mtime;
    }

    return lastWriteTime;
}

INTERNAL GameCode
LoadGameCode(const char* srcDll, const char* tempDll, const char* lockFilename) {
    GameCode gameCode{};

    // TODO: Check if .tmp file still exists

    gameCode.lastWritetime = GetLastWriteTime(srcDll);

    // Here is an extra check for the existence of lastWriteTime
    if (gameCode.lastWritetime) {
        gameCode.dll = dlopen(srcDll, RTLD_LAZY);
        if (gameCode.dll) {
            gameCode.updateAndRender =
                reinterpret_cast<update_and_render*>(dlsym(gameCode.dll, "UpdateAndRender"));
            gameCode.getSoundSamples =
                reinterpret_cast<get_sound_samples*>(dlsym(gameCode.dll, "GetSoundSamples"));

            gameCode.isValid = gameCode.updateAndRender && gameCode.getSoundSamples;
        } else {
            printf("Failed to load dll!\n");
            puts(dlerror());
        }
    } else {
        printf("lastWriteTime was invalid\n");
    }

    if (!gameCode.isValid) {
        printf("gameCode is invalid, game functions are null!\n");
    }

    return gameCode;
}

INTERNAL void
UnloadGameCode(GameCode* gameCode) {
    if (gameCode->dll) {
        dlclose(gameCode->dll);
        gameCode->dll = 0;
    }

    gameCode->isValid = false;
    gameCode->updateAndRender = 0;
    gameCode->getSoundSamples = 0;
}

} //namespace sdl

int
main() {
    sdl::AllState allState{};
    // Exe paths and stuff
    sdl::GetExePathAndFilename(&allState);

    // NOTE: a little string processing cause we are handmade
    char srcDllPath[sdl::all_State_File_Name_Count];
    sdl::BuildGamePathFilename(&allState, "handmade.so", srcDllPath, sizeof(srcDllPath));
    char tempDllPath[sdl::all_State_File_Name_Count];
    sdl::BuildGamePathFilename(&allState, "handmade_temp.so", tempDllPath, sizeof(tempDllPath));

    char lockFilePath[sdl::all_State_File_Name_Count];
    sdl::BuildGamePathFilename(&allState, "lock.tmp", lockFilePath, sizeof(lockFilePath));

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC | SDL_INIT_AUDIO);

    constexpr i32 startingWidth{ 960 };
    constexpr i32 startingHeight{ 540 };

    SDL_Window* window{ SDL_CreateWindow("Handmade Hero", SDL_WINDOWPOS_UNDEFINED,
                                         SDL_WINDOWPOS_UNDEFINED, startingWidth, startingHeight,
                                         SDL_WINDOW_RESIZABLE) };
    if (!window) {
        printf("Couldn't create window!\n");
        return 0;
    }

    SDL_Renderer* renderer{ SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC) };
    if (!renderer) {
        printf("Couldn't create renderer!\n");
        return 0;
    }

    sdl::ResizeTexture(&gScreenBuff, renderer, startingWidth, startingHeight);

    i32 monitorHz{ 60 };
    const i32 displayIndex{ SDL_GetWindowDisplayIndex(window) };
    SDL_DisplayMode displayMode{};
    const i32 displayModeresult{ SDL_GetDesktopDisplayMode(displayIndex, &displayMode) };
    if (displayModeresult == 0 && displayMode.refresh_rate > 1) {
        monitorHz = displayMode.refresh_rate;
        printf("Detected valid monitorHz: %d\n", monitorHz);
        //monitorHz = 60;
    } else {
        printf("Using default monitorHz: %d\n", monitorHz);
    }

    const f32 gameUpdateHz{ static_cast<f32>(monitorHz) / 2.0f };
    const f32 targetSecondsPerFrame{ 1.0f / gameUpdateHz };

#if HANDMADE_INTERNAL
    void* baseAddress{ reinterpret_cast<void*>(TERABYTES(2)) };
#else
    void* baseAddress{};
#endif

    GameMemory gameMemory{};
    gameMemory.permanentStorageSize = MEGABYTES(64);
    gameMemory.transientStorageSize = MEGABYTES(128);

    const u64 totalSize{ gameMemory.permanentStorageSize + gameMemory.transientStorageSize };
    gameMemory.permanentStorage =
        mmap(baseAddress, totalSize, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

    gameMemory.transientStorage =
        static_cast<u8*>(gameMemory.permanentStorage) + gameMemory.permanentStorageSize;
    if (!( //soundBuffSamples&&
            gameMemory.permanentStorage && gameMemory.transientStorage)) {
        ASSERT(!"One or more of the game memory allocations failed!");
    }

#if HANDMADE_INTERNAL
    // Platform exports
    gameMemory.exports.DEBUGFreeFileMemory = platform_export::DEBUGFreeFileMemory;
    gameMemory.exports.DEBUGReadFile = platform_export::DEBUGReadFile;
    gameMemory.exports.DEBUGWriteFile = platform_export::DEBUGWriteFile;
    gameMemory.exports.DEBUGPrintInt = platform_export::DEBUGPrintInt;
    gameMemory.exports.DEBUGPrintFloat = platform_export::DEBUGPrintFloat;
    gameMemory.exports.DEBUGPrint = platform_export::DEBUGPrint;
#endif

    allState.gameMemory = gameMemory.permanentStorage;
    allState.memorySize = totalSize;

    //for (i32 i{}; i < ARRAY_COUNT(allState.replayBuffers); ++i) {
    //    sdl::ReplayBuffer* replayBuffer{ &allState.replayBuffers[i] };

    //    sdl::GetInputFilePath(&allState, false, i, replayBuffer->replayFilePath,
    //                          sizeof(replayBuffer->replayFilePath));

    //    replayBuffer->fileHandle = open(replayBuffer->replayFilePath, O_RDWR | O_CREAT,
    //                                    S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

    //    ftruncate(replayBuffer->fileHandle, allState.memorySize);

    //    replayBuffer->memoryBlock = mmap(0, allState.memorySize, PROT_READ | PROT_WRITE,
    //                                     MAP_PRIVATE, replayBuffer->fileHandle, 0);
    //    ASSERT(replayBuffer->memoryBlock);
    //}

    //allState.recordingIndex = sdl::replay_Buffer_Not_Recording;
    //allState.playingIndex = sdl::replay_Buffer_Not_Playing;
    //allState.isReplayLooping = true;

    // TODO: other performance statistics
    gPerfCounterFreq = SDL_GetPerformanceFrequency();

    sdl::GameCode game{ sdl::LoadGameCode(srcDllPath, tempDllPath, lockFilePath) };
    Input gameInput{};

    gIsGameRunning = true;

    while (gIsGameRunning) {
        const time_t newDllWriteTime{ sdl::GetLastWriteTime(srcDllPath) };
        if (game.lastWritetime != newDllWriteTime) {
            sdl::UnloadGameCode(&game);
            game = sdl::LoadGameCode(srcDllPath, tempDllPath, lockFilePath);
        }

        // Keyboard input

        for (i32 controllerIndex{}; controllerIndex < ARRAY_COUNT(gameInput.playerInputs);
             ++controllerIndex) {
            for (i32 i{}; i < ARRAY_COUNT(gameInput.playerInputs[0].buttons); ++i) {
                gameInput.playerInputs[controllerIndex].buttons[i].halfTransitionCount = 0;
            }
        }

        sdl::ProcessPendingSDLEvents();

        OffScreenBuffer screenBuff{};
        screenBuff.memory = gScreenBuff.memory;
        screenBuff.width = gScreenBuff.width;
        screenBuff.height = gScreenBuff.height;
        screenBuff.bytesPerPixel = gScreenBuff.bytesPerPixel;
        screenBuff.pitch = gScreenBuff.pitch;

        ThreadContext threadContext{};

        if (game.updateAndRender) {
            game.updateAndRender(&threadContext, &gameMemory, &screenBuff, &gameInput);
        }

        const auto wndDimension{ sdl::GetWindowDimensions(window) };
        sdl::DisplayBufferWindow(renderer, &gScreenBuff, wndDimension.width, wndDimension.height);
    }

    SDL_Quit();

    return 0;
}
