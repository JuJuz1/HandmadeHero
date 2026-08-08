#include "handmade_render_group.h"

// Ridiculous stuff happening here, we prefix with the enum name...
// If we want to keep this 2 arguments we can't also use enum class
// clang-format off
#define PushRenderElement(group, type) (type*)PushRenderElement_((group), sizeof(type), RenderGroupEntryType_##type)
// clang-format on

NODISCARD
INTERNAL RenderGroupEntryHeader*
PushRenderElement_(RenderGroup* group, i32 size, RenderGroupEntryType type) {
    RenderGroupEntryHeader* result{};

    // TODO: why not <= ???
    if ((group->pushBufferSize + size) < group->maxPushBufferSize) {
        result = reinterpret_cast<RenderGroupEntryHeader*>(group->pushBufferBase +
                                                           group->pushBufferSize);
        result->type = type;
        group->pushBufferSize += size;
    } else {
        INVALID_CODE_PATH;
    }

    return result;
}

INTERNAL void
PushPiece(RenderGroup* group, LoadedBitmapInfo* bitmap, Vec2 offset, f32 offsetZ, Vec2 align,
          Vec4 color, f32 entityZC = 1.0f) {
    //ASSERT(group->pieceCount < group->pieces.size);
    //RenderGroupEntry* piece{ &group->pieces[group->pieceCount++] };
    auto* piece{ PushRenderElement(group, RenderEntryBitmap) };
    if (piece) {
        piece->bitmap = bitmap;

        piece->entityBasis.basis = group->defaultBasis;
        piece->entityBasis.offset = (group->metersToPixels * Vec2{ offset.x, -offset.y }) - align;
        piece->entityBasis.offsetZ = offsetZ;
        piece->entityBasis.entityZC = entityZC;

        // TODO: piece->color = color
        piece->color.r = color.r;
        piece->color.g = color.g;
        piece->color.b = color.b;
        piece->color.a = color.a;
    }
}

INTERNAL void
PushBitmap(RenderGroup* group, LoadedBitmapInfo* bitmap, Vec2 offset, f32 offsetZ, Vec2 align,
           f32 alpha = 1.0f, f32 entityZC = 1.0f) {
    PushPiece(group, bitmap, offset, offsetZ, align, Vec4{ 1.0f, 1.0f, 1.0f, alpha }, entityZC);
}

INTERNAL void
PushRect(RenderGroup* group, Vec2 offset, f32 offsetZ, Vec2 dim, Vec4 color, f32 entityZC = 1.0f) {
    // @Duplicate
    auto* piece{ PushRenderElement(group, RenderEntryRect) };
    if (piece) {
        piece->entityBasis.basis = group->defaultBasis;

        const Vec2 halfDim{ 0.5f * dim * group->metersToPixels };
        piece->entityBasis.offset = (group->metersToPixels * Vec2{ offset.x, -offset.y }) - halfDim;
        piece->entityBasis.offsetZ = offsetZ;
        piece->entityBasis.entityZC = entityZC;

        piece->dim = group->metersToPixels * dim;

        piece->color.r = color.r;
        piece->color.g = color.g;
        piece->color.b = color.b;
        piece->color.a = color.a;
    }
}

INTERNAL void
PushRectOutline(RenderGroup* group, Vec2 offset, f32 offsetZ, Vec2 dim, Vec4 color,
                f32 entityZC = 1.0f) {
    const f32 thickness{ 0.1f };

    // Top bottom
    PushPiece(group, nullptr, offset - Vec2{ 0, dim.y * 0.5f }, offsetZ, Vec2{ dim.x, thickness },
              color, entityZC);
    PushPiece(group, nullptr, offset + Vec2{ 0, dim.y * 0.5f }, offsetZ, Vec2{ dim.x, thickness },
              color, entityZC);

    // Left right
    PushPiece(group, nullptr, offset - Vec2{ dim.x * 0.5f, 0 }, offsetZ, Vec2{ thickness, dim.y },
              color, entityZC);
    PushPiece(group, nullptr, offset + Vec2{ dim.x * 0.5f, 0 }, offsetZ, Vec2{ thickness, dim.y },
              color, entityZC);
}

