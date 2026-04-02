/*
    https://github.com/KimJorgensen/sdl_handmade/blob/master/code/sdl_handmade.cpp

    A heavily modified version of the SDL Handmade Linux platform layer using SDL2
*/

#include <SDL2/SDL.h>

#include <cstdio>
#include <sys/mman.h>

#include "handmade.h"

#include "linux_handmade.h"

// NOTE: MAP_ANONYMOUS is not defined on macOS X and some other UNIX systems
// On the vast majority of those systems, one can use MAP_ANON instead
// Huge thanks to Adam Rosenfield for investigating this, and suggesting this workaround:
#ifndef MAP_ANONYMOUS
#    define MAP_ANONYMOUS MAP_ANON
#endif

GLOBAL bool32 gIsGameRunning;

GLOBAL sdl::OffScreenBuffer gScreenBuff;

namespace sdl {

INTERNAL void
ResizeTexture(OffScreenBuffer* screenBuff, SDL_Renderer* renderer, i32 w, i32 h) {
    if (screenBuff->memory) {
        munmap(screenBuff->memory,
               (screenBuff->width * screenBuff->height) * screenBuff->bytesPerPixel);
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

NODISCARD
INTERNAL WindowDimension
GetWindowDimensions(SDL_Window* window) {
    WindowDimension result;
    SDL_GetWindowSize(window, &result.width, &result.height);

    return result;
}

INTERNAL void
DisplayBufferWindow(const OffScreenBuffer* screenBuff, SDL_Renderer* renderer, i32 wndWidth,
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

            case SDL_WINDOWEVENT_EXPOSED: {
                // Similar to WM_PAINT on Windows

                SDL_Window* window{ SDL_GetWindowFromID(event.window.windowID) };
                SDL_Renderer* renderer{ SDL_GetRenderer(window) };
                const auto wndDimension{ GetWindowDimensions(window) };
                DisplayBufferWindow(&gScreenBuff, renderer, wndDimension.width,
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

} //namespace sdl

int
main(int argc, char** argv) {
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

    gIsGameRunning = true;

    while (gIsGameRunning) {
        sdl::ProcessPendingSDLEvents();
    }

    SDL_Quit();

    return 0;
}
