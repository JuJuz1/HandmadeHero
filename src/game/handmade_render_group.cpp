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
PushBitmap(RenderGroup* group, LoadedBitmapInfo* bitmap, Vec3 offset, f32 height,
           Vec4 color = Vec4::ONE) {
    //ASSERT(group->pieceCount < group->pieces.size);
    //RenderGroupEntry* piece{ &group->pieces[group->pieceCount++] };
    auto* entry{ PushRenderElement(group, RenderEntryBitmap) };
    if (entry) {
        entry->bitmap = bitmap;

        entry->entityBasis.basis = group->defaultBasis;
        const Vec2 size{ bitmap->widthOverHeight * height, height };
        entry->size = size;
        const Vec2 align{ bitmap->alignPercentage * size };
        entry->entityBasis.offset = offset - Vec3{ align, 0 };

        entry->color = color * group->globalAlpha;
    }
}

INTERNAL void
PushRect(RenderGroup* group, Vec3 offset, Vec2 dim, Vec4 color = Vec4::ONE) {
    // @Duplicate
    auto* entry{ PushRenderElement(group, RenderEntryRect) };
    if (entry) {
        entry->entityBasis.basis = group->defaultBasis;
        entry->entityBasis.offset = offset - Vec3{ dim * 0.5f, 0 };

        entry->dim = dim;
        // TODO: global alpha for rects
        entry->color = color //* group->globalAlpha
            ;
    }
}

INTERNAL void
PushRectOutline(RenderGroup* group, Vec3 offset, Vec2 dim, Vec4 color = Vec4::ONE,
                f32 thickness = 0.1f) {
    // Top bottom
    PushRect(group, offset - Vec3{ 0, dim.y * 0.5f, 0 }, Vec2{ dim.x, thickness }, color);
    PushRect(group, offset + Vec3{ 0, dim.y * 0.5f, 0 }, Vec2{ dim.x, thickness }, color);

    // Left right
    PushRect(group, offset - Vec3{ dim.x * 0.5f, 0, 0 }, Vec2{ thickness, dim.y }, color);
    PushRect(group, offset + Vec3{ dim.x * 0.5f, 0, 0 }, Vec2{ thickness, dim.y }, color);
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
    PushRect(group, {}, collision->totalVolume.dim.xy * scale, color);
}

NODISCARD
INTERNAL Vec4
SRGB255ToLinear1(Vec4 color) {
    Vec4 result;

    const f32 inv255{ 1.0f / 255.0f };
    result.r = Square(color.r * inv255);
    result.g = Square(color.g * inv255);
    result.b = Square(color.b * inv255);
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

    u8* texelPtr{ static_cast<u8*>(texture->memory) + y * texture->pitch + x * sizeof(u32) };
    result.a = *reinterpret_cast<u32*>(texelPtr);
    result.b = *reinterpret_cast<u32*>(texelPtr + sizeof(u32));
    result.c = *reinterpret_cast<u32*>(texelPtr + texture->pitch);
    result.d = *reinterpret_cast<u32*>(texelPtr + texture->pitch + sizeof(u32));

    return result;
}

