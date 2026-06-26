/*
    Web platform layer
*/

#if !HANDMADE_WEB
#    error "This file should only be used when building for the web!"
#endif

#include <SDL2/SDL.h>
#include <stdio.h> // Console I/O

#include <emscripten.h>

#include "game/handmade.h"

#include "web_handmade.h"

struct GameState {
    int x, y;
};

GLOBAL SDL_Window* gWindow{};
GLOBAL SDL_Renderer* gRenderer{};

GLOBAL GameState gGameState{};
GLOBAL Input gInput{};

INTERNAL void
ProcessInputEvent(Button* button, bool32 isDown) {
    if (button->endedDown != isDown) {
        button->endedDown = isDown;
        ++button->halfTransitionCount;
    }
}

void
ProcessInput(Input* input) {
    //ProcessInput() {
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
                ProcessInputEvent(&input->playerInputs->up, isDown);
                //gInput.up = true;
                //input->up = true;
            } break;
            case SDL_SCANCODE_S: {
                ProcessInputEvent(&input->playerInputs->down, isDown);
                //gInput.down = true;
                //input->down = true;
            } break;
            case SDL_SCANCODE_A: {
                ProcessInputEvent(&input->playerInputs->left, isDown);
            } break;
            case SDL_SCANCODE_D: {
                ProcessInputEvent(&input->playerInputs->right, isDown);
            } break;

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

            case SDL_SCANCODE_R: {
                ProcessInputEvent(&input->playerInputs->R, isDown);
            } break;

            case SDL_SCANCODE_LSHIFT:
            case SDL_SCANCODE_RSHIFT: {
                ProcessInputEvent(&input->playerInputs->shift, isDown);
            } break;
            case SDL_SCANCODE_RETURN: {
                ProcessInputEvent(&input->playerInputs->enter, isDown);
            } break;

            case SDL_SCANCODE_F11: {
                if (isDown) {
                    SDL_Window* window{ SDL_GetWindowFromID(event.window.windowID) };
                    if (window) {
                        //ToggleFullscreen(window);
                    }
                }
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

            // Similar to WM_PAINT on Windows
            case SDL_WINDOWEVENT_EXPOSED: {
                printf("SDL_WINDOWEVENT_EXPOSED\n");

                //const auto wndDimension{ GetWindowdimension(window) };
                //DisplayBufferWindow(gRenderer, &gScreenBuff, wndDimension.width,
                //                    wndDimension.height);
            } break;

            default: {
            } break;
            }
        }
        }
    }
}

void
UpdateAndRender() {
    for (i32 controllerIndex{}; controllerIndex < ARRAY_COUNT(gInput.playerInputs);
         ++controllerIndex) {
        for (i32 i{}; i < ARRAY_COUNT(gInput.playerInputs[0].buttons); ++i) {
            gInput.playerInputs[controllerIndex].buttons[i].halfTransitionCount = 0;
        }
    }

    ProcessInput(&gInput);

    if (gInput.playerInputs[0].up.endedDown) {
        if (gInput.playerInputs->shift.endedDown) {
            gGameState.y -= 2;
        }
    }
    if (gInput.playerInputs[0].down.endedDown) {
        gGameState.y += 2;
    }
    if (gInput.playerInputs[0].left.endedDown) {
        gGameState.x -= 2;
    }
    if (gInput.playerInputs[0].right.endedDown) {
        gGameState.x += 2;
    }

    SDL_SetRenderDrawColor(gRenderer, 20, 20, 20, 255);
    SDL_RenderClear(gRenderer);

    SDL_Rect rect{ gGameState.x, gGameState.y, 50, 50 };

    SDL_SetRenderDrawColor(gRenderer, 255, 200, 0, 255);
    SDL_RenderFillRect(gRenderer, &rect);

    SDL_RenderPresent(gRenderer);
}

int
main() {
    SDL_Init(SDL_INIT_VIDEO);
    gWindow = SDL_CreateWindow("Handmade Web", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 960,
                               540, SDL_WINDOW_RESIZABLE);
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

    gGameState.x = 100;
    gGameState.y = 100;

    printf("Init success\n");

    emscripten_set_main_loop(UpdateAndRender, 0, true);

    return 0;
}
