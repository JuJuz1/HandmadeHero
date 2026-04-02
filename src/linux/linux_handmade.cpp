#include "handmade.h"

#include "linux_handmade.h"

#include <cstdio>

#include <SDL2/SDL.h>

// Linux-specific code

int
main() {
    printf("Hello from linux mint\n");
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Handmade Hero", "This is handmade hero!",
                             0);
    return 0;
}
