/*
    Web platform layer
    TODO: this file shares a lot of code identical in sdl_handmade.cpp
    Consider pulling shared SDL code to a file and include to Linux and Mac platform layers also!

    For example:
        - input handling
        - displaying the buffer in the window (rendering)

    Known problems:
        - Resetting the game fully (F5) many times in a row results in a crash due to:
        Aborted(Cannot enlarge memory arrays to size 134381568 bytes (OOM). Either (1) compile with
        -sINITIAL_MEMORY=X with X higher than the current value 134217728, (2) compile with
        -sALLOW_MEMORY_GROWTH which allows increasing the size at runtime, or (3) if you want malloc
        to return NULL (0) instead of this abort, compile with -sABORTING_MALLOC=0)

        It's due to loading the art assets again, shouldn't probably do that anyways but we do free
        the memory malloced for the art asset files so this should not happen
*/

#if !HANDMADE_WEB
#    error "This file should only be used when building for the web!"
#endif

#include <SDL2/SDL.h>

#include <fcntl.h>    // File operation options
#include <malloc.h>   // Malloc
#include <stdio.h>    // Console I/O
#include <sys/stat.h> // File metadata
#include <unistd.h>   // File operations

#include <emscripten.h>

#include "game/handmade.h"
#include "game/handmade_input.h"

#include "web_handmade.h"

// Game! no DLL here
#include "game/handmade.cpp"

static_assert(sizeof(void*) == 4, "We are targeting 32-bit for now");

GLOBAL SDL_Window* gWindow;
GLOBAL SDL_Renderer* gRenderer;
GLOBAL hm_web::OffScreenBuffer gScreenBuff;

GLOBAL GameState gGameState;
GLOBAL Input gInput;
GLOBAL GameMemory gGameMemory;

GLOBAL bool32 gIsGamePaused;

GLOBAL u64 gLastCounter;
GLOBAL u64 gPerfCounterFreq;

GLOBAL constexpr i32 startingWidth{ 960 };
GLOBAL constexpr i32 startingHeight{ 540 };

namespace hm_platform_export {

#if HANDMADE_INTERNAL

INTERNAL
DEBUG_PRINT(DEBUGPrint) {
    UNUSED_PARAMS(threadContext);

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

#else

INTERNAL
DEBUG_PRINT(DEBUGPrint) {}

#endif // HANDMADE_INTERNAL

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

// TODO: unused
DEBUG_WRITE_FILE(DEBUGWriteFile) { return false; }

} //namespace hm_platform_export

namespace hm_web {

// TODO: Expose to be called from Javascript, if we want to use the button instead of F11
//extern "C"
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
        free(screenBuff->memory);
    }

    if (screenBuff->texture) {
        SDL_DestroyTexture(screenBuff->texture);
    }

    screenBuff->width = w;
    screenBuff->height = h;
    screenBuff->bytesPerPixel = 4;

    screenBuff->texture =
        SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
                          screenBuff->width, screenBuff->height);

    screenBuff->memory = malloc(screenBuff->width * screenBuff->height * screenBuff->bytesPerPixel);
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
    } else {
        constexpr i32 offsetX{ 50 };
        constexpr i32 offsetY{ 50 };

        destRect = SDL_Rect{ offsetX, offsetY, screenBuff->width, screenBuff->height };
    }

    SDL_RenderCopyEx(renderer, screenBuff->texture, &srcRect, &destRect, 0, nullptr,
                     SDL_FLIP_VERTICAL);

    SDL_RenderPresent(renderer);
}

INTERNAL void
ProcessInputEvent(Button* button, bool32 isDown) {
    if (button->endedDown != isDown) {
        button->endedDown = isDown;
        ++button->halfTransitionCount;
    }
}

