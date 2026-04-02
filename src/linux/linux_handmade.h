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

} //namespace sdl

#endif // HANDMADE_LINUX
#endif // LINUX_HANDMADE_H
