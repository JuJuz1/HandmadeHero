#include "handmade.h"

namespace game {

namespace input {

INTERNAL bool32
ActionJustPressed(const Button* button) {
    return button->endedDown && button->halfTransitionCount > 0;
}

INTERNAL bool32
ActionPressed(const Button* button) {
    return button->endedDown;
}

INTERNAL bool32
ActionReleased(const Button* button) {
    return !button->endedDown && button->halfTransitionCount > 0;
}

} //namespace input

// Write the sound data to buff
INTERNAL void
OutputSound(GameState* gameState, const SoundOutputBuffer* buff) {
    constexpr i32 toneVolume{ 3000 };
    const u32 wavePeriod{ buff->samplesPerSecond / gameState->toneHz };

    // NOTE: invalid semantics as buff is const but we copy the pointer...
    i16* sampleOut{ buff->samples };

    for (u32 i{ 0 }; i < buff->sampleCount; ++i) {
        // Sine wave
        const f32 sineValue{ sinf(gameState->tSine) };
        i16 sampleValue{ static_cast<i16>(sineValue * toneVolume) };
#if 1
        sampleValue = 0;
#endif
        *sampleOut++ = sampleValue;
        *sampleOut++ = sampleValue;

        gameState->tSine += 2 * PI32f / static_cast<f32>(wavePeriod);
        if (gameState->tSine > 2 * PI32f) {
            gameState->tSine -= 2 * PI32f;
        }
    }
}

INTERNAL void
DrawGradient(const OffScreenBuffer* screenBuff, u32 xOffset, u32 yOffset) {
    // NOTE: maybe see what the optimizer does to buff (passing by value vs pointer)
    // remember to not get fixated on micro-optimizations before actually doing optimization
    // though...

    // Pitch (length width-wise)
    u8* row{ static_cast<u8*>(screenBuff->memory) };

    // Drawing
    for (i32 y{ 0 }; y < screenBuff->height; ++y) {
        u32* pixel{ reinterpret_cast<u32*>(row) };
        for (i32 x{ 0 }; x < screenBuff->width; ++x) {
            // Windows flipped the order of rbg
            // Memory: BB GG RR xx
            // !little endianness!

            const u8 blue{ static_cast<u8>(x + xOffset) };
            const u8 green{ static_cast<u8>(y + yOffset) };

            // Register: xx RR GG BB
            *pixel++ = (green << 8) | blue;
        }

        row += screenBuff->pitch;
    }
}

INTERNAL void
RenderPlayer(const OffScreenBuffer* screenBuff, u32 playerPosX, u32 playerPosY, u32 color) {
    constexpr u32 playerDimension{ 16 }; // Width and height

    u8* memory{ static_cast<u8*>(screenBuff->memory) };

    u8* row{ memory + (playerPosY * screenBuff->pitch) };
    const u8* buffEnd{ memory + (screenBuff->pitch * screenBuff->height) };

    for (u32 y{}; y < playerDimension; ++y) {
        u8* pixel{ row + (playerPosX * screenBuff->bytesPerPixel) };
        for (u32 x{}; x < playerDimension; ++x) {
            if (pixel >= screenBuff->memory && pixel < buffEnd) {
                *reinterpret_cast<u32*>(pixel) = color;
                pixel += screenBuff->bytesPerPixel;
            }
        }

        row += screenBuff->pitch;
    }
}

// NOTE: use extern "C" to avoid name mangling
extern "C" GET_SOUND_SAMPLES(GetSoundSamples) {
    GameState* gameState{ static_cast<GameState*>(memory->permanentStorage) };
    OutputSound(gameState, soundBuff);
}

extern "C" UPDATE_AND_RENDER(UpdateAndRender) {
    ASSERT(sizeof(GameState) <= memory->permanentStorageSize);
    // NOTE: this macro depends on the order of the buttons inside InputButtons
    ASSERT(&input->playerInputs[0].E - &input->playerInputs[0].buttons[0] ==
           ARRAY_COUNT(input->playerInputs[0].buttons) - 1);

    GameState* gameState{ static_cast<GameState*>(memory->permanentStorage) };
    if (!memory->isInitialized) {
        // The memory is already zeroed
        //gameState->xOffset = 0;
        //gameState->yOffset = 0;
        gameState->toneHz = 256;
        //gameState->tSine = 0;
        gameState->playerPosX = 200;
        gameState->playerPosY = 100;

        const char* fileName{ __FILE__ };
        const auto readResult{ memory->DEBUGReadFile(fileName) };
        if (readResult.content) {
            memory->DEBUGWriteFile("test.out", readResult.content, readResult.contentSize);
            memory->DEBUGFreeFileMemory(readResult.content);
        }

        // TODO: maybe make platform set this
        memory->isInitialized = true;
    }

    // NOTE: if many players -> loop through input->playerInputs
    const InputButtons* input0{ &input->playerInputs[0] };

    // Input (Godot style)
    //if Input.just_pressed("A")  <==> endedDown && halfTransitionCount > 0
    //if Input.pressed("A")       <==> endedDown
    //if Input.just_released("A") <==> !endedDown && halfTransitionCount > 0
    // Managed to get the same functionality done!

    constexpr u32 gradientOffset{ 5 };

    u32 playerVelocityX{}, playerVelocityY{};
    constexpr u32 playerMoveAmount{ 5 };
    constexpr u32 playerMoveShiftModifier{ 3 };

    if (input::ActionPressed(&input0->up)) {
        gameState->yOffset -= gradientOffset;
        playerVelocityY -= playerMoveAmount;
    }
    if (input::ActionPressed(&input0->down)) {
        gameState->yOffset += gradientOffset;
        playerVelocityY += playerMoveAmount;
    }
    if (input::ActionPressed(&input0->left)) {
        gameState->xOffset -= gradientOffset;
        playerVelocityX -= playerMoveAmount;
    }
    if (input::ActionPressed(&input0->right)) {
        playerVelocityX += playerMoveAmount;
    }
    // Shift
    if (input::ActionPressed(&input0->shift)) {
        playerVelocityX *= playerMoveShiftModifier;
        playerVelocityY *= playerMoveShiftModifier;
    }

    gameState->playerPosX += playerVelocityX;
    gameState->playerPosY += playerVelocityY;

    constexpr u32 toneHzOffset{ 30 };
    //memory->DEBUGPrintInt("toneHz", gameState->toneHz);

    if (input::ActionJustPressed(&input0->E)) {
        gameState->toneHz += toneHzOffset;
    } else if (input::ActionJustPressed(&input0->Q)) {
        if (gameState->toneHz > toneHzOffset) {
            gameState->toneHz -= toneHzOffset;
        } else {
            gameState->toneHz = 1;
        }
    }

    // TODO: allow sample offsets
    //OutputSound(soundBuff, gameState->toneHz);

    DrawGradient(screenBuff, gameState->xOffset, gameState->yOffset);
    RenderPlayer(screenBuff, gameState->playerPosX, gameState->playerPosY, 0xFFFFFFFF);
}

} //namespace game
