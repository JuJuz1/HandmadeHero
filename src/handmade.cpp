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
OutputSound(GameState* gameState, const SoundOutputBuffer* buff, u32 toneHz) {
    constexpr i32 toneVolume{ 3000 };
    const u32 wavePeriod{ buff->samplesPerSecond / toneHz };

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

        //gameState->tSine += 2 * PI32f / static_cast<f32>(wavePeriod);
        //if (gameState->tSine > 2 * PI32f) {
        //    gameState->tSine -= 2 * PI32f;
        //}
    }
}

INTERNAL void
DrawRectangle(const OffScreenBuffer* screenBuff, f32 minX, f32 minY, f32 maxX, f32 maxY, f32 r,
              f32 g, f32 b) {
    i32 roundedMinX{ RoundF32ToI32(minX) };
    i32 roundedMinY{ RoundF32ToI32(minY) };
    i32 roundedMaxX{ RoundF32ToI32(maxX) };
    i32 roundedMaxY{ RoundF32ToI32(maxY) };

    if (roundedMinX < 0) {
        roundedMinX = 0;
    }
    if (roundedMinY < 0) {
        roundedMinY = 0;
    }

    // Not including fill pixel
    if (roundedMaxX > screenBuff->width) {
        roundedMaxX = screenBuff->width;
    }
    if (roundedMaxY > screenBuff->height) {
        roundedMaxY = screenBuff->height;
    }

    // AA RR GG BB
    const u32 color{ (RoundF32ToU32(r * 255.0f) << 16) | (RoundF32ToU32(g * 255.0f) << 8) |
                     (RoundF32ToU32(b * 255.0f) << 0) };

    u8* memory{ static_cast<u8*>(screenBuff->memory) };
    u8* row{ memory + (roundedMinX * screenBuff->bytesPerPixel) +
             (roundedMinY * screenBuff->pitch) };

    for (i32 y{ roundedMinY }; y < roundedMaxY; ++y) {
        u32* pixel{ reinterpret_cast<u32*>(row) };
        for (i32 x{ roundedMinX }; x < roundedMaxX; ++x) {
            *pixel++ = color;
        }

        row += screenBuff->pitch;
    }
}

INTERNAL void
InitializeGameState(GameState* gameState, GameMemory* memory, ThreadContext* threadContext) {
    // TODO: maybe make platform set this
    memory->isInitialized = true;

    gameState->playerPosX = 100.0f;
    gameState->playerPosY = 200.0f;
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

    const f32 delta{ input->frameDeltaTime };

    // NOTE: if many players -> loop through input->playerInputs
    const InputButtons* input0Keyboard{ &input->playerInputs[0] };

    f32 playerVelocityX{}, playerVelocityY{};
    constexpr f32 playerSpeed{ 200 }; // Pixels per second
    if (input::ActionPressed(&input0Keyboard->up)) {
        playerVelocityY -= playerSpeed;
    }
    if (input::ActionPressed(&input0Keyboard->down)) {
        playerVelocityY += playerSpeed;
    }
    if (input::ActionPressed(&input0Keyboard->left)) {
        playerVelocityX -= playerSpeed;
    }
    if (input::ActionPressed(&input0Keyboard->right)) {
        playerVelocityX += playerSpeed;
    }

    gameState->playerPosX += playerVelocityX * delta;
    gameState->playerPosY += playerVelocityY * delta;

    DrawRectangle(screenBuff, 0.0f, 0.0f, static_cast<f32>(screenBuff->width),
                  static_cast<f32>(screenBuff->height), 0.0f, 0.1f, 0.2f);

    u32 tilemap[9][17]{ { 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1 },
                        { 1, 1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1 },
                        { 1, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 1 },
                        { 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1 },
                        { 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0 },
                        { 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1 },
                        { 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1 },
                        { 1, 1, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 1 },
                        { 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1 } };

    constexpr f32 upperLeftX{ -30 };
    constexpr f32 upperLeftY{ 0 };
    constexpr u32 tileWidth{ 60 };
    constexpr u32 tileHeight{ 60 };

    // sizeof(tilemap) / sizeof(tilemap[0])
    for (u32 row{}; row < 9; ++row) {
        for (u32 column{}; column < 17; ++column) {
            const u32 tileID{ tilemap[row][column] };
            f32 gray{ tileID ? 1.0f : 0.5f };

            const f32 minX{ upperLeftX + (static_cast<f32>(column) * tileWidth) };
            const f32 minY{ upperLeftY + (static_cast<f32>(row) * tileHeight) };
            const f32 maxX{ minX + tileWidth };
            const f32 maxY{ minY + tileHeight };

            DrawRectangle(screenBuff, minX, minY, maxX, maxY, .1f, gray, gray);
        }
    }

    // Drawing player
    constexpr f32 playerR{ .5f };
    constexpr f32 playerG{ .1f };
    constexpr f32 playerB{ .5f };

    constexpr f32 playerWidth{ tileWidth * 0.75f };
    constexpr f32 playerHeight{ tileHeight * 0.9f };

    const f32 playerPosLeft{ gameState->playerPosX - (0.5f * playerWidth) };
    const f32 playerPosTop{ gameState->playerPosY - playerHeight };

    DrawRectangle(screenBuff, playerPosLeft, playerPosTop, playerPosLeft + playerWidth,
                  gameState->playerPosY, playerR, playerG, playerB);
}

// NOTE: use extern "C" to avoid name mangling
extern "C" GET_SOUND_SAMPLES(GetSoundSamples) {
    UNUSED_PARAMS(threadContext);

    GameState* gameState{ static_cast<GameState*>(memory->permanentStorage) };
    OutputSound(gameState, soundBuff, 256);
}

} //namespace game