NODISCARD
INTERNAL inline Vec3
SampleEnvironmentMap(Vec2 screenSpaceUV, Vec3 sampleDir, f32 roughness, EnvironmentMap* map,
                     f32 distanceFromMapInZ) {
    /*
       screenSpaceUV tells us where the ray is being cast from in normalized screen coordinates

       sampleDir tells us what direction the cast is going, doesn't have to be normalized

       roughness tells us which LODs of the map we sample from

       distanceFromMapInZ tells us how far the map is from the sample point in Z, in meters
    */

    ASSERT(roughness >= 0.0f && roughness <= 1.0f);
    // TODO: This is hit every time...
    //ASSERT(sampleDir.y > 0.0f);

    const i32 lodIndex{ RoundToI32(roughness * static_cast<f32>(map->lod.size - 1)) };
    ASSERT(lodIndex < map->lod.size);

    auto* lod{ &map->lod[lodIndex] };

    // Compute the distance to the map and the scaling factor from meters to UVs
    const f32 uvsPerMeter{
        0.1f
    }; // TODO: parameterize this, and should differ for X/Y based on map
    const f32 coefficient{ (uvsPerMeter * distanceFromMapInZ) / sampleDir.y };
    const Vec2 offset{ Vec2{ sampleDir.x, sampleDir.z } * coefficient };

    Vec2 uv{ screenSpaceUV + offset };
    uv = Clamp01(uv);

    // Bilinear sample again
    const f32 texelX{ uv.x * static_cast<f32>(lod->width - 2) };
    const f32 texelY{ uv.y * static_cast<f32>(lod->height - 2) };

    const i32 roundedX{ static_cast<i32>(texelX) };
    const i32 roundedY{ static_cast<i32>(texelY) };

    ASSERT(roundedX >= 0 && roundedX < lod->width);
    ASSERT(roundedY >= 0 && roundedY < lod->height);

    const f32 fX{ texelX - static_cast<f32>(roundedX) };
    const f32 fY{ texelY - static_cast<f32>(roundedY) };

    auto sample{ BilinearSampleFromTex(lod, roundedX, roundedY) };
    const Vec3 result{ SRGBBilinearBlend(sample, fX, fY).xyz };

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

            *destPtr = { (TruncToU32(result.a + 0.5f) << 24) | (TruncToU32(result.r + 0.5f) << 16) |
                         (TruncToU32(result.g + 0.5f) << 8) | (TruncToU32(result.b + 0.5f) << 0) };

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

    i32 roundedMinX{ RoundToI32(xPos) };
    i32 roundedMinY{ RoundToI32(yPos) };
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

            *destPtr = { (TruncToU32(result.a + 0.5f) << 24) | (TruncToU32(result.r + 0.5f) << 16) |
                         (TruncToU32(result.g + 0.5f) << 8) | (TruncToU32(result.b + 0.5f) << 0) };

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

    i32 roundedMinX{ RoundToI32(min.x) };
    i32 roundedMinY{ RoundToI32(min.y) };
    i32 roundedMaxX{ RoundToI32(max.x) };
    i32 roundedMaxY{ RoundToI32(max.y) };

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
    const i32 roundedColor{ (RoundToI32(a * 255.0f) << 24) | (RoundToI32(r * 255.0f) << 16) |
                            (RoundToI32(g * 255.0f) << 8) | (RoundToI32(b * 255.0f) << 0) };

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
               EnvironmentMap* middle, EnvironmentMap* bottom, f32 pixelsToMeters) {
    BEGIN_TIMED_BLOCK(DrawRectSlowly);

    ASSERT(texture);

    // Premultiply color
    color.rgb *= color.a;
    // AA RR GG BB
    u32 colorRounded{ (RoundToU32(color.a * 255.0f) << 24) | (RoundToU32(color.r * 255.0f) << 16) |
                      (RoundToU32(color.g * 255.0f) << 8) | (RoundToU32(color.b * 255.0f) << 0) };

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
        const i32 floorX{ FloorToI32(points[i].x) };
        const i32 ceilX{ CeilToI32(points[i].x) };
        const i32 floorY{ FloorToI32(points[i].y) };
        const i32 ceilY{ CeilToI32(points[i].y) };

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

    f32 xAxisLen{ Length(xAxis) };
    f32 yAxisLen{ Length(yAxis) };
    Vec2 nXCoefficient{ (yAxisLen / xAxisLen) * xAxis };
    Vec2 nYCoefficient{ (xAxisLen / yAxisLen) * yAxis };
    f32 nZScale{ 0.5f * (xAxisLen + yAxisLen) };

    const f32 originZ{};
    const f32 originY{ (origin + (0.5f * xAxis) + (0.5f * yAxis)).y };
    const f32 fixedCastY{ originY * heightMaxInv };

    u8* row{ static_cast<u8*>(buff->memory) + (minX * bitmap_Bytes_Per_Pixel) +
             (minY * buff->pitch) };

    BEGIN_TIMED_BLOCK(ProcessPixel);
    for (i32 y{ minY }; y <= maxY; ++y) {
        u32* pixel{ reinterpret_cast<u32*>(row) };
        for (i32 x{ minX }; x <= maxX; ++x) {
            //BEGIN_TIMED_BLOCK(TestPixel);

            const Vec2 pixelPos{ x, y };
            const Vec2 d{ pixelPos - origin };

            const f32 edge0{ Dot(d, -Perp(xAxis)) };
            const f32 edge1{ Dot(d - xAxis, -Perp(yAxis)) };
            const f32 edge2{ Dot(d - xAxis - yAxis, Perp(xAxis)) };
            const f32 edge3{ Dot(d - yAxis, Perp(yAxis)) };
            if ((edge0 < 0) && (edge1 < 0) && (edge2 < 0) && (edge3 < 0)) {
                //BEGIN_TIMED_BLOCK(FillPixel);

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

                    // Rotate the normals based on axises
                    normal.xy = (normal.x * nXCoefficient) + (normal.y * nYCoefficient);
                    // Compensate the size change for z
                    normal.z *= nZScale;
                    normal.xyz = Normalize(normal.xyz);

#if 1
                    // Assumed to always be {0, 0, 1}, so we can simplify
                    Vec3 bounceDir{ 2.0f * normal.z * normal.xyz };
                    bounceDir.z -= 1.0f;

                    // TODO: support top-down view and sideways
                    bounceDir.z = -bounceDir.z;

                    const Vec2 screenSpaceUV{ x * widthMaxInv, fixedCastY };
                    const f32 zDiff{ pixelsToMeters * (static_cast<f32>(y) - originY) };
                    const f32 pZ{ originZ + zDiff };

                    EnvironmentMap* farMap{};
                    const f32 tEnvMap{ bounceDir.y };
                    f32 tFarMap{};

                    if (tEnvMap < -0.5f) {
                        farMap = bottom;
                        tFarMap = -1.0f - 2.0f * tEnvMap;
                    } else if (tEnvMap > 0.5f) {
                        farMap = top;
                        tFarMap = 2.0f * (tEnvMap - 0.5f);
                    }

                    tFarMap *= tFarMap;
                    tFarMap *= tFarMap;

                    Vec3 lightColor{
                        // TODO: How do we sample from the middle map?
                    };

                    if (farMap) {
                        const f32 distanceFromMapInZ{ farMap->zPos - pZ };
                        const Vec3 farMapColor{ SampleEnvironmentMap(
                            screenSpaceUV, bounceDir, normal.w, farMap, distanceFromMapInZ) };

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

                *pixel = { (TruncToU32(blended.a + 0.5f) << 24) |
                           (TruncToU32(blended.r + 0.5f) << 16) |
                           (TruncToU32(blended.g + 0.5f) << 8) |
                           (TruncToU32(blended.b + 0.5f) << 0) };

                //END_TIMED_BLOCK(FillPixel);
            }
#else
                    *pixel = colorRounded;
#endif

            ++pixel;

            //END_TIMED_BLOCK(TestPixel);
        }

        row += buff->pitch;
    }

    END_TIMED_BLOCK_COUNTED(ProcessPixel, (maxX - minX + 1) * (maxY - minY + 1));
    END_TIMED_BLOCK(DrawRectSlowly);
}

struct Counts {
    int mm_add_ps;
    int mm_sub_ps;
    int mm_mul_ps;
    int mm_castps_si128;
    int mm_and_ps;
    int mm_or_si128;
    int mm_cmpge_ps;
    int mm_cmple_ps;
    int mm_min_ps;
    int mm_max_ps;
    int mm_cvttps_epi32;
    int mm_cvtps_epi32;
    int mm_cvtepi32_ps;
    int mm_and_si128;
    int mm_andnot_si128;
    int mm_srli_epi32;
    int mm_slli_epi32;
    int mm_sqrt_ps;
};

INTERNAL void
DrawRectQuickly(const LoadedBitmapInfo* buff, Vec2 origin, Vec2 xAxis, Vec2 yAxis, Vec4 color,
                LoadedBitmapInfo* texture, f32 pixelsToMeters) {
    BEGIN_TIMED_BLOCK(DrawRectQuickly);

    ASSERT(texture);

    // My first SIMD code :)
    //__m128 valueA{ _mm_set_ps(1.0f, 2, 3, 4) };
    //__m128 valueB{ _mm_set_ps(10, 100, 1000, 10000) };
    //__m128 sum{ _mm_add_ps(valueA, valueB) };

    // Premultiply color
    color.rgb *= color.a;
    // AA RR GG BB
    u32 colorRounded{ (RoundToU32(color.a * 255.0f) << 24) | (RoundToU32(color.r * 255.0f) << 16) |
                      (RoundToU32(color.g * 255.0f) << 8) | (RoundToU32(color.b * 255.0f) << 0) };

    // TODO: IMPORTATN: stop doing this once we have real row loading
    const i32 widthMax{ buff->width - 1 - 3 };
    const i32 heightMax{ buff->height - 1 - 3 };
    const f32 widthMaxInv{ 1.0f / widthMax };
    const f32 heightMaxInv{ 1.0f / heightMax };

    i32 minX{ widthMax };
    i32 minY{ heightMax };
    i32 maxX{};
    i32 maxY{};

#if 1
    Array<Vec2, 4> points{ origin, origin + xAxis, origin + xAxis + yAxis, origin + yAxis };
    for (i32 i{}; i < points.size; ++i) {
        const i32 floorX{ FloorToI32(points[i].x) };
        const i32 ceilX{ CeilToI32(points[i].x) };
        const i32 floorY{ FloorToI32(points[i].y) };
        const i32 ceilY{ CeilToI32(points[i].y) };

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
    if (maxX > widthMax) {
        maxX = widthMax;
    }
    if (maxY > heightMax) {
        maxY = heightMax;
    }

    const f32 xAxisLenSqInv{ 1.0f / LengthSq(xAxis) };
    const f32 yAxisLenSqInv{ 1.0f / LengthSq(yAxis) };

    f32 xAxisLen{ Length(xAxis) };
    f32 yAxisLen{ Length(yAxis) };
    Vec2 nXCoefficient{ (yAxisLen / xAxisLen) * xAxis };
    Vec2 nYCoefficient{ (xAxisLen / yAxisLen) * yAxis };
    f32 nZScale{ 0.5f * (xAxisLen + yAxisLen) };

    const Vec2 nXAxis{ xAxisLenSqInv * xAxis };
    const Vec2 nYAxis{ yAxisLenSqInv * yAxis };

    const f32 originZ{};
    const f32 originY{ (origin + (0.5f * xAxis) + (0.5f * yAxis)).y };
    const f32 fixedCastY{ originY * heightMaxInv };

    const f32 inv255{ 1.0f / 255.0f };
    const __m128 inv255x4{ _mm_set_ps1(inv255) };
    const f32 one255{ 255.0f };

    // Behold!
    __m128 mOne = _mm_set_ps1(1.0f);
    __m128 mOne255 = _mm_set_ps1(255.0f);
    __m128 mZero = _mm_set_ps1(0.0f);
    __m128 mFour = _mm_set_ps1(4);
    __m128i maskFF = _mm_set1_epi32(0xFF);
    __m128i maskFFFF = _mm_set1_epi32(0xFFFF);
    __m128i maskFF00FF = _mm_set1_epi32(0x00FF00FF);
    __m128 colorRx4 = _mm_set_ps1(color.r);
    __m128 colorGx4 = _mm_set_ps1(color.g);
    __m128 colorBx4 = _mm_set_ps1(color.b);
    __m128 colorAx4 = _mm_set_ps1(color.a);
    __m128 nXAxisXx4 = _mm_set_ps1(nXAxis.x);
    __m128 nXAxisYx4 = _mm_set_ps1(nXAxis.y);
    __m128 nYAxisXx4 = _mm_set_ps1(nYAxis.x);
    __m128 nYAxisYx4 = _mm_set_ps1(nYAxis.y);
    __m128 originXx4 = _mm_set_ps1(origin.x);
    __m128 originYx4 = _mm_set_ps1(origin.y);
    __m128 maxColorValue = _mm_set_ps1(255.0f * 255.0f);

    __m128 widthM2 = _mm_set_ps1(static_cast<f32>(texture->width - 2));
    __m128 heightM2 = _mm_set_ps1(static_cast<f32>(texture->height - 2));

    const i32 texturePitch{ texture->pitch };
    void* textureMemory{ texture->memory };

    u8* row{ static_cast<u8*>(buff->memory) + (minX * bitmap_Bytes_Per_Pixel) +
             (minY * buff->pitch) };

    BEGIN_TIMED_BLOCK(ProcessPixel);
    for (i32 y{ minY }; y <= maxY; ++y) {
        __m128 pixelPosY = _mm_set_ps1(static_cast<f32>(y));
        pixelPosY = _mm_sub_ps(pixelPosY, originYx4);

        // Incremented by 4 at the end of the loop of x
        __m128 pixelPosX = _mm_set_ps(static_cast<f32>(minX + 3), static_cast<f32>(minX + 2),
                                      static_cast<f32>(minX + 1), static_cast<f32>(minX + 0));
        pixelPosX = _mm_sub_ps(pixelPosX, originXx4);

        u32* pixel{ reinterpret_cast<u32*>(row) };
        for (i32 x{ minX }; x <= maxX; x += 4) {

            // Helper macros for some cumbersome intrinsic syntax
#define MM_SQUARE(a) _mm_mul_ps((a), (a))
#define M(a, i) (reinterpret_cast<f32*>(&(a)))[(i)]
#define Mi(a, i) (reinterpret_cast<u32*>(&(a)))[(i)]

#define COUNT_CYCLES 0

            // clang-format off
#if COUNT_CYCLES
            Counts counts = {};
#define _mm_add_ps(a, b) ++counts.mm_add_ps; a; b
#define _mm_sub_ps(a, b) ++counts.mm_sub_ps; a; b
#define _mm_mul_ps(a, b) ++counts.mm_mul_ps; a; b
#define _mm_castps_si128(a) ++counts.mm_castps_si128; a
#define _mm_and_ps(a, b) ++counts.mm_and_ps; a; b
#define _mm_or_si128(a, b) ++counts.mm_or_si128; a; b
#define _mm_cmpge_ps(a, b) ++counts.mm_cmpge_ps; a; b
#define _mm_cmple_ps(a, b) ++counts.mm_cmple_ps; a; b
#define _mm_min_ps(a, b) ++counts.mm_min_ps; a; b
#define _mm_max_ps(a, b) ++counts.mm_max_ps; a; b
#define _mm_cvttps_epi32(a) ++counts.mm_cvttps_epi32; a
#define _mm_cvtps_epi32(a) ++counts.mm_cvtps_epi32; a
#define _mm_cvtepi32_ps(a) ++counts.mm_cvtepi32_ps; a
#define _mm_and_si128(a, b) ++counts.mm_and_si128; a; b
#define _mm_andnot_si128(a, b) ++counts.mm_andnot_si128; a; b
#define _mm_srli_epi32(a, b) ++counts.mm_srli_epi32; a
#define _mm_slli_epi32(a, b) ++counts.mm_slli_epi32; a
#define _mm_sqrt_ps(a) ++counts.mm_sqrt_ps; a
#undef MM_SQUARE
#define MM_SQUARE(a) ++counts.mm_mul_ps; a
#define __m128 int
#define __m128i int

#define _mm_loadu_si128(a) 0
#define _mm_storeu_si128(a, b)
#endif
            // clang-format on

            // Using LLVM's machine code analyzer
            // We start a region with a comment in asm
            // This would not work in MSVC
#if HANDMADE_MCA && !COMPILER_MSVC
            asm volatile("# LLVM-MCA-BEGIN DrawRectQuickly" ::: "memory");
#endif

            // Iterations:        100
            // Instructions:      29300
            // Total Cycles:      8359
            // Total uOps:        29700
            // We get a Block RThroughput: 49.5 with my Zen 4 cpu
            // Not quite there yet, as 8359 / 100 = 83.6 cycles/4 pixel block
            // 83.6 / 4 = 20.9 cycles/pixel

            // Actual measured ProcessPixel in release builds:
            // MSVC:  32,205,034 / 901,420 ~= 35.7 cycles/hit
            // Clang: 18,339,490 / 901,420 ~= 20.3 cycles/hit
            // How is MSVC so much worse here?

            __m128 u =
                _mm_add_ps(_mm_mul_ps(pixelPosX, nXAxisXx4), _mm_mul_ps(pixelPosY, nXAxisYx4));
            __m128 v =
                _mm_add_ps(_mm_mul_ps(pixelPosX, nYAxisXx4), _mm_mul_ps(pixelPosY, nYAxisYx4));
            __m128i writeMask = _mm_castps_si128(
                _mm_and_ps(_mm_and_ps(_mm_cmpge_ps(u, mZero), _mm_cmple_ps(u, mOne)),
                           _mm_and_ps(_mm_cmpge_ps(v, mZero), _mm_cmple_ps(v, mOne))));

            //if (_mm_movemask_epi8(writeMask))
            {
                __m128i originalDest = _mm_loadu_si128(reinterpret_cast<__m128i*>(pixel));

                u = _mm_min_ps(_mm_max_ps(u, mZero), mOne);
                v = _mm_min_ps(_mm_max_ps(v, mZero), mOne);

                __m128 texelX = _mm_mul_ps(u, widthM2);
                __m128 texelY = _mm_mul_ps(v, heightM2);

                __m128i fetchXx4 = _mm_cvttps_epi32(texelX);
                __m128i fetchYx4 = _mm_cvttps_epi32(texelY);

                __m128 fX = _mm_sub_ps(texelX, _mm_cvtepi32_ps(fetchXx4));
                __m128 fY = _mm_sub_ps(texelY, _mm_cvtepi32_ps(fetchYx4));

                __m128i sample1;
                __m128i sample2;
                __m128i sample3;
                __m128i sample4;

#if COUNT_CYCLES
                sample1 = 0;
                sample2 = 0;
                sample3 = 0;
                sample4 = 0;
#else
                        for (i32 i{}; i < 4; ++i) {
                            const i32 fetchX = Mi(fetchXx4, i);
                            const i32 fetchY = Mi(fetchYx4, i);
                            ASSERT(fetchX >= 0 && fetchX < texture->width);
                            ASSERT(fetchY >= 0 && fetchY < texture->height);

                            // BilinearSampleFromTex
                            u8* texelPtr{ static_cast<u8*>(textureMemory) + fetchY * texturePitch +
                                          fetchX * sizeof(u32) };
                            Mi(sample1, i) = *reinterpret_cast<u32*>(texelPtr);
                            Mi(sample2, i) = *reinterpret_cast<u32*>(texelPtr + sizeof(u32));
                            Mi(sample3, i) = *reinterpret_cast<u32*>(texelPtr + texturePitch);
                            Mi(sample4, i) =
                                *reinterpret_cast<u32*>(texelPtr + texturePitch + sizeof(u32));
                        }
#endif

                // Unpack bilinear samples, shuffling
                __m128i texel1RB = _mm_and_si128(sample1, maskFF00FF);
                __m128i texel1AG = _mm_and_si128(_mm_srli_epi32(sample1, 8), maskFF00FF);
                texel1RB = _mm_mullo_epi16(texel1RB, texel1RB);
                __m128 texel1A = _mm_cvtepi32_ps(_mm_srli_epi32(texel1AG, 16));
                texel1AG = _mm_mullo_epi16(texel1AG, texel1AG);

                __m128i texel2RB = _mm_and_si128(sample2, maskFF00FF);
                __m128i texel2AG = _mm_and_si128(_mm_srli_epi32(sample2, 8), maskFF00FF);
                texel2RB = _mm_mullo_epi16(texel2RB, texel2RB);
                __m128 texel2A = _mm_cvtepi32_ps(_mm_srli_epi32(texel2AG, 16));
                texel2AG = _mm_mullo_epi16(texel2AG, texel2AG);

                __m128i texel3RB = _mm_and_si128(sample3, maskFF00FF);
                __m128i texel3AG = _mm_and_si128(_mm_srli_epi32(sample3, 8), maskFF00FF);
                texel3RB = _mm_mullo_epi16(texel3RB, texel3RB);
                __m128 texel3A = _mm_cvtepi32_ps(_mm_srli_epi32(texel3AG, 16));
                texel3AG = _mm_mullo_epi16(texel3AG, texel3AG);

                __m128i texel4RB = _mm_and_si128(sample4, maskFF00FF);
                __m128i texel4AG = _mm_and_si128(_mm_srli_epi32(sample4, 8), maskFF00FF);
                texel4RB = _mm_mullo_epi16(texel4RB, texel4RB);
                __m128 texel4A = _mm_cvtepi32_ps(_mm_srli_epi32(texel4AG, 16));
                texel4AG = _mm_mullo_epi16(texel4AG, texel4AG);

                // Load destination
                __m128 destB = _mm_cvtepi32_ps(_mm_and_si128(originalDest, maskFF));
                __m128 destG =
                    _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(originalDest, 8), maskFF));
                __m128 destR =
                    _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(originalDest, 16), maskFF));
                __m128 destA =
                    _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(originalDest, 24), maskFF));

                // Convert texture from sRGB to linear
                __m128 texel1R = _mm_cvtepi32_ps(_mm_srli_epi32(texel1RB, 16));
                __m128 texel1G = _mm_cvtepi32_ps(_mm_and_si128(texel1AG, maskFFFF));
                __m128 texel1B = _mm_cvtepi32_ps(_mm_and_si128(texel1RB, maskFFFF));

                __m128 texel2R = _mm_cvtepi32_ps(_mm_srli_epi32(texel2RB, 16));
                __m128 texel2G = _mm_cvtepi32_ps(_mm_and_si128(texel2AG, maskFFFF));
                __m128 texel2B = _mm_cvtepi32_ps(_mm_and_si128(texel2RB, maskFFFF));

                __m128 texel3R = _mm_cvtepi32_ps(_mm_srli_epi32(texel3RB, 16));
                __m128 texel3G = _mm_cvtepi32_ps(_mm_and_si128(texel3AG, maskFFFF));
                __m128 texel3B = _mm_cvtepi32_ps(_mm_and_si128(texel3RB, maskFFFF));

                __m128 texel4R = _mm_cvtepi32_ps(_mm_srli_epi32(texel4RB, 16));
                __m128 texel4G = _mm_cvtepi32_ps(_mm_and_si128(texel4AG, maskFFFF));
                __m128 texel4B = _mm_cvtepi32_ps(_mm_and_si128(texel4RB, maskFFFF));

                // Bilinear texture blend
                __m128 invfX = _mm_sub_ps(mOne, fX);
                __m128 invfY = _mm_sub_ps(mOne, fY);

                // Coefficients for lerp
                __m128 c0 = _mm_mul_ps(invfY, invfX);
                __m128 c1 = _mm_mul_ps(invfY, fX);
                __m128 c2 = _mm_mul_ps(fY, invfX);
                __m128 c3 = _mm_mul_ps(fY, fX);

                __m128 texelR =
                    _mm_add_ps(_mm_add_ps(_mm_mul_ps(c0, texel1R), _mm_mul_ps(c1, texel2R)),
                               _mm_add_ps(_mm_mul_ps(c2, texel3R), _mm_mul_ps(c3, texel4R)));
                __m128 texelG =
                    _mm_add_ps(_mm_add_ps(_mm_mul_ps(c0, texel1G), _mm_mul_ps(c1, texel2G)),
                               _mm_add_ps(_mm_mul_ps(c2, texel3G), _mm_mul_ps(c3, texel4G)));
                __m128 texelB =
                    _mm_add_ps(_mm_add_ps(_mm_mul_ps(c0, texel1B), _mm_mul_ps(c1, texel2B)),
                               _mm_add_ps(_mm_mul_ps(c2, texel3B), _mm_mul_ps(c3, texel4B)));
                __m128 texelA =
                    _mm_add_ps(_mm_add_ps(_mm_mul_ps(c0, texel1A), _mm_mul_ps(c1, texel2A)),
                               _mm_add_ps(_mm_mul_ps(c2, texel3A), _mm_mul_ps(c3, texel4A)));

                // Modulate by incoming color
                texelR = _mm_mul_ps(texelR, colorRx4);
                texelG = _mm_mul_ps(texelG, colorGx4);
                texelB = _mm_mul_ps(texelB, colorBx4);
                texelA = _mm_mul_ps(texelA, colorAx4);

                // Clamp colors
                texelR = _mm_min_ps(_mm_max_ps(texelR, mZero), maxColorValue);
                texelG = _mm_min_ps(_mm_max_ps(texelG, mZero), maxColorValue);
                texelB = _mm_min_ps(_mm_max_ps(texelB, mZero), maxColorValue);

                // Go from sRGB to linear
                destR = MM_SQUARE(destR);
                destG = MM_SQUARE(destG);
                destB = MM_SQUARE(destB);

                // Destination blend
                __m128 invTexelA = _mm_sub_ps(mOne, _mm_mul_ps(inv255x4, texelA));
                __m128 blendedR = _mm_add_ps(_mm_mul_ps(invTexelA, destR), texelR);
                __m128 blendedG = _mm_add_ps(_mm_mul_ps(invTexelA, destG), texelG);
                __m128 blendedB = _mm_add_ps(_mm_mul_ps(invTexelA, destB), texelB);
                __m128 blendedA = _mm_add_ps(_mm_mul_ps(invTexelA, destA), texelA);

                // Go from linear to sRGB
                // This is slightly faster on MSVC
