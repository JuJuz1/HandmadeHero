#include "handmade_render_group.h"

NODISCARD
INTERNAL RenderGroup*
AllocRenderGroup(MemoryArena* arena, i32 maxPushBufferSize, f32 metersToPixels) {
    RenderGroup* result{ PushStruct(arena, RenderGroup) };
    result->pushBufferBase = static_cast<u8*>(PushSize(arena, maxPushBufferSize));

    result->defaultBasis = PushStruct(arena, RenderBasis);
    result->defaultBasis->pos = Vec3{};
    result->metersToPixels = metersToPixels;

    result->pieceCount = 0;
    result->maxPushBufferSize = maxPushBufferSize;
    result->pushBufferSize = 0;

    return result;
}
