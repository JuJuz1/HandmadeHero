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

    const i32 lodIndex{ RoundF32ToI32(roughness * static_cast<f32>(map->lod.size - 1)) };
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
               EnvironmentMap* middle, EnvironmentMap* bottom, f32 pixelsToMeters) {
    BEGIN_TIMED_BLOCK(DrawRectSlowly);

    ASSERT(texture);

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

    for (i32 y{ minY }; y <= maxY; ++y) {
        u32* pixel{ reinterpret_cast<u32*>(row) };
        for (i32 x{ minX }; x <= maxX; ++x) {
            BEGIN_TIMED_BLOCK(TestPixel);

            const Vec2 pixelPos{ x, y };
            const Vec2 d{ pixelPos - origin };

            const f32 edge0{ Dot(d, -Perp(xAxis)) };
            const f32 edge1{ Dot(d - xAxis, -Perp(yAxis)) };
            const f32 edge2{ Dot(d - xAxis - yAxis, Perp(xAxis)) };
            const f32 edge3{ Dot(d - yAxis, Perp(yAxis)) };
            if ((edge0 < 0) && (edge1 < 0) && (edge2 < 0) && (edge3 < 0)) {
                BEGIN_TIMED_BLOCK(FillPixel);

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

                *pixel = { (TruncateF32ToU32(blended.a + 0.5f) << 24) |
                           (TruncateF32ToU32(blended.r + 0.5f) << 16) |
                           (TruncateF32ToU32(blended.g + 0.5f) << 8) |
                           (TruncateF32ToU32(blended.b + 0.5f) << 0) };

                END_TIMED_BLOCK(FillPixel);
            }
#else
                    *pixel = colorRounded;
#endif

            ++pixel;

            END_TIMED_BLOCK(TestPixel);
        }

        row += buff->pitch;
    }

    END_TIMED_BLOCK(DrawRectSlowly);
}

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
    u32 colorRounded{ (RoundF32ToU32(color.a * 255.0f) << 24) |
                      (RoundF32ToU32(color.r * 255.0f) << 16) |
                      (RoundF32ToU32(color.g * 255.0f) << 8) |
                      (RoundF32ToU32(color.b * 255.0f) << 0) };

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
    const f32 one255{ 255.0f };

    u8* row{ static_cast<u8*>(buff->memory) + (minX * bitmap_Bytes_Per_Pixel) +
             (minY * buff->pitch) };

    for (i32 y{ minY }; y <= maxY; ++y) {
        u32* pixel{ reinterpret_cast<u32*>(row) };
        for (i32 x{ minX }; x <= maxX; x += 4) {
            BEGIN_TIMED_BLOCK(TestPixel);

            // A bit nasty
            f32 texel1R[4];
            f32 texel1G[4];
            f32 texel1B[4];
            f32 texel1A[4];

            f32 texel2R[4];
            f32 texel2G[4];
            f32 texel2B[4];
            f32 texel2A[4];

            f32 texel3R[4];
            f32 texel3G[4];
            f32 texel3B[4];
            f32 texel3A[4];

            f32 texel4R[4];
            f32 texel4G[4];
            f32 texel4B[4];
            f32 texel4A[4];

            // Load destination
            f32 destR[4];
            f32 destG[4];
            f32 destB[4];
            f32 destA[4];

            f32 blendedR[4];
            f32 blendedG[4];
            f32 blendedB[4];
            f32 blendedA[4];

            f32 fX[4];
            f32 fY[4];

            bool32 shouldFill[4];

            for (i32 i{}; i < 4; ++i) {
                const Vec2 pixelPos{ x + i, y };
                const Vec2 d{ pixelPos - origin };

                const f32 u{ Dot(d, nXAxis) };
                const f32 v{ Dot(d, nYAxis) };

                shouldFill[i] = ((u >= 0.0f) && (u <= 1.0f) && (v >= 0.0f) && (v <= 1.0f));

                if (shouldFill[i]) {
                    // Pretend the texture is 1 pixel smaller in both dimensions
                    const f32 texelX{ u * static_cast<f32>(texture->width - 2) };
                    const f32 texelY{ v * static_cast<f32>(texture->height - 2) };

                    const i32 roundedX{ static_cast<i32>(texelX) };
                    const i32 roundedY{ static_cast<i32>(texelY) };
                    ASSERT(roundedX >= 0 && roundedX < texture->width);
                    ASSERT(roundedY >= 0 && roundedY < texture->height);

                    fX[i] = static_cast<f32>(texelX - roundedX);
                    fY[i] = static_cast<f32>(texelY - roundedY);

                    // BilinearSampleFromTex
                    u8* texelPtr{ static_cast<u8*>(texture->memory) + roundedY * texture->pitch +
                                  roundedX * sizeof(u32) };
                    u32 sample1{ *reinterpret_cast<u32*>(texelPtr) };
                    u32 sample2{ *reinterpret_cast<u32*>(texelPtr + sizeof(u32)) };
                    u32 sample3{ *reinterpret_cast<u32*>(texelPtr + texture->pitch) };
                    u32 sample4{ *reinterpret_cast<u32*>(texelPtr + texture->pitch + sizeof(u32)) };

                    // SRGBBilinearBlend, unpacks
                    texel1R[i] = static_cast<f32>((sample1 >> 16) & 0xFF);
                    texel1G[i] = static_cast<f32>((sample1 >> 8) & 0xFF);
                    texel1B[i] = static_cast<f32>((sample1 >> 0) & 0xFF);
                    texel1A[i] = static_cast<f32>((sample1 >> 24) & 0xFF);

                    texel2R[i] = static_cast<f32>((sample2 >> 16) & 0xFF);
                    texel2G[i] = static_cast<f32>((sample2 >> 8) & 0xFF);
                    texel2B[i] = static_cast<f32>((sample2 >> 0) & 0xFF);
                    texel2A[i] = static_cast<f32>((sample2 >> 24) & 0xFF);

                    texel3R[i] = static_cast<f32>((sample3 >> 16) & 0xFF);
                    texel3G[i] = static_cast<f32>((sample3 >> 8) & 0xFF);
                    texel3B[i] = static_cast<f32>((sample3 >> 0) & 0xFF);
                    texel3A[i] = static_cast<f32>((sample3 >> 24) & 0xFF);

                    texel4R[i] = static_cast<f32>((sample4 >> 16) & 0xFF);
                    texel4G[i] = static_cast<f32>((sample4 >> 8) & 0xFF);
                    texel4B[i] = static_cast<f32>((sample4 >> 0) & 0xFF);
                    texel4A[i] = static_cast<f32>((sample4 >> 24) & 0xFF);

                    // Load destination
                    // Basically Unpack4x8, flattened
                    destR[i] = static_cast<f32>((*(pixel + i) >> 16) & 0xFF);
                    destG[i] = static_cast<f32>((*(pixel + i) >> 8) & 0xFF);
                    destB[i] = static_cast<f32>((*(pixel + i) >> 0) & 0xFF);
                    destA[i] = static_cast<f32>((*(pixel + i) >> 24) & 0xFF);
                }
            }

            for (i32 i{}; i < 4; ++i) {
                // Convert texture from sRGB to linear
                //texel1R = Square(texel1R * inv255);
                //texel1G = Square(texel1G * inv255);
                //texel1B = Square(texel1B * inv255);
                //texel1A = texel1A * inv255;

                // Flattened
                texel1R[i] = texel1R[i] * inv255;
                texel1R[i] *= texel1R[i];
                texel1G[i] = texel1G[i] * inv255;
                texel1G[i] *= texel1G[i];
                texel1B[i] = texel1B[i] * inv255;
                texel1B[i] *= texel1B[i];
                texel1A[i] = texel1A[i] * inv255;
                //

                texel2R[i] = Square(texel2R[i] * inv255);
                texel2G[i] = Square(texel2G[i] * inv255);
                texel2B[i] = Square(texel2B[i] * inv255);
                texel2A[i] = texel2A[i] * inv255;

                texel3R[i] = Square(texel3R[i] * inv255);
                texel3G[i] = Square(texel3G[i] * inv255);
                texel3B[i] = Square(texel3B[i] * inv255);
                texel3A[i] = texel3A[i] * inv255;

                texel4R[i] = Square(texel4R[i] * inv255);
                texel4G[i] = Square(texel4G[i] * inv255);
                texel4B[i] = Square(texel4B[i] * inv255);
                texel4A[i] = texel4A[i] * inv255;

                // Bilinear texture blend
                f32 invfX{ 1.0f - fX[i] };
                f32 invfY{ 1.0f - fY[i] };

                // Coefficients for lerp
                f32 c0{ invfY * invfX };
                f32 c1{ invfY * fX[i] };
                f32 c2{ fY[i] * invfX };
                f32 c3{ fY[i] * fX[i] };

                f32 texelR{ c0 * texel1R[i] + c1 * texel2R[i] + c2 * texel3R[i] + c3 * texel4R[i] };
                f32 texelG{ c0 * texel1G[i] + c1 * texel2G[i] + c2 * texel3G[i] + c3 * texel4G[i] };
                f32 texelB{ c0 * texel1B[i] + c1 * texel2B[i] + c2 * texel3B[i] + c3 * texel4B[i] };
                f32 texelA{ c0 * texel1A[i] + c1 * texel2A[i] + c2 * texel3A[i] + c3 * texel4A[i] };

                // Modulate by incoming color
                texelR = texelR * color.r;
                texelG = texelG * color.g;
                texelB = texelB * color.b;
                texelA = texelA * color.a;

                // Clamp colors
                texelR = Clamp01(texelR);
                texelG = Clamp01(texelG);
                texelB = Clamp01(texelB);
                //texel.a = Clamp01(texel.a);

                // Go from sRGB to linear
                destR[i] = Square(destR[i] * inv255);
                destG[i] = Square(destG[i] * inv255);
                destB[i] = Square(destB[i] * inv255);
                destA[i] = destA[i] * inv255;

                // Destination blend
                f32 invTexelA{ 1.0f - texelA };
                blendedR[i] = (destR[i] * invTexelA) + texelR;
                blendedG[i] = (destG[i] * invTexelA) + texelG;
                blendedB[i] = (destB[i] * invTexelA) + texelB;
                blendedA[i] = (destA[i] * invTexelA) + texelA;

                // Go from linear to sRGB
                blendedR[i] = Sqrt(blendedR[i]) * one255;
                blendedG[i] = Sqrt(blendedG[i]) * one255;
                blendedB[i] = Sqrt(blendedB[i]) * one255;
                blendedA[i] = blendedA[i] * one255;
            }

            for (i32 i{}; i < 4; ++i) {
                if (shouldFill[i]) {
                    *(pixel + i) = { (TruncateF32ToU32(blendedA[i] + 0.5f) << 24) |
                                     (TruncateF32ToU32(blendedR[i] + 0.5f) << 16) |
                                     (TruncateF32ToU32(blendedG[i] + 0.5f) << 8) |
                                     (TruncateF32ToU32(blendedB[i] + 0.5f) << 0) };
                }
            }

            pixel += 4;

            END_TIMED_BLOCK(TestPixel);
        }

        row += buff->pitch;
    }

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