#if 1
                blendedR = _mm_mul_ps(blendedR, _mm_rsqrt_ps(blendedR));
                blendedG = _mm_mul_ps(blendedG, _mm_rsqrt_ps(blendedG));
                blendedB = _mm_mul_ps(blendedB, _mm_rsqrt_ps(blendedB));
#else
                        blendedR = _mm_sqrt_ps(blendedR);
                        blendedG = _mm_sqrt_ps(blendedG);
                        blendedB = _mm_sqrt_ps(blendedB);
#endif

                // TODO: Rounding mode is nearest by default
                __m128i intR = _mm_cvtps_epi32(blendedR);
                __m128i intG = _mm_cvtps_epi32(blendedG);
                __m128i intB = _mm_cvtps_epi32(blendedB);
                __m128i intA = _mm_cvtps_epi32(blendedA);

                // Shift properly and write to pixel
                __m128i shiftedR = _mm_slli_epi32(intR, 16);
                __m128i shiftedG = _mm_slli_epi32(intG, 8);
                __m128i shiftedB = intB;
                __m128i shiftedA = _mm_slli_epi32(intA, 24);

                __m128i out = _mm_or_si128(_mm_or_si128(shiftedR, shiftedG),
                                           _mm_or_si128(shiftedB, shiftedA));

                // Mask with the write mask
                __m128i maskedOut = _mm_or_si128(_mm_and_si128(writeMask, out),
                                                 _mm_andnot_si128(writeMask, originalDest));

                // Alignment...
                _mm_storeu_si128(reinterpret_cast<__m128i*>(pixel), maskedOut);
            }

