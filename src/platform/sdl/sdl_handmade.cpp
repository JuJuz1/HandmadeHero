/*
    Used as a very informative reference:
    https://github.com/KimJorgensen/sdl_handmade/blob/master/code/sdl_handmade.cpp

    A heavily modified version of the SDL Handmade Linux platform layer using SDL 2

    Tested to work on:
        - Linux Mint Cinnamon 22.3 compiled with clang++ 22.1.0
        - macOS Sequoia 15.2 compiled with Apple clang 16.0
*/

#include <SDL2/SDL.h>

#include <cstdio>      // Console I/O
#include <dlfcn.h>     // DLL unload and load
#include <fcntl.h>     // File operation options
#include <sys/mman.h>  // Memory mapping
#include <sys/stat.h>  // File metadata
#include <sys/types.h> // Standard types
#include <unistd.h>    // File operations
#include <x86intrin.h> // Cpu intrinsics

#include "game/handmade.h"

#include "sdl_handmade.h"

// NOTE: MAP_ANONYMOUS is not defined on Mac and some other UNIX systems
// On the vast majority of those systems, one can use MAP_ANON instead
// Huge thanks to Adam Rosenfield for investigating this, and suggesting this workaround:
#ifndef MAP_ANONYMOUS
#    define MAP_ANONYMOUS MAP_ANON
#endif

GLOBAL bool32 gIsGameRunning;
GLOBAL bool32 gIsGamePaused;
GLOBAL bool32 gShowCursor;

GLOBAL hm_sdl::OffScreenBuffer gScreenBuff;
GLOBAL u64 gPerfCounterFreq;

#if HANDMADE_INTERNAL

