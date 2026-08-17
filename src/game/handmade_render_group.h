#ifndef HANDMADE_RENDER_GROUP_H
#define HANDMADE_RENDER_GROUP_H

struct LoadedBitmapInfo {
    void* memory;
    i32 width;
    i32 height;
    i32 pitch;
};

struct EnvironmentMap {
    Array<LoadedBitmapInfo, 4> lod;
};

struct RenderBasis {
    Vec3 pos;
};

// See the .cpp PushRenderElement macro for an explanation for the naming
enum RenderGroupEntryType {
    RenderGroupEntryType_RenderEntryClear = 0,
    RenderGroupEntryType_RenderEntryRect,
    RenderGroupEntryType_RenderEntryBitmap,
    RenderGroupEntryType_RenderEntryCoordinateSystem,
};

// Callers don't have to know about this now
struct RenderGroupEntryHeader {
    RenderGroupEntryType type;
};

struct RenderEntityBasis {
    RenderBasis* basis;
    Vec2 offset;
    f32 offsetZ;
    f32 entityZC;
};

struct RenderEntryClear {
    Vec4 color;
};

struct RenderEntryRect {
    RenderEntityBasis entityBasis;
    Vec4 color;
    Vec2 dim;
};

struct RenderEntryBitmap {
    RenderEntityBasis entityBasis;
    LoadedBitmapInfo* bitmap;
    Vec4 color;
};

struct RenderEntryCoordinateSystem {
    Vec2 origin;
    Vec2 xAxis;
    Vec2 yAxis;
    Vec4 color;
    LoadedBitmapInfo* texture;
    LoadedBitmapInfo* normalMap;

    EnvironmentMap* top;
    EnvironmentMap* middle;
    EnvironmentMap* bottom;

    //Array<Vec2, 16> points;
};

struct RenderGroup {
    RenderBasis* defaultBasis;
    f32 metersToPixels;

    u8* pushBufferBase;
    i32 pushBufferSize;
    i32 maxPushBufferSize;
};

#endif // HANDMADE_RENDER_GROUP_H