#if COUNT_CYCLES
#    undef _mm_add_ps
            f32 half = 1.0f / 2.0f;
            f32 third = 1.0f / 3.0f;

            f32 total = 0.0f;
            // clang-format off
            #define Sum(throughput, count) (throughput * static_cast<f32>(count)); total += (throughput * static_cast<f32>(count));
            // clang-format on

            f32 mm_add_ps = Sum(half, counts.mm_add_ps);
            f32 mm_sub_ps = Sum(half, counts.mm_sub_ps);
            f32 mm_mul_ps = Sum(half, counts.mm_mul_ps);
            f32 mm_and_ps = Sum(third, counts.mm_and_ps);
            f32 mm_cmpge_ps = Sum(half, counts.mm_cmpge_ps);
            f32 mm_cmple_ps = Sum(half, counts.mm_cmple_ps);
            f32 mm_min_ps = Sum(half, counts.mm_min_ps);
            f32 mm_max_ps = Sum(half, counts.mm_max_ps);
            f32 mm_castps_si128 = Sum(0, 0);
            f32 mm_or_si128 = Sum(third, counts.mm_or_si128);
            f32 mm_cvttps_epi32 = Sum(half, counts.mm_cvttps_epi32);
            f32 mm_cvtps_epi32 = Sum(half, counts.mm_cvtps_epi32);
            f32 mm_cvtepi32_ps = Sum(half, counts.mm_cvtepi32_ps);
            f32 mm_and_si128 = Sum(third, counts.mm_and_si128);
            f32 mm_andnot_si128 = Sum(third, counts.mm_andnot_si128);
            f32 mm_srli_epi32 = Sum(half, counts.mm_srli_epi32);
            f32 mm_slli_epi32 = Sum(half, counts.mm_slli_epi32);
            f32 mm_sqrt_ps = Sum(3, counts.mm_sqrt_ps);
            // For newest values in the intrinsics guide: 96.6666641
