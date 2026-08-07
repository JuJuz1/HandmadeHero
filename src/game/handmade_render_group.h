#ifndef HANDMADE_RENDER_GROUP_H
#define HANDMADE_RENDER_GROUP_H

struct RenderBasis {
    Vec3 pos;
};

struct EntityVisiblePiece {
    RenderBasis* basis;
    LoadedBitmapInfo* bitmap;
    Vec2 offset;
    f32 offsetZ;
    f32 entityZC;

    f32 r, g, b, a;
    Vec2 dimension;
};

// TODO: this should just be a part of the renderer...
struct RenderGroup {
    RenderBasis* defaultBasis;
    f32 metersToPixels;

    i32 pieceCount;

    u8* pushBufferBase;
    i32 pushBufferSize;
    i32 maxPushBufferSize;
};

INTERNAL void*
PushRenderElement(RenderGroup* group, i32 size) {
    void* result{};

    if ((group->pushBufferSize + size) < group->maxPushBufferSize) {
        result = group->pushBufferBase + group->pushBufferSize;
        group->pushBufferSize += size;
    } else {
        INVALID_CODE_PATH;
    }

    return result;
}

INTERNAL void
PushPiece(RenderGroup* group, LoadedBitmapInfo* bitmap, Vec2 offset, f32 offsetZ, Vec2 align,
          Vec2 dimension, Vec4 color, f32 entityZC = 1.0f) {
    //ASSERT(group->pieceCount < group->pieces.size);
    //EntityVisiblePiece* piece{ &group->pieces[group->pieceCount++] };
    EntityVisiblePiece* piece{ static_cast<EntityVisiblePiece*>(
        PushRenderElement(group, sizeof(EntityVisiblePiece))) };

    piece->basis = group->defaultBasis;
    piece->bitmap = bitmap;
    piece->offset = (group->metersToPixels * Vec2{ offset.x, -offset.y }) - align;
    piece->offsetZ = offsetZ;
    piece->entityZC = entityZC;

    piece->dimension = dimension;

    piece->r = color.r;
    piece->g = color.g;
    piece->b = color.b;
    piece->a = color.a;
}

INTERNAL void
PushBitmap(RenderGroup* group, LoadedBitmapInfo* bitmap, Vec2 offset, f32 offsetZ, Vec2 align,
           f32 alpha = 1.0f, f32 entityZC = 1.0f) {
    PushPiece(group, bitmap, offset, offsetZ, align, Vec2{}, Vec4{ 1.0f, 1.0f, 1.0f, alpha },
              entityZC);
}

INTERNAL void
PushRect(RenderGroup* group, Vec2 offset, f32 offsetZ, Vec2 dim, Vec4 color, f32 entityZC = 1.0f) {
    PushPiece(group, nullptr, offset, offsetZ, Vec2{}, dim, color, entityZC);
}

INTERNAL void
PushRectOutline(RenderGroup* group, Vec2 offset, f32 offsetZ, Vec2 dim, Vec4 color,
                f32 entityZC = 1.0f) {
    const f32 thickness{ 0.1f };

    // Top bottom
    PushPiece(group, 0, offset - Vec2{ 0, dim.y * 0.5f }, offsetZ, Vec2{}, Vec2{ dim.x, thickness },
              color, entityZC);
    PushPiece(group, 0, offset + Vec2{ 0, dim.y * 0.5f }, offsetZ, Vec2{}, Vec2{ dim.x, thickness },
              color, entityZC);

    // Left right
    PushPiece(group, 0, offset - Vec2{ dim.x * 0.5f, 0 }, offsetZ, Vec2{}, Vec2{ thickness, dim.y },
              color, entityZC);
    PushPiece(group, 0, offset + Vec2{ dim.x * 0.5f, 0 }, offsetZ, Vec2{}, Vec2{ thickness, dim.y },
              color, entityZC);
}

#endif // HANDMADE_RENDER_GROUP_H
