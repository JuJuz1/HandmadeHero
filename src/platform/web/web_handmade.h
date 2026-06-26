#ifndef WEB_HANDMADE_H
#define WEB_HANDMADE_H

#if HANDMADE_WEB

namespace hm_web {

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

} //namespace hm_web

#endif // HANDMADE_WEB
#endif // WEB_HANDMADE_H