#endif

            pixelPosX = _mm_add_ps(pixelPosX, mFour);
            pixel += 4;

            // Make sure not to include these COUNT_CYCLES things withing the region
#if HANDMADE_MCA && !COMPILER_MSVC
            asm volatile("# LLVM-MCA-END DrawRectQuickly" ::: "memory");
#endif
        }

        row += buff->pitch;
    }

    END_TIMED_BLOCK_COUNTED(ProcessPixel, (maxX - minX + 1) * (maxY - minY + 1));
    END_TIMED_BLOCK(DrawRectQuickly);
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
AllocRenderGroup(MemoryArena* arena, i32 maxPushBufferSize, Vec2 resolutionPixels) {
    RenderGroup* result{ PushStruct(arena, RenderGroup) };
    result->pushBufferBase = static_cast<u8*>(PushSize(arena, maxPushBufferSize));

    result->defaultBasis = PushStruct(arena, RenderBasis);
    result->defaultBasis->pos = Vec3{};

    result->maxPushBufferSize = maxPushBufferSize;
    result->pushBufferSize = 0;

    result->globalAlpha = 1.0f;

    result->gameCamera.focalLength = 0.6f;
    result->gameCamera.cameraDistAboveTarget = 9.0f;

    result->renderCamera = result->gameCamera;
    //result->renderCamera.cameraDistAboveTarget = 30.0f;

    // TODO: adjust based on buffer size, monitor width for now in meters
    result->metersToPixels = resolutionPixels.x * 0.635f;

    const f32 pixelsToMeters{ 1.0f / result->metersToPixels };
    result->monitorHalfDimInMeters = Vec2{ 0.5 * resolutionPixels.x * pixelsToMeters,
                                           0.5 * resolutionPixels.y * pixelsToMeters };

    return result;
}

