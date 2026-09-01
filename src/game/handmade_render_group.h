#ifndef HANDMADE_RENDER_GROUP_H
#define HANDMADE_RENDER_GROUP_H

struct LoadedBitmapInfo {
    void* memory;
    i32 width;
    i32 height;
    i32 pitch;

    Vec2 alignPercentage;
    f32 widthOverHeight;
};

struct EnvironmentMap {
    Array<LoadedBitmapInfo, 4> lod;
    f32 zPos;
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
    RenderGroupEntryType_RenderEntrySaturation,
};

// Callers don't have to know about this now
struct RenderGroupEntryHeader {
    RenderGroupEntryType type;
};

struct RenderEntityBasis {
    RenderBasis* basis;
    Vec3 offset;
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
    Vec2 size;
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

struct RenderEntrySaturation {
    f32 saturation;
};

struct RenderGroupCamera {
    // Modifiable camera properties, needs tuning
    f32 focalLength; // How far the person is sitting from the monitor in meters
    f32 cameraDistAboveTarget;
};

struct RenderGroup {
    RenderGroupCamera gameCamera;   // Where the game will think the camera is at
    RenderGroupCamera renderCamera; // Where we actually render from

    f32 metersToPixels; // Meters on the monitor to pixels on the monitor
    Vec2 monitorHalfDimInMeters;

    RenderBasis* defaultBasis;
    f32 globalAlpha;

    u8* pushBufferBase;
    i32 pushBufferSize;
    i32 maxPushBufferSize;
};

#endif // HANDMADE_RENDER_GROUP_H
