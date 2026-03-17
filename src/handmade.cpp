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
#if 1
        const i16 sampleValue{};
#else
        const f32 sineValue{ sinf(gameState->tSine) };
        const i16 sampleValue{ static_cast<i16>(sineValue * toneVolume) };
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

INTERNAL void
InitializeGameState(GameState* gameState, GameMemory* memory, ThreadContext* threadContext) {
    // TODO: maybe make platform set this
    memory->isInitialized = true;
}

extern "C" UPDATE_AND_RENDER(UpdateAndRender) {
    ASSERT(sizeof(GameState) <= memory->permanentStorageSize);
    // NOTE: this macro depends on the order of the buttons inside InputButtons
    ASSERT(&input->playerInputs[0].E - &input->playerInputs[0].buttons[0] ==
           ARRAY_COUNT(input->playerInputs[0].buttons) - 1);
    ASSERT(&input->mouseButtons.x2 - &input->mouseButtons.buttons[0] ==
           ARRAY_COUNT(input->mouseButtons.buttons) - 1);

    GameState* gameState{ static_cast<GameState*>(memory->permanentStorage) };
    if (!memory->isInitialized) {
        InitializeGameState(gameState, memory, threadContext);
    }

    const f32 delta{ memory->frameDeltaTime };

    // NOTE: if many players -> loop through input->playerInputs
    const InputButtons* input0Keyboard{ &input->playerInputs[0] };
}

// NOTE: use extern "C" to avoid name mangling
extern "C" GET_SOUND_SAMPLES(GetSoundSamples) {
    UNUSED_PARAMS(threadContext);

    GameState* gameState{ static_cast<GameState*>(memory->permanentStorage) };
    OutputSound(gameState, soundBuff);
}

} //namespace game