NODISCARD
INTERNAL inline Rect2
GetCameraRectAtDist(RenderGroup* group, f32 distFromCamera) {
    const Vec2 rawXY{ (distFromCamera / group->gameCamera.focalLength) *
                      group->monitorHalfDimInMeters };
    const Rect2 result{ RectCenterHalfDim(Vec2{}, rawXY) };

    return result;
}

NODISCARD
INTERNAL inline Rect2
GetCameraRectAtTarget(RenderGroup* group) {
    const Rect2 result{ GetCameraRectAtDist(group, group->gameCamera.cameraDistAboveTarget) };

    return result;
}

struct RenderEntityBasisPosResult {
    Vec2 pos;
    f32 scale;
    bool32 valid;
};

NODISCARD
INTERNAL RenderEntityBasisPosResult
GetRenderEntityBasisPos(RenderGroup* group, RenderEntityBasis* entityBasis, Vec2 screenDim) {
    RenderEntityBasisPosResult result{};

    const Vec2 screenCenter{ screenDim * 0.5f };

    const Vec3 entityBasePos{ entityBasis->basis->pos };

    const f32 depth{ group->renderCamera.cameraDistAboveTarget - entityBasePos.z };
    const f32 nearClipPlane{ 0.2f };

    const Vec3 rawXY{ entityBasePos.xy + entityBasis->offset.xy, 1.0f };
    if (depth > nearClipPlane) {
        const Vec3 projectedXY{ (1.0f / depth) * group->renderCamera.focalLength * rawXY };
        result.pos = screenCenter + (projectedXY.xy * group->metersToPixels);
        result.scale = projectedXY.z * group->metersToPixels;
        result.valid = true;
    }

    return result;
}

