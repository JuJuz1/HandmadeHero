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

INTERNAL bool32
IsTilemapPointEmpty(const TilemapData* tilemapData, f32 x, f32 y) {
    // Move to world coordinates by subtracting the possible screen offsets
    const i32 playerTileX{ TruncateF32ToI32((x - tilemapData->upperLeftX) /
                                            static_cast<i32>(tilemapData->tileWidth)) };
    const i32 playerTileY{ TruncateF32ToI32((y - tilemapData->upperLeftY) /
                                            static_cast<i32>(tilemapData->tileHeight)) };

    bool32 isValid{};
    if (playerTileX >= 0 && playerTileX < static_cast<i32>(tilemapData->columns) &&
        playerTileY >= 0 && playerTileY < static_cast<i32>(tilemapData->rows)) {
        const u32 tilemapValue{
            tilemapData->tiles[(tilemapData->columns * playerTileY) + playerTileX]
        };
        isValid = (tilemapValue == 0);
    }

    return isValid;
}

extern "C" UPDATE_AND_RENDER(UpdateAndRender) {
    ASSERT(sizeof(GameState) <= memory->permanentStorageSize);
    // NOTE: this macro depends on the order of the buttons inside InputButtons
    // Already used static_assert in handmade.h to handle this, but let it be asserted here as well
    ASSERT(&input->playerInputs[0].E - &input->playerInputs[0].buttons[0] ==
           ARRAY_COUNT(input->playerInputs[0].buttons) - 1);
    ASSERT(&input->mouseButtons.x2 - &input->mouseButtons.buttons[0] ==
           ARRAY_COUNT(input->mouseButtons.buttons) - 1);

    GameState* gameState{ static_cast<GameState*>(memory->permanentStorage) };
    if (!memory->isInitialized) {
        InitializeGameState(gameState, memory, threadContext);
    }

    constexpr u32 tileMapRows{ 9 };
    constexpr u32 tileMapColumns{ 17 };
    u32 tilemapData[tileMapRows][tileMapColumns]{
        { 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1 },
        { 1, 1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1 },
        { 1, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 1 },
        { 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1 },
        { 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0 },
        { 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1 },
        { 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1 },
        { 1, 1, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 1 },
        { 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1 }
    };

    TilemapData tilemap{};

    tilemap.tiles = reinterpret_cast<u32*>(tilemapData);
    tilemap.rows = tileMapRows;
    tilemap.columns = tileMapColumns;

    tilemap.upperLeftX = -30; // Move half tileWidth right
    tilemap.upperLeftY = 0;
    tilemap.tileWidth = 60;
    tilemap.tileHeight = 60;

    const f32 delta{ input->frameDeltaTime };

    // NOTE: if many players -> loop through input->playerInputs
    const InputButtons* input0Keyboard{ &input->playerInputs[0] };

    f32 playerVelocityX{}, playerVelocityY{};
    constexpr f32 playerSpeed{ 150 }; // Pixels per second
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

    const f32 newPlayerX{ gameState->playerPosX + (playerVelocityX * delta) };
    const f32 newPlayerY{ gameState->playerPosY + (playerVelocityY * delta) };

    if (IsTilemapPointEmpty(&tilemap, newPlayerX, newPlayerY)) {
        gameState->playerPosX = newPlayerX;
        gameState->playerPosY = newPlayerY;
    }

    memory->DEBUGPrintFloat(threadContext, "playerX", gameState->playerPosX);
    memory->DEBUGPrintFloat(threadContext, "playerY", gameState->playerPosY);

    // Background
    DrawRectangle(screenBuff, 0.0f, 0.0f, static_cast<f32>(screenBuff->width),
                  static_cast<f32>(screenBuff->height), 0.0f, 0.1f, 0.2f);

    // sizeof(tilemap) / sizeof(tilemap[0])
    for (u32 row{}; row < tilemap.rows; ++row) {
        for (u32 column{}; column < tilemap.columns; ++column) {
            const u32 tileID{ tilemap.tiles[row * tilemap.columns + column] };
            f32 gray{ tileID ? 1.0f : 0.5f };

            const f32 minX{ tilemap.upperLeftX + (static_cast<f32>(column) * tilemap.tileWidth) };
            const f32 minY{ tilemap.upperLeftY + (static_cast<f32>(row) * tilemap.tileHeight) };
            const f32 maxX{ minX + tilemap.tileWidth };
            const f32 maxY{ minY + tilemap.tileHeight };

            DrawRectangle(screenBuff, minX, minY, maxX, maxY, .1f, gray, gray);
        }
    }

    // Drawing player
    constexpr f32 playerR{ 0.5f };
    constexpr f32 playerG{ 0.1f };
    constexpr f32 playerB{ 0.5f };

    const f32 playerWidth{ tilemap.tileWidth * 0.75f };
    const f32 playerHeight{ tilemap.tileHeight * 0.9f };

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