INTERNAL void
DrawBitmap(LoadedBitmapInfo* buff, const LoadedBitmapInfo* bitmap, f32 xPos, f32 yPos,
           f32 CAlpha = 1.0f) {
    // TODO: never have this case? use a placeholder instead?
    if (!bitmap->memory) {
        return;
    }

    i32 roundedMinX{ RoundF32ToI32(xPos) };
    i32 roundedMinY{ RoundF32ToI32(yPos) };
    i32 roundedMaxX{ roundedMinX + bitmap->width };
    i32 roundedMaxY{ roundedMinY + bitmap->height };

    i32 srcOffsetX{};
    if (roundedMinX < 0) {
        srcOffsetX = -roundedMinX;
        roundedMinX = 0;
    }

    i32 srcOffsetY{};
    if (roundedMinY < 0) {
        srcOffsetY = -roundedMinY;
        roundedMinY = 0;
    }

    if (roundedMaxX > buff->width) {
        roundedMaxX = buff->width;
    }
    if (roundedMaxY > buff->height) {
        roundedMaxY = buff->height;
    }

    // Start from the last row (top row of the image) as the bitmap is stored bottom up
    u8* srcRow{ static_cast<u8*>(bitmap->memory) + (srcOffsetY * bitmap->pitch) +
                (srcOffsetX * bitmap_Bytes_Per_Pixel) };
    u8* destRow{ static_cast<u8*>(buff->memory) + (roundedMinY * buff->pitch) +
                 (roundedMinX * bitmap_Bytes_Per_Pixel) };

    for (i32 y{ roundedMinY }; y < roundedMaxY; ++y) {
        u32* dest{ reinterpret_cast<u32*>(destRow) };
        u32* src{ reinterpret_cast<u32*>(srcRow) };
        for (i32 x{ roundedMinX }; x < roundedMaxX; ++x) {
            const f32 srcAlpha{ static_cast<f32>((*src >> 24) & 0xFF) };
            const f32 srcRelAlpha{ (srcAlpha / 255.0f) * CAlpha };

            const f32 srcRed{ CAlpha * static_cast<f32>((*src >> 16) & 0xFF) };
            const f32 srcGreen{ CAlpha * static_cast<f32>((*src >> 8) & 0xFF) };
            const f32 srcBlue{ CAlpha * static_cast<f32>((*src >> 0) & 0xFF) };

            const f32 destAlpha{ static_cast<f32>((*dest >> 24) & 0xFF) };
            const f32 destRelAlpha{ destAlpha / 255.0f };

            const f32 destRed{ static_cast<f32>((*dest >> 16) & 0xFF) };
            const f32 destGreen{ static_cast<f32>((*dest >> 8) & 0xFF) };
            const f32 destBlue{ static_cast<f32>((*dest >> 0) & 0xFF) };

            const f32 invRelAlpha{ 1.0f - srcRelAlpha };
            const f32 resultAlpha{ 255.0f *
                                   (srcRelAlpha + destRelAlpha - (srcRelAlpha * destRelAlpha)) };
            const f32 resultRed{ (invRelAlpha * destRed) + srcRed };
            const f32 resultGreen{ (invRelAlpha * destGreen) + srcGreen };
            const f32 resultBlue{ (invRelAlpha * destBlue) + srcBlue };

            *dest = { (TruncateF32ToU32(resultAlpha + 0.5f) << 24) |
                      (TruncateF32ToU32(resultRed + 0.5f) << 16) |
                      (TruncateF32ToU32(resultGreen + 0.5f) << 8) |
                      (TruncateF32ToU32(resultBlue + 0.5f) << 0) };

            ++dest;
            ++src;
        }

        destRow += buff->pitch;
        // Move to the start of the above row
        srcRow += bitmap->pitch;
    }
}

INTERNAL void
DrawRect(const LoadedBitmapInfo* buff, Vec2 min, Vec2 max, f32 r, f32 g, f32 b) {
    i32 roundedMinX{ RoundF32ToI32(min.x) };
    i32 roundedMinY{ RoundF32ToI32(min.y) };
    i32 roundedMaxX{ RoundF32ToI32(max.x) };
    i32 roundedMaxY{ RoundF32ToI32(max.y) };

    if (roundedMinX < 0) {
        roundedMinX = 0;
    }
    if (roundedMinY < 0) {
        roundedMinY = 0;
    }

    if (roundedMaxX > buff->width) {
        roundedMaxX = buff->width;
    }
    if (roundedMaxY > buff->height) {
        roundedMaxY = buff->height;
    }

    // AA RR GG BB
    const i32 color{ (RoundF32ToI32(r * 255.0f) << 16) | (RoundF32ToI32(g * 255.0f) << 8) |
                     (RoundF32ToI32(b * 255.0f) << 0) };

    u8* memory{ static_cast<u8*>(buff->memory) };
    u8* row{ memory + (roundedMinX * bitmap_Bytes_Per_Pixel) + (roundedMinY * buff->pitch) };

    for (i32 y{ roundedMinY }; y < roundedMaxY; ++y) {
        // Not including fill pixel
        u32* pixel{ reinterpret_cast<u32*>(row) };
        for (i32 x{ roundedMinX }; x < roundedMaxX; ++x) {
            *pixel++ = color;
        }

        row += buff->pitch;
    }
}

