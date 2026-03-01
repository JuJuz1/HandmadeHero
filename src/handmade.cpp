#include "handmade.h"

INTERNAL void
GameOutputSound(const GameSoundOutputBuffer* buff) {
    LOCAL_PERSIST f32 tSine;
    constexpr i32 toneVolume{ 3000 };
    constexpr u32 toneHz{ 256 };
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
DrawGradient(const GameOffScreenBuffer* buff, u32 xOffset, u32 yOffset) {
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
GameUpdateAndRender(const GameOffScreenBuffer* buff, u32 xOffset, u32 yOffset,
                    const GameSoundOutputBuffer* soundBuff) {
    // TODO: allow sample offsets
    GameOutputSound(soundBuff);

    DrawGradient(buff, xOffset, yOffset);
}
