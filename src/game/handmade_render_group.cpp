#include "handmade_render_group.h"

// Ridiculous stuff happening here, we prefix with the enum name...
// If we want to keep this 2 arguments we can't also use enum class
// clang-format off
#define PushRenderElement(group, type) (type*)PushRenderElement_((group), sizeof(type), RenderGroupEntryType_##type)
// clang-format on

NODISCARD
INTERNAL void*
PushRenderElement_(RenderGroup* group, i32 size, RenderGroupEntryType type) {
    void* result{};

    size += sizeof(RenderGroupEntryHeader);

    // TODO: why not <= ??? now we prevent pushing if we hit max size
    if ((group->pushBufferSize + size) <= group->maxPushBufferSize) {
        auto* header{ reinterpret_cast<RenderGroupEntryHeader*>(group->pushBufferBase +
                                                                group->pushBufferSize) };
        header->type = type;
        result = header + 1; // Data is right after the header
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
    auto* entry{ PushRenderElement(group, RenderEntryBitmap) };
    if (entry) {
        entry->bitmap = bitmap;

        entry->entityBasis.basis = group->defaultBasis;
        entry->entityBasis.offset = (group->metersToPixels * Vec2{ offset.x, -offset.y }) - align;
        entry->entityBasis.offsetZ = offsetZ;
        entry->entityBasis.entityZC = entityZC;

        entry->color = color;
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
    auto* entry{ PushRenderElement(group, RenderEntryRect) };
    if (entry) {
        entry->entityBasis.basis = group->defaultBasis;

        const Vec2 halfDim{ 0.5f * dim * group->metersToPixels };
        entry->entityBasis.offset = (group->metersToPixels * Vec2{ offset.x, -offset.y }) - halfDim;
        entry->entityBasis.offsetZ = offsetZ;
        entry->entityBasis.entityZC = entityZC;

        entry->dim = group->metersToPixels * dim;

        entry->color = color;
    }
}

INTERNAL void
PushRectOutline(RenderGroup* group, Vec2 offset, f32 offsetZ, Vec2 dim, Vec4 color,
                f32 thickness = 0.1f, f32 entityZC = 1.0f) {
    // Top bottom
    PushRect(group, offset - Vec2{ 0, dim.y * 0.5f }, offsetZ, Vec2{ dim.x, thickness }, color,
             entityZC);
    PushRect(group, offset + Vec2{ 0, dim.y * 0.5f }, offsetZ, Vec2{ dim.x, thickness }, color,
             entityZC);

    // Left right
    PushRect(group, offset - Vec2{ dim.x * 0.5f, 0 }, offsetZ, Vec2{ thickness, dim.y }, color,
             entityZC);
    PushRect(group, offset + Vec2{ dim.x * 0.5f, 0 }, offsetZ, Vec2{ thickness, dim.y }, color,
             entityZC);
}

NODISCARD
INTERNAL RenderEntryCoordinateSystem*
PushCoordinateSystem(RenderGroup* group, Vec2 origin, Vec2 xAxis, Vec2 yAxis, Vec4 color,
                     LoadedBitmapInfo* texture, LoadedBitmapInfo* normalMap, EnvironmentMap* top,
                     EnvironmentMap* middle, EnvironmentMap* bottom) {
    auto* entry{ PushRenderElement(group, RenderEntryCoordinateSystem) };
    if (entry) {
        entry->origin = origin;
        entry->xAxis = xAxis;
        entry->yAxis = yAxis;
        entry->color = color;
        entry->texture = texture;

        entry->normalMap = normalMap;
        entry->top = top;
        entry->middle = middle;
        entry->bottom = bottom;
    }

    return entry;
}

INTERNAL void
ScreenClear(RenderGroup* group, Vec4 color) {
    auto* entry{ PushRenderElement(group, RenderEntryClear) };
    if (entry) {
        entry->color = color;
    }
}

INTERNAL void
PushSaturation(RenderGroup* group, f32 saturation) {
    auto* entry{ PushRenderElement(group, RenderEntrySaturation) };
    if (entry) {
        entry->saturation = saturation;
    }
}

INTERNAL void
PushCollisionBox(RenderGroup* group, SimEntityCollisionVolumeGroup* collision, Vec4 color,
                 f32 scale) {
    PushRect(group, collision->totalVolume.offsetPos.xy, 0.0f,
             collision->totalVolume.dim.xy * scale, color);
}

NODISCARD
INTERNAL Vec4
SRGB255ToLinear1(Vec4 color) {
    Vec4 result;

    const f32 inv255{ 1.0f / 255.0f };
    result.r = SquareF32(color.r * inv255);
    result.g = SquareF32(color.g * inv255);
    result.b = SquareF32(color.b * inv255);
    result.a = color.a * inv255;

    return result;
}

NODISCARD
INTERNAL Vec4
Linear1ToSRGB255(Vec4 color) {
    Vec4 result;

    const f32 one255{ 255.0f };
    result.r = Sqrt(color.r) * one255;
    result.g = Sqrt(color.g) * one255;
    result.b = Sqrt(color.b) * one255;
    result.a = color.a * one255;

    return result;
}

NODISCARD
INTERNAL inline Vec4
UnscaleAndBiasNormal(Vec4 normal) {
    Vec4 result;

    const f32 inv255{ 1.0f / 255.0f };
    result.x = -1.0f + 2.0f * (normal.x * inv255);
    result.y = -1.0f + 2.0f * (normal.y * inv255);
    result.z = -1.0f + 2.0f * (normal.z * inv255);

    result.w = normal.w * inv255;

    return result;
}

NODISCARD
INTERNAL inline Vec4
Unpack4x8(u32 packed) {
    const Vec4 result{ static_cast<f32>((packed >> 16) & 0xFF),
                       static_cast<f32>((packed >> 8) & 0xFF),
                       static_cast<f32>((packed >> 0) & 0xFF),
                       static_cast<f32>((packed >> 24) & 0xFF) };

    return result;
}

struct BilinearSample {
    i32 a, b, c, d;
};

NODISCARD
INTERNAL inline Vec4
SRGBBilinearBlend(BilinearSample sample, f32 fX, f32 fY) {
    Vec4 result;

    // TODO: color.a
    Vec4 texelA{ Unpack4x8(sample.a) };
    Vec4 texelB{ Unpack4x8(sample.b) };
    Vec4 texelC{ Unpack4x8(sample.c) };
    Vec4 texelD{ Unpack4x8(sample.d) };

    texelA = SRGB255ToLinear1(texelA);
    texelB = SRGB255ToLinear1(texelB);
    texelC = SRGB255ToLinear1(texelC);
    texelD = SRGB255ToLinear1(texelD);

    // Lerp the color with the neighbours
    result = Lerp(Lerp(texelA, fX, texelB), fY, Lerp(texelC, fX, texelD));

    return result;
}

NODISCARD
INTERNAL inline BilinearSample
BilinearSampleFromTex(LoadedBitmapInfo* texture, i32 x, i32 y) {
    BilinearSample result;

    u8* normalPtr{ static_cast<u8*>(texture->memory) + y * texture->pitch + x * sizeof(u32) };
    result.a = *reinterpret_cast<u32*>(normalPtr);
    result.b = *reinterpret_cast<u32*>(normalPtr + sizeof(u32));
    result.c = *reinterpret_cast<u32*>(normalPtr + texture->pitch);
    result.d = *reinterpret_cast<u32*>(normalPtr + texture->pitch + sizeof(u32));

    return result;
}

NODISCARD
INTERNAL inline Vec3
SampleEnvironmentMap(Vec2 screenSpaceUV, Vec3 sampleDir, f32 roughness, EnvironmentMap* map) {
    ASSERT(roughness >= 0.0f && roughness <= 1.0f);
    const i32 lodIndex{ RoundF32ToI32(roughness * (map->lod.size - 1)) };
    ASSERT(lodIndex < map->lod.size);

    auto* lod{ &map->lod[lodIndex] };

    ASSERT(sampleDir.y > 0.0f);
    const f32 distFromMapInZ{ 1.0f };
    const f32 UVsPerMeter{ 0.01f }; // TODO: figure out
    const f32 coefficient{ (UVsPerMeter * distFromMapInZ) / sampleDir.y };
    const Vec2 offset{ Vec2{ sampleDir.x, sampleDir.z } * coefficient };

    Vec2 uv{ screenSpaceUV + offset };
    uv.x = Clamp01(uv.x);
    uv.y = Clamp01(uv.y);

    f32 texelX{ uv.x * static_cast<f32>(lod->width - 2) };
    f32 texelY{ uv.y * static_cast<f32>(lod->height - 2) };

    //const f32 texelX{ lod->width / 2 + (sampleDir.x * lod->width / 2) };
    //const f32 texelY{ lod->height / 2 + (sampleDir.y * lod->height / 2) };

    // @Duplicate
    const i32 roundedX{ static_cast<i32>(texelX) };
    const i32 roundedY{ static_cast<i32>(texelY) };
    ASSERT(roundedX >= 0 && roundedX < lod->width);
    ASSERT(roundedY >= 0 && roundedY < lod->height);

    const f32 fX{ static_cast<f32>(texelX - roundedX) };
    const f32 fY{ static_cast<f32>(texelY - roundedY) };

    auto sample{ BilinearSampleFromTex(lod, roundedX, roundedY) };
    Vec3 result{ SRGBBilinearBlend(sample, fX, fY).xyz };

    return result;
}

INTERNAL void
ChangeSaturation(LoadedBitmapInfo* buff, f32 saturation) {
    u8* destRow{ static_cast<u8*>(buff->memory) };

    for (i32 y{}; y < buff->height; ++y) {
        u32* destPtr{ reinterpret_cast<u32*>(destRow) };
        for (i32 x{}; x < buff->width; ++x) {
            Vec4 dest{ static_cast<f32>((*destPtr >> 16) & 0xFF),
                       static_cast<f32>((*destPtr >> 8) & 0xFF),
                       static_cast<f32>((*destPtr >> 0) & 0xFF),
                       static_cast<f32>((*destPtr >> 24) & 0xFF) };
            dest = SRGB255ToLinear1(dest);
            //const f32 destRelAlpha{ dest.a / 255.0f };

            const f32 avg{ (1.0f / 3.0f) * (dest.r + dest.g + dest.b) };
            const Vec3 delta{ dest.r - avg, dest.g - avg, dest.b - avg };

            Vec4 result{ Vec3{ avg, avg, avg } + (saturation * delta), dest.a };
            result = Linear1ToSRGB255(result);

            *destPtr = { (TruncateF32ToU32(result.a + 0.5f) << 24) |
                         (TruncateF32ToU32(result.r + 0.5f) << 16) |
                         (TruncateF32ToU32(result.g + 0.5f) << 8) |
                         (TruncateF32ToU32(result.b + 0.5f) << 0) };

            ++destPtr;
        }

        destRow += buff->pitch;
    }
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
        u32* destPtr{ reinterpret_cast<u32*>(destRow) };
        u32* srcPtr{ reinterpret_cast<u32*>(srcRow) };
        for (i32 x{ roundedMinX }; x < roundedMaxX; ++x) {
            Vec4 texel{ static_cast<f32>((*srcPtr >> 16) & 0xFF),
                        static_cast<f32>((*srcPtr >> 8) & 0xFF),
                        static_cast<f32>((*srcPtr >> 0) & 0xFF),
                        static_cast<f32>((*srcPtr >> 24) & 0xFF) };
            texel = SRGB255ToLinear1(texel);
            texel *= CAlpha;

            Vec4 dest{ static_cast<f32>((*destPtr >> 16) & 0xFF),
                       static_cast<f32>((*destPtr >> 8) & 0xFF),
                       static_cast<f32>((*destPtr >> 0) & 0xFF),
                       static_cast<f32>((*destPtr >> 24) & 0xFF) };
            dest = SRGB255ToLinear1(dest);
            //const f32 destRelAlpha{ dest.a / 255.0f };

            Vec4 result{ (dest * (1.0f - texel.a)) + texel };
            result = Linear1ToSRGB255(result);

            *destPtr = { (TruncateF32ToU32(result.a + 0.5f) << 24) |
                         (TruncateF32ToU32(result.r + 0.5f) << 16) |
                         (TruncateF32ToU32(result.g + 0.5f) << 8) |
                         (TruncateF32ToU32(result.b + 0.5f) << 0) };

            ++destPtr;
            ++srcPtr;
        }

        destRow += buff->pitch;
        // Move to the start of the above row
        srcRow += bitmap->pitch;
    }
}

INTERNAL void
DrawRect(const LoadedBitmapInfo* buff, Vec2 min, Vec2 max, Vec4 color) {
    f32 r{ color.r };
    f32 g{ color.g };
    f32 b{ color.b };
    f32 a{ color.a };

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
    const i32 roundedColor{ (RoundF32ToI32(a * 255.0f) << 24) | (RoundF32ToI32(r * 255.0f) << 16) |
                            (RoundF32ToI32(g * 255.0f) << 8) | (RoundF32ToI32(b * 255.0f) << 0) };

    u8* memory{ static_cast<u8*>(buff->memory) };
    u8* row{ memory + (roundedMinX * bitmap_Bytes_Per_Pixel) + (roundedMinY * buff->pitch) };

    for (i32 y{ roundedMinY }; y < roundedMaxY; ++y) {
        // Not including fill pixel
        u32* pixel{ reinterpret_cast<u32*>(row) };
        for (i32 x{ roundedMinX }; x < roundedMaxX; ++x) {
            *pixel++ = roundedColor;
        }

        row += buff->pitch;
    }
}

INTERNAL void
DrawRectSlowly(const LoadedBitmapInfo* buff, Vec2 origin, Vec2 xAxis, Vec2 yAxis, Vec4 color,
               LoadedBitmapInfo* texture, LoadedBitmapInfo* normalMap, EnvironmentMap* top,
               EnvironmentMap* middle, EnvironmentMap* bottom) {
    // Premultiply color
    color.rgb *= color.a;
    // AA RR GG BB
    u32 colorRounded{ (RoundF32ToU32(color.a * 255.0f) << 24) |
                      (RoundF32ToU32(color.r * 255.0f) << 16) |
                      (RoundF32ToU32(color.g * 255.0f) << 8) |
                      (RoundF32ToU32(color.b * 255.0f) << 0) };

    const i32 widthMax{ buff->width - 1 };
    const i32 heightMax{ buff->height - 1 };
    const f32 widthMaxInv{ 1.0f / static_cast<f32>(buff->width - 1) };
    const f32 heightMaxInv{ 1.0f / static_cast<f32>(buff->height - 1) };

    i32 minX{ widthMax };
    i32 minY{ heightMax };
    i32 maxX{};
    i32 maxY{};

#if 1
    Array<Vec2, 4> points{ origin, origin + xAxis, origin + xAxis + yAxis, origin + yAxis };
    for (i32 i{}; i < points.size; ++i) {
        const i32 floorX{ FloorF32ToI32(points[i].x) };
        const i32 ceilX{ CeilF32ToI32(points[i].x) };
        const i32 floorY{ FloorF32ToI32(points[i].y) };
        const i32 ceilY{ CeilF32ToI32(points[i].y) };

        if (floorX < minX) {
            minX = floorX;
        }
        if (ceilX > maxX) {
            maxX = ceilX;
        }
        if (floorY < minY) {
            minY = floorY;
        }
        if (ceilY > maxY) {
            maxY = ceilY;
        }
    }
#endif

    if (minX < 0) {
        minX = 0;
    }
    if (minY < 0) {
        minY = 0;
    }
    if (maxX > buff->width - 1) {
        maxX = buff->width - 1;
    }
    if (maxY > buff->height - 1) {
        maxY = buff->height - 1;
    }

    const f32 xAxisLenSqInv{ 1.0f / LengthSq(xAxis) };
    const f32 yAxisLenSqInv{ 1.0f / LengthSq(yAxis) };

    u8* row{ static_cast<u8*>(buff->memory) + (minX * bitmap_Bytes_Per_Pixel) +
             (minY * buff->pitch) };

    for (i32 y{ minY }; y <= maxY; ++y) {
        u32* pixel{ reinterpret_cast<u32*>(row) };
        for (i32 x{ minX }; x <= maxX; ++x) {
            const Vec2 pixelPos{ x, y };
            const Vec2 d{ pixelPos - origin };

            const f32 edge0{ Dot(d, -Perp(xAxis)) };
            const f32 edge1{ Dot(d - xAxis, -Perp(yAxis)) };
            const f32 edge2{ Dot(d - xAxis - yAxis, Perp(xAxis)) };
            const f32 edge3{ Dot(d - yAxis, Perp(yAxis)) };
            if ((edge0 < 0) && (edge1 < 0) && (edge2 < 0) && (edge3 < 0)) {
                // Lookup into texture
                const Vec2 uv{ Dot(d, xAxis) * xAxisLenSqInv, Dot(d, yAxis) * yAxisLenSqInv };
                // TODO: needs to be clamped
                // @Re-enable asserts
                //ASSERT(uv.x >= 0.0f && uv.x <= 1.0f);
                //ASSERT(uv.y >= 0.0f && uv.y <= 1.0f);

                // Pretend the texture is 1 pixel smaller in both dimensions
                const f32 texelX{ uv.x * static_cast<f32>(texture->width - 2) };
                const f32 texelY{ uv.y * static_cast<f32>(texture->height - 2) };

                const i32 roundedX{ static_cast<i32>(texelX) };
                const i32 roundedY{ static_cast<i32>(texelY) };
                //ASSERT(roundedX >= 0 && roundedX < texture->width);
                //ASSERT(roundedY >= 0 && roundedY < texture->height);

                const f32 fX{ static_cast<f32>(texelX - roundedX) };
                const f32 fY{ static_cast<f32>(texelY - roundedY) };

                auto texelSample{ BilinearSampleFromTex(texture, roundedX, roundedY) };
                Vec4 texel{ SRGBBilinearBlend(texelSample, fX, fY) };

                /// Normals
                if (normalMap) {
                    auto normalSample{ BilinearSampleFromTex(normalMap, roundedX, roundedY) };

                    Vec4 normalA{ Unpack4x8(normalSample.a) };
                    Vec4 normalB{ Unpack4x8(normalSample.b) };
                    Vec4 normalC{ Unpack4x8(normalSample.c) };
                    Vec4 normalD{ Unpack4x8(normalSample.d) };

                    Vec4 normal{ Lerp(Lerp(normalA, fX, normalB), fY, Lerp(normalC, fX, normalD)) };
                    normal = UnscaleAndBiasNormal(normal);
                    // TODO: needed?
                    normal.xyz = Normalize(normal.xyz);

#if 1
                    // Assumed to always be {0, 0, 1}, so we can simplify
                    Vec3 bounceDir{ 2.0f * normal.z * normal.xyz };
                    bounceDir.z -= 1.0f;

                    EnvironmentMap* farMap{};
                    f32 tEnvMap{ bounceDir.y };
                    f32 tFarMap{};
                    if (tEnvMap < -0.5f) {
                        farMap = bottom;
                        tFarMap = 1.0f - ((tEnvMap + 1.0f) * 2);
                        bounceDir.y = -bounceDir.y;
                    } else if (tEnvMap > 0.5f) {
                        farMap = top;
                        tFarMap = (tEnvMap - 0.5f) * 2;
                    }

                    const Vec2 screenSpaceUV{ x * widthMaxInv, y * heightMaxInv };
                    Vec3 lightColor{
                        //SampleEnvironmentMap(screenSpaceUV, normal.xyz, normal.w, middle)
                    };
                    if (farMap) {
                        const Vec3 farMapColor{ SampleEnvironmentMap(screenSpaceUV, bounceDir,
                                                                     normal.w, farMap) };
                        lightColor = Lerp(lightColor, tFarMap, farMapColor);
                    }

                    texel.rgb += texel.a * lightColor;
                }

                // Figure out the final color
                texel *= color;
                texel.r = Clamp01(texel.r);
                texel.g = Clamp01(texel.g);
                texel.b = Clamp01(texel.b);
                //texel.a = Clamp01(texel.a);

                Vec4 dest{ static_cast<f32>((*pixel >> 16) & 0xFF),
                           static_cast<f32>((*pixel >> 8) & 0xFF),
                           static_cast<f32>((*pixel >> 0) & 0xFF),
                           static_cast<f32>((*pixel >> 24) & 0xFF) };
                dest = SRGB255ToLinear1(dest);

                Vec4 blended{ (dest * (1.0f - texel.a)) + texel };
                blended = Linear1ToSRGB255(blended);

                *pixel = { (TruncateF32ToU32(blended.a + 0.5f) << 24) |
                           (TruncateF32ToU32(blended.r + 0.5f) << 16) |
                           (TruncateF32ToU32(blended.g + 0.5f) << 8) |
                           (TruncateF32ToU32(blended.b + 0.5f) << 0) };
            }
#else
                    *pixel = colorRounded;
#endif

            ++pixel;
        }

        row += buff->pitch;
    }
}

// We simply don't need this now as we use PushRectOutline to do this via the push buffer
#if 0
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
#endif

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
    const Vec2 screenCenter{ outputTarget->width * 0.5f, outputTarget->height * 0.5f };

    for (i32 baseAddress{}; baseAddress < group->pushBufferSize;) {
        auto* header{ reinterpret_cast<RenderGroupEntryHeader*>(group->pushBufferBase +
                                                                baseAddress) };
        baseAddress += sizeof(*header);
        // Data is located right after
        void* data{ header + 1 };

        switch (header->type) {
            // This is a bit ugly but it can't be perfect everywhere
        case RenderGroupEntryType_RenderEntryClear: {
            auto* entry{ reinterpret_cast<RenderEntryClear*>(data) };
            baseAddress += sizeof(*entry);

            DrawRect(outputTarget, Vec2{}, Vec2{ outputTarget->width, outputTarget->height },
                     entry->color);
        } break;
        case RenderGroupEntryType_RenderEntryRect: {
            auto* entry{ reinterpret_cast<RenderEntryRect*>(data) };
            baseAddress += sizeof(*entry);

            const Vec2 pos{ GetRenderEntityBasisPos(group, &entry->entityBasis, screenCenter) };

            DrawRect(outputTarget, pos, pos + entry->dim, entry->color);
        } break;
        case RenderGroupEntryType_RenderEntryBitmap: {
            auto* entry{ reinterpret_cast<RenderEntryBitmap*>(data) };
            baseAddress += sizeof(*entry);

#if 0
            const Vec2 pos{ GetRenderEntityBasisPos(group, &entry->entityBasis, screenCenter) };

            ASSERT(entry->bitmap);
            DrawBitmap(outputTarget, entry->bitmap, pos.x, pos.y, entry->color.a);
#endif
        } break;
        case RenderGroupEntryType_RenderEntryCoordinateSystem: {
            auto* entry{ reinterpret_cast<RenderEntryCoordinateSystem*>(data) };
            baseAddress += sizeof(*entry);

            const Vec2 dim{ 2, 2 };
            const Vec4 color{ 1, 1, 0, 1 };
            Vec2 pos{ entry->origin };
            DrawRect(outputTarget, pos - dim, pos + dim, color);
            pos = entry->origin + entry->xAxis;
            DrawRect(outputTarget, pos - dim, pos + dim, color);
            pos = entry->origin + entry->yAxis;
            DrawRect(outputTarget, pos - dim, pos + dim, color);
            pos = entry->origin + entry->xAxis + entry->yAxis;
            DrawRect(outputTarget, pos - dim, pos + dim, color);

            DrawRectSlowly(outputTarget, entry->origin, entry->xAxis, entry->yAxis, entry->color,
                           entry->texture, entry->normalMap, entry->top, entry->middle,
                           entry->bottom);

#if 0
            for (i32 i{}; i < entry->points.size; ++i) {
                Vec2 p{ entry->points[i] };
                p = entry->origin + (entry->xAxis * p.x) + (entry->yAxis * p.y);
                // Hadamard produces a funny squeezing grid
                //p = entry->origin + (entry->xAxis * p) + (entry->yAxis * p);
                DrawRect(outputTarget, p - dim, p + dim, entry->color.r, entry->color.g,
                         entry->color.b);
            }
#endif
        } break;

        case RenderGroupEntryType_RenderEntrySaturation: {
            auto* entry{ reinterpret_cast<RenderEntrySaturation*>(data) };
            baseAddress += sizeof(*entry);

            ChangeSaturation(outputTarget, entry->saturation);
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