INTERNAL void
DrawRectOutline(const LoadedBitmapInfo* buff, Vec2 min, Vec2 max, Vec3 color,
                f32 thickness = 1.0f) {
    // Top bottom
    DrawRect(buff, Vec2{ min.x - thickness, min.y - thickness },
             Vec2{ max.x + thickness, min.y + thickness }, color.r, color.g, color.b);
    DrawRect(buff, Vec2{ min.x - thickness, max.y - thickness },
             Vec2{ max.x + thickness, max.y + thickness }, color.r, color.g, color.b);

    // Left right
    DrawRect(buff, Vec2{ min.x - thickness, min.y - thickness },
             Vec2{ min.x + thickness, max.y + thickness }, color.r, color.g, color.b);
    DrawRect(buff, Vec2{ max.x - thickness, min.y - thickness },
             Vec2{ max.x + thickness, max.y + thickness }, color.r, color.g, color.b);
}

NODISCARD
INTERNAL RenderGroup*
AllocRenderGroup(MemoryArena* arena, i32 maxPushBufferSize, f32 metersToPixels) {
    RenderGroup* result{ PushStruct(arena, RenderGroup) };
    result->pushBufferBase = static_cast<u8*>(PushSize(arena, maxPushBufferSize));

    result->defaultBasis = PushStruct(arena, RenderBasis);
    result->defaultBasis->pos = Vec3{};
    result->metersToPixels = metersToPixels;

    result->maxPushBufferSize = maxPushBufferSize;
    result->pushBufferSize = 0;

    return result;
}

NODISCARD
INTERNAL Vec2
GetRenderEntityBasisPos(RenderGroup* group, RenderEntityBasis* entityBasis, Vec2 screenCenter) {
    const Vec3 entityBasePos{ entityBasis->basis->pos };
    const f32 zFudge{ 1.0f + 0.1f * (entityBasePos.z + entityBasis->offsetZ) };

    //const Vec2 entityGroundPoint{ screenCenter.x + (gameState->metersToPixels
    //* entity->pos.x),
    //                              screenCenter.y -
    //                                  (gameState->metersToPixels *
    //                                  entity->pos.y) };
    const Vec2 entityGroundPoint{
        screenCenter.x + (group->metersToPixels * entityBasePos.x * zFudge),
        screenCenter.y - (group->metersToPixels * entityBasePos.y * zFudge)
    };
    const f32 entityZ{ -entityBasePos.z * group->metersToPixels };

    const Vec2 center{ entityGroundPoint.x + entityBasis->offset.x,
                       entityGroundPoint.y + entityBasis->offset.y +
                           //(group->metersToPixels * entityBasis->offsetZ) +
                           (entityZ * entityBasis->entityZC) };

    return center;
}

INTERNAL void
RenderGroupToOutput(RenderGroup* group, LoadedBitmapInfo* outputTarget, GameState* gameState) {
    const Vec2 screenCenter{ static_cast<f32>(outputTarget->width) * 0.5f,
                             static_cast<f32>(outputTarget->height) * 0.5f };

    for (i32 baseAddress{}; baseAddress < group->pushBufferSize;) {
        auto* header{ reinterpret_cast<RenderGroupEntryHeader*>(group->pushBufferBase +
                                                                baseAddress) };

        switch (header->type) {
            // This is a bit ugly but it can't be perfect everywhere
        case RenderGroupEntryType_RenderEntryClear: {
            auto* entry{ reinterpret_cast<RenderEntryClear*>(header) };
            baseAddress += sizeof(*entry);
        } break;
        case RenderGroupEntryType_RenderEntryRect: {
            auto* entry{ reinterpret_cast<RenderEntryRect*>(header) };
            baseAddress += sizeof(*entry);

            const Vec2 pos{ GetRenderEntityBasisPos(group, &entry->entityBasis, screenCenter) };

            DrawRect(outputTarget, pos, pos + entry->dim, entry->color.r, entry->color.g,
                     entry->color.b);
        } break;
        case RenderGroupEntryType_RenderEntryBitmap: {
            auto* entry{ reinterpret_cast<RenderEntryBitmap*>(header) };
            baseAddress += sizeof(*entry);

            const Vec2 pos{ GetRenderEntityBasisPos(group, &entry->entityBasis, screenCenter) };

            ASSERT(entry->bitmap);
            DrawBitmap(outputTarget, entry->bitmap, pos.x, pos.y, entry->color.a);
        } break;

            INVALID_DEFAULT_CASE;
        }

        // @Debug collision box
        if (gameState->showCollisionBoxes) {
// Don't draw for room space as it blocks the whole screen
// @Re-enable after getting reference to entity here, probably store to the piece?
#if 0
            if (entity->type != EntityType::SPACE) {
                const Vec2 leftTop{ entityGroundPoint.x - (0.5f * group->metersToPixels *
                                                           entity->collision->totalVolume.dim.x),
                                    entityGroundPoint.y - (0.5f * group->metersToPixels *
                                                           entity->collision->totalVolume.dim.y) };

                DrawRect(outputTarget, leftTop,
                         leftTop + entity->collision->totalVolume.dim.xy *
                                       group->metersToPixels // *0.95f
                         ,
                         0.5f, 0.1f, 0.5f);
            }
#endif
        }
    }
}