INTERNAL void
RenderGroupToOutput(RenderGroup* group, LoadedBitmapInfo* outputTarget, GameState* gameState) {
    // TODO: can we use something more automatic like __FUNCTION__
    // Didn't work at first try at least as __FUNCTION__ pastes a string
    BEGIN_TIMED_BLOCK(RenderGroupToOutput);

    const Vec2 screenDim{ outputTarget->width, outputTarget->height };

    // The divisor can be modified to give some zoom
    const f32 pixelsToMeters{ 1.0f / group->metersToPixels };

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

            const auto basis{ GetRenderEntityBasisPos(group, &entry->entityBasis, screenDim) };

            DrawRect(outputTarget, basis.pos, basis.pos + (entry->dim * basis.scale), entry->color);
        } break;
        case RenderGroupEntryType_RenderEntryBitmap: {
            auto* entry{ reinterpret_cast<RenderEntryBitmap*>(data) };
            baseAddress += sizeof(*entry);

            const auto basis{ GetRenderEntityBasisPos(group, &entry->entityBasis, screenDim) };

#if 0
            DrawRectSlowly(outputTarget, basis.pos, Vec2{ entry->size.x, 0 } * basis.scale,
                           Vec2{ 0, entry->size.y } * basis.scale, entry->color, entry->bitmap,
                           nullptr, nullptr, nullptr, nullptr, pixelsToMeters);
#else
            DrawRectQuickly(outputTarget, basis.pos, Vec2{ entry->size.x, 0 } * basis.scale,
                            Vec2{ 0, entry->size.y } * basis.scale, entry->color, entry->bitmap,
                            pixelsToMeters);
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
                           entry->bottom, pixelsToMeters);

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
#if 0
        if (gameState->showCollisionBoxes) {
// Don't draw for room space as it blocks the whole screen
// @Re-enable after getting reference to entity here, probably store to the piece?
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
        }
#endif
    }

    END_TIMED_BLOCK(RenderGroupToOutput);
}
