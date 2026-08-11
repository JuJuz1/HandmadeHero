#ifndef HANDMADE_RENDER_GROUP_H
#define HANDMADE_RENDER_GROUP_H

struct RenderBasis {
    Vec3 pos;
};

enum RenderGroupEntryType {
    RenderGroupEntryType_RenderEntryClear = 0,
    RenderGroupEntryType_RenderEntryRect,
    RenderGroupEntryType_RenderEntryBitmap,
    RenderGroupEntryType_RenderEntryCoordinateSystem,
};

struct RenderGroupEntryHeader {
    RenderGroupEntryType type;
};

struct RenderEntityBasis {
    RenderBasis* basis;
    Vec2 offset;
    f32 offsetZ;
    f32 entityZC;
};

// IMPORTANT: Each entry should contain the header
struct RenderEntryClear {
    RenderGroupEntryHeader header;
    Vec4 color;
};

struct RenderEntryRect {
    RenderGroupEntryHeader header;

    RenderEntityBasis entityBasis;
    Vec4 color;
    Vec2 dim;
};

struct RenderEntryBitmap {
    RenderGroupEntryHeader header;

    RenderEntityBasis entityBasis;
    LoadedBitmapInfo* bitmap;
    Vec4 color;
};

struct RenderEntryCoordinateSystem {
    RenderGroupEntryHeader header;

    Vec2 origin;
    Vec2 xAxis;
    Vec2 yAxis;
    Vec4 color;
    LoadedBitmapInfo* texture;

    Array<Vec2, 16> points;
};

struct RenderGroup {
    RenderBasis* defaultBasis;
    f32 metersToPixels;

    u8* pushBufferBase;
    i32 pushBufferSize;
    i32 maxPushBufferSize;
};

#endif // HANDMADE_RENDER_GROUP_H