namespace hm_platform_export {

INTERNAL
DEBUG_PRINT(DEBUGPrint) {
    UNUSED_PARAMS(threadContext);

    printf("%s", message);
}

INTERNAL
DEBUG_PRINT_I32(DEBUGPrintInt) {
    UNUSED_PARAMS(threadContext);

    printf("%s%d\n", valueName, value);
}

INTERNAL
DEBUG_PRINT_U32(DEBUGPrintUInt) {
    UNUSED_PARAMS(threadContext);

    printf("%s%u\n", valueName, value);
}

INTERNAL
DEBUG_PRINT_F32(DEBUGPrintFloat) {
    UNUSED_PARAMS(threadContext);

    printf("%s%f\n", valueName, value);
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
        const ssize_t bytesRead{ read(fileHandle, nextByteLocation, bytesToRead) };
        if (bytesRead == -1) {
            free(result.content);
            result.content = nullptr;
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
    if (fileHandle == -1) {
        printf("DEBUGWriteFile opening file failed\n");
        return false;
    }

    u32 bytesToWrite{ fileSize };
    u8* nextByteLocation{ static_cast<u8*>(memory) };
    while (bytesToWrite) {
        const ssize_t bytesWritten{ write(fileHandle, nextByteLocation, bytesToWrite) };
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

} //namespace hm_platform_export

#endif // HANDMADE_INTERNAL

namespace hm_sdl {

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
GetWindowdimension(SDL_Window* window) {
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
    // Needed to check?
    const ssize_t charactersRead{ readlink("/proc/self/exe", allState->exePath.data_,
                                           allState->exePath.size) };
    allState->exeFilename = allState->exePath.data_;

    for (char* scan{ allState->exePath.data_ }; *scan; ++scan) {
        if (*scan == '\\' || *scan == '/') {
            allState->exeFilename = scan + 1;
        }
    }
}

INTERNAL void
BuildGamePathFilename(const AllState* allState, const char* filename, char* dest, i32 destCount) {
    CatStrings(allState->exePath.data_, allState->exeFilename - allState->exePath.data_, filename,
               StrLength(filename), dest, destCount);
}

INTERNAL void
GetInputFilePath(const AllState* allState, bool32 inputStream, i32 slotIndex, char* dest,
                 i32 destCount) {
    Array<char, 32> temp;
    sprintf(temp.data_, "loop_edit%d_%s.hmi", slotIndex, inputStream ? "input" : "state");
    BuildGamePathFilename(allState, temp.data_, dest, destCount);
}

NODISCARD
INTERNAL ReplayBuffer*
GetReplayBuffer(AllState* allState, i32 index) {
    ASSERT(index < allState->replayBuffers.size);
    ReplayBuffer* replayBuffer{ &allState->replayBuffers[index] };
    return replayBuffer;
}

INTERNAL void
BeginRecordInput(AllState* allState, i32 recordingIndex) {
    ReplayBuffer* replayBuffer{ GetReplayBuffer(allState, recordingIndex) };
    if (replayBuffer->memoryBlock) {
        allState->recordingIndex = recordingIndex;

        Array<char, file_Name_Count> filePath;
        GetInputFilePath(allState, true, recordingIndex, filePath.data_, filePath.size);

        const i32 fileHandle{ open(filePath.data_, O_WRONLY | O_CREAT | O_TRUNC,
                                   S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) };
        if (fileHandle != -1) {
            allState->playingHandle = fileHandle;
        } else {
            printf("BeginRecordInput CreateFileA writing to file failed!\n");
        }

#if 0
        lseek(State->recordingHandle, State->TotalSize, SEEK_SET);
#endif

        memcpy(replayBuffer->memoryBlock, allState->gameMemory, allState->memorySize);
        if (!replayBuffer->isRecordedAtLeastOnce) {
            replayBuffer->isRecordedAtLeastOnce = true;
        }
    }
}

INTERNAL void
EndRecordInput(AllState* allState) {
    close(allState->recordingHandle);
    allState->recordingIndex = replay_Buffer_Not_Recording;
}

INTERNAL void
BeginInputPlayback(AllState* allState, i32 playingIndex) {
    ReplayBuffer* replayBuffer{ GetReplayBuffer(allState, playingIndex) };
    if (!replayBuffer->isRecordedAtLeastOnce) {
        printf("Can't playback buffer %d as it has not yet been recorded to!\n", playingIndex);
        return;
    }

    if (replayBuffer->memoryBlock) {
        allState->playingIndex = playingIndex;

        Array<char, file_Name_Count> filePath;
        GetInputFilePath(allState, true, playingIndex, filePath.data_, filePath.size);
        const i32 fileHandle{ open(filePath.data_, O_RDONLY) };
        if (fileHandle != -1) {
            allState->playingHandle = fileHandle;
        } else {
            printf("BeginInputPlayback CreateFileA reading file failed!\n");
        }

#if 0
        lseek(allState->playingHandle, allState->memorySize, SEEK_SET);
#endif

        memcpy(allState->gameMemory, replayBuffer->memoryBlock, allState->memorySize);
    }
}

INTERNAL void
EndInputPlayback(AllState* allState) {
    close(allState->playingHandle);

    allState->playingIndex = replay_Buffer_Not_Playing;
}

INTERNAL void
RecordInput(AllState* allState, Input* input) {
    const ssize_t bytesWritten{ write(allState->recordingHandle, input, sizeof(*input)) };
    if (bytesWritten != sizeof(*input)) {
        printf("RecordInput writing to file failed!\n");
    }
}

INTERNAL void
PlaybackInput(AllState* allState, Input* input) {
    const ssize_t bytesRead{ read(allState->playingHandle, input, sizeof(*input)) };
    if (bytesRead == 0) {
        const i32 playingIndex{ allState->playingIndex };
        EndInputPlayback(allState);

        if (allState->isReplayLooping) {
            printf("PlaybackInput starting loop again!\n");
            BeginInputPlayback(allState, playingIndex);
            read(allState->playingHandle, input, sizeof(*input));
        }
    }
}

INTERNAL void
ClearInputMemory(Input* input) {
    memset(input, 0, sizeof(*input));
}

INTERNAL void
HandleRecordButton(AllState* allState, Input* input, i32 selectedIndex) {
    ASSERT(0 <= selectedIndex && selectedIndex < allState->replayBuffers.size);
    ClearInputMemory(input);

    if (allState->playingIndex == replay_Buffer_Not_Playing) {
        if (allState->recordingIndex == replay_Buffer_Not_Recording) {
            printf("L: Recording STARTED, selected: %d!\n", selectedIndex);
            BeginRecordInput(allState, selectedIndex);
        } else {
            printf("L: Recording STOPPED, selected: %d!\n", selectedIndex);
            EndRecordInput(allState);
            BeginInputPlayback(allState, selectedIndex);
        }
    } else {
        printf("L: PLAYING STOPPED, selected: %d!\n", selectedIndex);
        EndInputPlayback(allState);
    }
}

INTERNAL void
HandleSwitchReplayBuffer(AllState* allState, Input* input, i32 selectedIndex, bool32 shiftPressed) {
    ASSERT(0 <= selectedIndex && selectedIndex < allState->replayBuffers.size);
    ClearInputMemory(input);

    if (allState->recordingIndex == selectedIndex) {
        printf("Selected same index when recording -> playback\n");
        EndRecordInput(allState);
        BeginInputPlayback(allState, selectedIndex);
        return;
    }
    if (!(allState->recordingIndex == replay_Buffer_Not_Recording)) {
        printf("Can't switch buffer while recording!\n");
        return;
    }

    allState->selectedIndex = selectedIndex;
    printf("Replay buffer %d selected\n", selectedIndex);

    EndInputPlayback(allState);

    if (shiftPressed) {
        BeginRecordInput(allState, selectedIndex);
    } else {
        const ReplayBuffer* replayBuffer{ GetReplayBuffer(allState, selectedIndex) };
        if (replayBuffer->isRecordedAtLeastOnce) {
            BeginInputPlayback(allState, allState->selectedIndex);
        }
    }
}

INTERNAL void
ProcessInputEvent(Button* button, bool32 isDown) {
    if (button->endedDown != isDown) {
        button->endedDown = isDown;
        ++button->halfTransitionCount;
    }
}

INTERNAL void
ProcessPendingEvents(Input* input, AllState* allState) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT: {
            printf("SDL_QUIT\n");
            gIsGameRunning = false;
        } break;

        // Keyboard input
        case SDL_KEYUP:
        case SDL_KEYDOWN: {
            const SDL_Scancode scancode{ event.key.keysym.scancode };
            const bool32 isDown{ event.key.state == SDL_PRESSED };

            const u8* kbState{ SDL_GetKeyboardState(0) };
            const bool32 shiftPressed{ kbState[SDL_SCANCODE_LSHIFT] ||
                                       kbState[SDL_SCANCODE_RSHIFT] };
            const bool32 altPressed{ kbState[SDL_SCANCODE_LALT] || kbState[SDL_SCANCODE_RALT] };

            // NOTE: In the windows version, we used "if (isDown != WasDown)"
            // to detect key repeats. SDL has the 'repeat' value, though,
            // which we'll use
            if (event.key.repeat != 0) {
                continue;
            }

            switch (scancode) {
            case SDL_SCANCODE_W: {
                ProcessInputEvent(&input->playerInputs->up, isDown);
            } break;
            case SDL_SCANCODE_S: {
                ProcessInputEvent(&input->playerInputs->down, isDown);
            } break;
            case SDL_SCANCODE_A: {
                ProcessInputEvent(&input->playerInputs->left, isDown);
            } break;
            case SDL_SCANCODE_D: {
                ProcessInputEvent(&input->playerInputs->right, isDown);
            } break;

                //case SDL_SCANCODE_UP: {
                //    ProcessInputEvent(&input->playerInputs[1].up, isDown);
                //} break;
                //case SDL_SCANCODE_DOWN: {
                //    ProcessInputEvent(&input->playerInputs[1].down, isDown);
                //} break;
                //case SDL_SCANCODE_LEFT: {
                //    ProcessInputEvent(&input->playerInputs[1].left, isDown);
                //} break;
                //case SDL_SCANCODE_RIGHT: {
                //    ProcessInputEvent(&input->playerInputs[1].right, isDown);
                //} break;

            case SDL_SCANCODE_UP: {
                ProcessInputEvent(&input->playerInputs->actionUp, isDown);
            } break;
            case SDL_SCANCODE_DOWN: {
                ProcessInputEvent(&input->playerInputs->actionDown, isDown);
            } break;
            case SDL_SCANCODE_LEFT: {
                ProcessInputEvent(&input->playerInputs->actionLeft, isDown);
            } break;
            case SDL_SCANCODE_RIGHT: {
                ProcessInputEvent(&input->playerInputs->actionRight, isDown);
            } break;

            case SDL_SCANCODE_SPACE: {
                ProcessInputEvent(&input->playerInputs->space, isDown);
            } break;

            case SDL_SCANCODE_Q: {
                ProcessInputEvent(&input->playerInputs->Q, isDown);
            } break;
            case SDL_SCANCODE_E: {
                ProcessInputEvent(&input->playerInputs->E, isDown);
            } break;

            case SDL_SCANCODE_LSHIFT:
            case SDL_SCANCODE_RSHIFT: {
                ProcessInputEvent(&input->playerInputs->shift, isDown);
            } break;
            case SDL_SCANCODE_RETURN: {
                ProcessInputEvent(&input->playerInputs->enter, isDown);
            } break;

            case SDL_SCANCODE_F4: {
                if (isDown) {
                    if (altPressed) {
                        gIsGameRunning = false;
                    }
                }
            } break;
            case SDL_SCANCODE_F11: {
                if (isDown) {
                    SDL_Window* window{ SDL_GetWindowFromID(event.window.windowID) };
                    if (window) {
                        ToggleFullscreen(window);
                    }
                }
            } break;

#if HANDMADE_INTERNAL
            case SDL_SCANCODE_L: {
                if (isDown) {
                    if (shiftPressed) {
                        if (allState->isReplayLooping) {
                            printf("Shift + L: Disable replay loop!\n");
                        } else {
                            printf("Shift + L: Enable replay loop!\n");
                        }

                        allState->isReplayLooping = !allState->isReplayLooping;
                    } else {
                        HandleRecordButton(allState, input, allState->selectedIndex);
                    }
                }
            } break;
            case SDL_SCANCODE_1: {
                if (isDown) {
                    HandleSwitchReplayBuffer(allState, input, 0, shiftPressed);
                }
            } break;
            case SDL_SCANCODE_2: {
                if (isDown) {
                    HandleSwitchReplayBuffer(allState, input, 1, shiftPressed);
                }
            } break;
            case SDL_SCANCODE_3: {
                if (isDown) {
                    HandleSwitchReplayBuffer(allState, input, 2, shiftPressed);
                }
            } break;
            case SDL_SCANCODE_4: {
                if (isDown) {
                    HandleSwitchReplayBuffer(allState, input, 3, shiftPressed);
                }
            } break;
            case SDL_SCANCODE_P: {
                if (isDown) {
                    if (gIsGamePaused) {
                        printf("P: Game unpaused!\n");
                    } else {
                        printf("P: Game paused!\n");
                    }

                    gIsGamePaused = !gIsGamePaused;
                }
            } break;

            case SDL_SCANCODE_Z: {
                ProcessInputEvent(&input->playerInputs->Z, isDown);
            } break;
#endif

            default: {
                if (isDown) {
                    printf("scancode %u NOT HANDLED\n", scancode);
                }
            } break;
            }
        } break;

        // Window events
        case SDL_WINDOWEVENT: {
            switch (event.window.event) {
            case SDL_WINDOWEVENT_SIZE_CHANGED: {
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
                const auto wndDimension{ GetWindowdimension(window) };
                DisplayBufferWindow(renderer, &gScreenBuff, wndDimension.width,
                                    wndDimension.height);

            } break;

            default: {
            } break;
            }
        } break;

        default: {
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
LoadGameCode(const char* srcDll, const char* lockFilename) {
    GameCode gameCode{};

    struct stat ignored;
    if (stat(lockFilename, &ignored) == 0) {
        printf("Lockfile is still present!\n");
        return gameCode;
    }

    gameCode.lastWritetime = GetLastWriteTime(srcDll);

    // Here is an extra check for the existence of lastWriteTime
    if (gameCode.lastWritetime) {
        // On Linux we can just overwrite the dll (.so) and it will work
        gameCode.dll = dlopen(srcDll, RTLD_LAZY);
        if (gameCode.dll) {
            gameCode.updateAndRender =
                reinterpret_cast<update_and_render*>(dlsym(gameCode.dll, "UpdateAndRender"));
            gameCode.getSoundSamples =
                reinterpret_cast<get_sound_samples*>(dlsym(gameCode.dll, "GetSoundSamples"));

            gameCode.isValid = gameCode.updateAndRender && gameCode.getSoundSamples;
        } else {
            printf("Failed to load dll!\n");
            //puts(dlerror());
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
        gameCode->dll = nullptr;
    }

    gameCode->isValid = false;
    gameCode->updateAndRender = nullptr;
    gameCode->getSoundSamples = nullptr;
}

} //namespace hm_sdl

int
main() {
    hm_sdl::AllState allState{};
    hm_sdl::GetExePathAndFilename(&allState);

    Array<char, hm_sdl::file_Name_Count> srcDllPath;
    // This is pretty much the only difference between Linux and macOS
    #if HANDMADE_LINUX
    hm_sdl::BuildGamePathFilename(&allState, "handmade.so", srcDllPath.data_, srcDllPath.size);
    #elif HANDMADE_MACOS
    hm_sdl::BuildGamePathFilename(&allState, "handmade.dylib", srcDllPath.data_, srcDllPath.size);
    #endif

    Array<char, hm_sdl::file_Name_Count> lockFilePath;
    hm_sdl::BuildGamePathFilename(&allState, "lock.tmp", lockFilePath.data_, lockFilePath.size);

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

    hm_sdl::ResizeTexture(&gScreenBuff, renderer, startingWidth, startingHeight);

#if HANDMADE_INTERNAL
    gShowCursor = true;
#endif

    SDL_ShowCursor(gShowCursor ? SDL_ENABLE : SDL_DISABLE);

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
    gameMemory.exports.DEBUGPrintInt = hm_platform_export::DEBUGPrintInt;
    gameMemory.exports.DEBUGPrintUInt = hm_platform_export::DEBUGPrintUInt;
    gameMemory.exports.DEBUGPrintFloat = hm_platform_export::DEBUGPrintFloat;
    gameMemory.exports.DEBUGPrint = hm_platform_export::DEBUGPrint;

    gameMemory.exports.DEBUGFreeFileMemory = hm_platform_export::DEBUGFreeFileMemory;
    gameMemory.exports.DEBUGReadFile = hm_platform_export::DEBUGReadFile;
    gameMemory.exports.DEBUGWriteFile = hm_platform_export::DEBUGWriteFile;
#endif

    allState.gameMemory = gameMemory.permanentStorage;
    allState.memorySize = totalSize;

    for (i32 i{}; i < allState.replayBuffers.size; ++i) {
        hm_sdl::ReplayBuffer* replayBuffer{ &allState.replayBuffers[i] };

        hm_sdl::GetInputFilePath(&allState, false, i, replayBuffer->replayFilePath.data_,
                                 replayBuffer->replayFilePath.size);
        const i32 fileHandle{ open(replayBuffer->replayFilePath.data_, O_RDWR | O_CREAT,
                                   S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) };
        if (fileHandle != -1) {
            replayBuffer->fileHandle = fileHandle;
        } else {
            printf("Replaybuffer mapping writing to file failed!\n");
        }

        ftruncate(replayBuffer->fileHandle, static_cast<off_t>(allState.memorySize));

        replayBuffer->memoryBlock =
            mmap(0, static_cast<size_t>(allState.memorySize), PROT_READ | PROT_WRITE, MAP_PRIVATE,
                 replayBuffer->fileHandle, 0);
        ASSERT(replayBuffer->memoryBlock);
    }

    allState.recordingIndex = hm_sdl::replay_Buffer_Not_Recording;
    allState.playingIndex = hm_sdl::replay_Buffer_Not_Playing;
    allState.isReplayLooping = true;

    gPerfCounterFreq = SDL_GetPerformanceFrequency();

    u64 lastCounter{ hm_sdl::GetWallClock() };
    u64 lastCycleCount{ _rdtsc() };

    hm_sdl::GameCode game{ hm_sdl::LoadGameCode(srcDllPath.data_, lockFilePath.data_) };
    Input gameInput{};

    gIsGameRunning = true;

    while (gIsGameRunning) {
        const time_t newDllWriteTime{ hm_sdl::GetLastWriteTime(srcDllPath.data_) };
        if (game.lastWritetime != newDllWriteTime) {
            hm_sdl::UnloadGameCode(&game);
            game = hm_sdl::LoadGameCode(srcDllPath.data_, lockFilePath.data_);
        }

        // Keyboard input

        for (i32 controllerIndex{}; controllerIndex < ARRAY_COUNT(gameInput.playerInputs);
             ++controllerIndex) {
            for (i32 i{}; i < ARRAY_COUNT(gameInput.playerInputs[0].buttons); ++i) {
                gameInput.playerInputs[controllerIndex].buttons[i].halfTransitionCount = 0;
            }
        }

        hm_sdl::ProcessPendingEvents(&gameInput, &allState);

        gameInput.playerInputs[1].enter = gameInput.playerInputs[0].enter;

        if (gIsGamePaused) {
            continue;
        }

        OffScreenBuffer screenBuff{};
        screenBuff.memory = gScreenBuff.memory;
        screenBuff.width = gScreenBuff.width;
        screenBuff.height = gScreenBuff.height;
        screenBuff.bytesPerPixel = gScreenBuff.bytesPerPixel;
        screenBuff.pitch = gScreenBuff.pitch;

        if (allState.recordingIndex != hm_sdl::replay_Buffer_Not_Recording) {
            hm_sdl::RecordInput(&allState, &gameInput);
        } else if (allState.playingIndex != hm_sdl::replay_Buffer_Not_Playing) {
            hm_sdl::PlaybackInput(&allState, &gameInput);
        }

        ThreadContext threadContext{};

        gameInput.frameDeltaTime = targetSecondsPerFrame;

        if (game.updateAndRender) {
            game.updateAndRender(&threadContext, &gameMemory, &screenBuff, &gameInput);
        }

        u64 endCounter{ hm_sdl::GetWallClock() };
        const f64 secondsElapsed{ hm_sdl::GetSecondsElapsed(lastCounter, endCounter) };
        f64 secondsElapedForFrame{ secondsElapsed };

#if 1
        if (secondsElapedForFrame < targetSecondsPerFrame) {
            // SDL doesn't have TimeBeginPeriod so we can't determine if sleep is granular
            // So we just spinlock every time
            while (secondsElapedForFrame < targetSecondsPerFrame) {
                const u32 remainingMS{ static_cast<u32>(
                    (targetSecondsPerFrame - secondsElapedForFrame) * 1000.0f) };
                if (remainingMS > 0) {
                    SDL_Delay(remainingMS);
                }

                secondsElapedForFrame =
                    hm_sdl::GetSecondsElapsed(lastCounter, hm_sdl::GetWallClock());
            }
        } else {
            // log the missed frame!!!
        }
#endif

        endCounter = hm_sdl::GetWallClock();
        const f64 ms{ 1000 * hm_sdl::GetSecondsElapsed(lastCounter, endCounter) };
        const f64 FPS{ 1000 / ms };

        const auto wndDimension{ hm_sdl::GetWindowdimension(window) };
        hm_sdl::DisplayBufferWindow(renderer, &gScreenBuff, wndDimension.width,
                                    wndDimension.height);

        const u64 endCycleCount{ _rdtsc() };
        const f64 cycleElapsedM{ static_cast<f64>((endCycleCount - lastCycleCount)) /
                                 (1000 * 1000) };

#if 0
        printf("frame: %.5f ms | FPS: %.2f | cycles: %.4f M\n", ms, FPS, cycleElapsedM);
#endif

        lastCycleCount = endCycleCount;

        const u64 resetCounter{ hm_sdl::GetWallClock() };
        lastCounter = resetCounter;
    }

    SDL_Quit();

    return 0;
}
