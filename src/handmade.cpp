#include "handmade.h"

namespace game {

// Write the sound data to buff
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
UpdateAndRender(GameMemory* memory, const OffScreenBuffer* buff, const SoundOutputBuffer* soundBuff,
                const Input* input) {

    ASSERT(sizeof(GameState) <= memory->permanentStorageSize);

    GameState* gameState{ static_cast<GameState*>(memory->permanentStorage) };
    if (!memory->isInitialized) {
        // The memory is already zeroed
        //gameState->xOffset = 0;
        //gameState->yOffset = 0;
        gameState->toneHz = 256;

        const char* fileName{ __FILE__ };
        platform::DEBUGFileReadResult readResult{ platform::DEBUGPlatformReadFile(fileName) };
        if (readResult.content) {
            platform::DEBUGPlatformWriteFile("test.out", readResult.content,
                                             readResult.contentSize);
            platform::DEBUGPlatformFreeFileMemory(readResult.content);
        }

        // TODO: maybe make platform set this
        memory->isInitialized = true;
    }

    const InputButtons* input0{ &input->playerInputs[0] };
    // Input (Godot style)
    //if Input.just_pressed("A")
    //if Input.just_released("A")
    //if Input.pressed("A")

    // Controllers:
    //Input.AButtonEndedDown
    //Input.AButtonHalfTransitionCount

    constexpr u32 offset{ 10 };
    //if (input0->up.justPressed)
    if (input0->up.released) {
        gameState->yOffset -= offset;
    }
    if (input0->down.released) {
        gameState->yOffset += offset;
    }
    if (input0->left.pressed) {
        gameState->xOffset -= offset;
    }
    if (input0->right.pressed) {
        gameState->xOffset += offset;
    }

    // TODO: allow sample offsets
    OutputSound(soundBuff, gameState->toneHz);

    DrawGradient(buff, gameState->xOffset, gameState->yOffset);
    //++xOffset;
    //++yOffset;
}

} //namespace game