INTERNAL void
ProcessPendingEvents(Input* input) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT: {
            printf("SDL_QUIT\n");
            emscripten_cancel_main_loop();
        } break;

        case SDL_KEYUP:
        case SDL_KEYDOWN: {
            const SDL_Scancode scancode{ event.key.keysym.scancode };
            const bool32 isDown{ event.key.state == SDL_PRESSED };

            const u8* kbState{ SDL_GetKeyboardState(0) };
            const bool32 shiftPressed{ kbState[SDL_SCANCODE_LSHIFT] ||
                                       kbState[SDL_SCANCODE_RSHIFT] };
            const bool32 altPressed{ kbState[SDL_SCANCODE_LALT] || kbState[SDL_SCANCODE_RALT] };

            if (event.key.repeat != 0) {
                continue;
            }

            switch (scancode) {
            case SDL_SCANCODE_W: {
                hm_input::ProcessInputEvent(&input->playerInputs->up, isDown);
            } break;
            case SDL_SCANCODE_S: {
                hm_input::ProcessInputEvent(&input->playerInputs->down, isDown);
            } break;
            case SDL_SCANCODE_A: {
                hm_input::ProcessInputEvent(&input->playerInputs->left, isDown);
            } break;
            case SDL_SCANCODE_D: {
                hm_input::ProcessInputEvent(&input->playerInputs->right, isDown);
            } break;

                //case SDL_SCANCODE_UP: {
                //    hm_input::ProcessInputEvent(&input->playerInputs[1].up, isDown);
                //} break;
                //case SDL_SCANCODE_DOWN: {
                //    hm_input::ProcessInputEvent(&input->playerInputs[1].down, isDown);
                //} break;
                //case SDL_SCANCODE_LEFT: {
                //    hm_input::ProcessInputEvent(&input->playerInputs[1].left, isDown);
                //} break;
                //case SDL_SCANCODE_RIGHT: {
                //    hm_input::ProcessInputEvent(&input->playerInputs[1].right, isDown);
                //} break;

            case SDL_SCANCODE_UP: {
                hm_input::ProcessInputEvent(&input->playerInputs->actionUp, isDown);
            } break;
            case SDL_SCANCODE_DOWN: {
                hm_input::ProcessInputEvent(&input->playerInputs->actionDown, isDown);
            } break;
            case SDL_SCANCODE_LEFT: {
                hm_input::ProcessInputEvent(&input->playerInputs->actionLeft, isDown);
            } break;
            case SDL_SCANCODE_RIGHT: {
                hm_input::ProcessInputEvent(&input->playerInputs->actionRight, isDown);
            } break;

            case SDL_SCANCODE_SPACE: {
                hm_input::ProcessInputEvent(&input->playerInputs->space, isDown);
            } break;

            case SDL_SCANCODE_Q: {
                hm_input::ProcessInputEvent(&input->playerInputs->Q, isDown);
            } break;
            case SDL_SCANCODE_E: {
                hm_input::ProcessInputEvent(&input->playerInputs->E, isDown);
            } break;

            case SDL_SCANCODE_R: {
                hm_input::ProcessInputEvent(&input->playerInputs->R, isDown);
            } break;
            case SDL_SCANCODE_F: {
                hm_input::ProcessInputEvent(&input->playerInputs->F, isDown);
            } break;

            case SDL_SCANCODE_LSHIFT:
            case SDL_SCANCODE_RSHIFT: {
                hm_input::ProcessInputEvent(&input->playerInputs->shift, isDown);
            } break;
            case SDL_SCANCODE_LCTRL:
            case SDL_SCANCODE_RCTRL: {
                hm_input::ProcessInputEvent(&input->playerInputs->ctrl, isDown);
            } break;
            case SDL_SCANCODE_RETURN: {
                hm_input::ProcessInputEvent(&input->playerInputs->enter, isDown);
            } break;

            case SDL_SCANCODE_F11: {
                if (isDown) {
                    ToggleFullscreen(gWindow);
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

            case SDL_SCANCODE_F1: {
                hm_input::ProcessInputEvent(&input->playerInputs->F1, isDown);
            } break;
            case SDL_SCANCODE_F2: {
                hm_input::ProcessInputEvent(&input->playerInputs->F2, isDown);
            } break;
            case SDL_SCANCODE_F3: {
                hm_input::ProcessInputEvent(&input->playerInputs->F3, isDown);
            } break;
            case SDL_SCANCODE_F5: {
                hm_input::ProcessInputEvent(&input->playerInputs->F5, isDown);
            } break;
            case SDL_SCANCODE_F6: {
                hm_input::ProcessInputEvent(&input->playerInputs->F6, isDown);
            } break;
            case SDL_SCANCODE_F7: {
                hm_input::ProcessInputEvent(&input->playerInputs->F7, isDown);
            } break;
            case SDL_SCANCODE_F8: {
                hm_input::ProcessInputEvent(&input->playerInputs->F8, isDown);
            } break;
            case SDL_SCANCODE_F9: {
                hm_input::ProcessInputEvent(&input->playerInputs->F9, isDown);
            } break;
            case SDL_SCANCODE_F10: {
                hm_input::ProcessInputEvent(&input->playerInputs->F10, isDown);
            } break;

            default: {
                //if (isDown) {
                printf("scancode %u, keycode: %u NOT HANDLED\n", scancode, event.key.keysym.sym);
                //}
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

            case SDL_WINDOWEVENT_EXPOSED: {
                printf("SDL_WINDOWEVENT_EXPOSED\n");

                const auto wndDimension{ GetWindowdimension(gWindow) };
                DisplayBufferWindow(gRenderer, &gScreenBuff, wndDimension.width,
                                    wndDimension.height);
            } break;

            default: {
            } break;
            }
        }
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
INTERNAL f64
GetSecondsElapsed(u64 Start, u64 End) {
    const f64 result{ static_cast<f64>(End - Start) / static_cast<f64>(gPerfCounterFreq) };
    return result;
}

} //namespace hm_web

INTERNAL void
WebMainLoop() {
    const u64 endCounter{ hm_web::GetWallClock() };
    f64 secondsElapsed{ hm_web::GetSecondsElapsed(gLastCounter, endCounter) };
    gLastCounter = endCounter;

    // 15 fps -> 66,66... ms
    constexpr f64 maxDelta{ 1 / 15.0f };
    if (secondsElapsed > maxDelta) {
        secondsElapsed = maxDelta;
    }

    gInput.frameDeltaTime = secondsElapsed;
    //printf("dt %.4f (%.2f ms)\n", secondsElapsed, secondsElapsed * 1000.0);

    for (i32 controllerIndex{}; controllerIndex < ARRAY_COUNT(gInput.playerInputs);
         ++controllerIndex) {
        for (i32 i{}; i < ARRAY_COUNT(gInput.playerInputs[0].buttons); ++i) {
            gInput.playerInputs[controllerIndex].buttons[i].halfTransitionCount = 0;
        }
    }

    hm_web::ProcessPendingEvents(&gInput);

    if (!gIsGamePaused) {
        ThreadContext threadContext{};

        OffScreenBuffer screenBuff{};
        screenBuff.memory = gScreenBuff.memory;
        screenBuff.width = gScreenBuff.width;
        screenBuff.height = gScreenBuff.height;
        screenBuff.pitch = gScreenBuff.pitch;

        // Game call
        UpdateAndRender(&threadContext, &gGameMemory, &screenBuff, &gInput);

        // TODO: audio

        const auto wndDimension{ hm_web::GetWindowdimension(gWindow) };
        hm_web::DisplayBufferWindow(gRenderer, &gScreenBuff, wndDimension.width,
                                    wndDimension.height);
    }
}

int
main() {
    SDL_Init(SDL_INIT_VIDEO);
    gWindow = SDL_CreateWindow("Handmade Web", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                               startingWidth, startingHeight, SDL_WINDOW_RESIZABLE);
    if (!gWindow) {
        printf("Couldn't create window!\n");
        return 0;
    }

    gRenderer =
        SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!gRenderer) {
        printf("Couldn't create renderer!\n");
        return 0;
    }

    hm_web::ResizeTexture(&gScreenBuff, gRenderer, startingWidth, startingHeight);

    // TODO: take a look at increasing max size, currently seems to be 16 MB
    // Increased to 32 MB by compile option
    gGameMemory.permanentStorageSize = MEGABYTES(32);
    gGameMemory.transientStorageSize = MEGABYTES(64);

    const u32 totalSize{ gGameMemory.permanentStorageSize + gGameMemory.transientStorageSize };
    gGameMemory.permanentStorage = malloc(totalSize);

    gGameMemory.transientStorage =
        static_cast<u8*>(gGameMemory.permanentStorage) + gGameMemory.permanentStorageSize;
    if (!(gGameMemory.permanentStorage && gGameMemory.transientStorage)) {
        ASSERT(!"One or more of the game memory allocations failed!");
    }

    // Platform exports
    gGameMemory.exports.DEBUGPrint = hm_platform_export::DEBUGPrint;

    gGameMemory.exports.DEBUGFreeFileMemory = hm_platform_export::DEBUGFreeFileMemory;
    gGameMemory.exports.DEBUGReadFile = hm_platform_export::DEBUGReadFile;
    gGameMemory.exports.DEBUGWriteFile = hm_platform_export::DEBUGWriteFile;

    gPerfCounterFreq = SDL_GetPerformanceFrequency();
    printf("PerfCounterFreq: %llu\n", gPerfCounterFreq);

    // This code is not relevant to the web it seems
    // We can't control the frame pacing so have to manually calculate delta time

    //i32 monitorHz{ 60 };
    //const i32 displayIndex{ SDL_GetWindowDisplayIndex(gWindow) };
    //SDL_DisplayMode displayMode{};
    //const i32 displayModeresult{ SDL_GetDesktopDisplayMode(displayIndex, &displayMode) };
    //if (displayModeresult == 0 && displayMode.refresh_rate > 1) {
    //    monitorHz = displayMode.refresh_rate;
    //    printf("Detected valid monitorHz: %d\n", monitorHz);
    //    //monitorHz = 60;
    //} else {
    //    printf("Using default monitorHz: %d\n", monitorHz);
    //}

    //const f32 gameUpdateHz{ static_cast<f32>(monitorHz) / 2.0f };
    //const f32 targetSecondsPerFrame{ 1.0f / gameUpdateHz };

    gLastCounter = hm_web::GetWallClock();

    emscripten_set_main_loop(WebMainLoop, 0, true);

    return 0;
}
