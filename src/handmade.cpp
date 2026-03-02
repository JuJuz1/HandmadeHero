#include "handmade.h"

namespace game {

INTERNAL void
OutputSound(const SoundOutputBuffer* buff, u32 toneHz) {
    LOCAL_PERSIST f32 tSine;
    constexpr i32 toneVolume{ 3000 };
    const u32 wavePeriod{ buff->samplesPerSecond / toneHz };

    i16* sampleOut{ buff->samples };

    for (u32 i{ 0 }; i < buff->sampleCount; ++i) {
        // Sine wave
        const f32 sineValue{ sinf(tSine) };
        const i16 sampleValue{ static_cast<i16>(sineValue * toneVolume) };

        *sampleOut++ = sampleValue;
        *sampleOut++ = sampleValue;

        tSine += 2 * PI32 / static_cast<f32>(wavePeriod);
    }
}

INTERNAL void
DrawGradient(const OffScreenBuffer* buff, u32 xOffset, u32 yOffset) {
    // NOTE: maybe see what the optimizer does to buff (passing by value vs pointer)
    // remember to not get fixated on micro-optimizations before actually doing optimization
    // though...

    // Pitch (length width-wise)
    u8* row{ static_cast<u8*>(buff->memory) };

    // Drawing
    for (i32 y{ 0 }; y < buff->height; ++y) {
        u32* pixel{ reinterpret_cast<u32*>(row) };
        for (i32 x{ 0 }; x < buff->width; ++x) {
            // Windows flipped the order of rbg
            // Memory: BB GG RR xx
            // !little endianness!

            u8 blue{ static_cast<u8>(x + xOffset) };
            u8 green{ static_cast<u8>(y + yOffset) };

            // Register: xx RR GG BB
            *pixel++ = ((green << 8) | blue);
        }

        row += buff->pitch;
    }
}

INTERNAL void
UpdateAndRender(const OffScreenBuffer* buff, const SoundOutputBuffer* soundBuff,
                const Input* input) {
    LOCAL_PERSIST u32 xOffset{};
    LOCAL_PERSIST u32 yOffset{};
    LOCAL_PERSIST u32 toneHz{ 256 };

    const InputButtons* input0{ &input->playerInputs[0] };
    // Input (Godot style)
    //if Input.just_pressed("A")
    //if Input.just_released("A")
    //if Input.pressed("A")

    // Controllers:
    //Input.AButtonEndedDown
    //Input.AButtonHalfTransitionCount

    constexpr u32 offset{ 10 };
    if (input0->up.endedDown) {
        yOffset -= offset;
    }
    if (input0->down.endedDown) {
        yOffset += offset;
    }
    if (input0->left.endedDown) {
        xOffset -= offset;
    }
    if (input0->right.endedDown) {
        xOffset += offset;
    }

    // TODO: allow sample offsets
    OutputSound(soundBuff, toneHz);

    DrawGradient(buff, xOffset, yOffset);
    //++xOffset;
    //++yOffset;
}

} //namespace game
