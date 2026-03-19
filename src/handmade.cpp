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

    gameState->playerPosX = 150.0f;
    gameState->playerPosY = 200.0f;
}

INTERNAL Tilemap*
GetTileMap(const World* world, i32 tilemapX, i32 tilemapY) {
    Tilemap* tilemap{};

    if (tilemapX >= 0 && tilemapX < static_cast<i32>(world->tilemapColumns) && tilemapY >= 0 &&
        tilemapY < static_cast<i32>(world->tilemapRows)) {
        tilemap = &world->tilemaps[(world->tilemapRows * tilemapY) + tilemapX];
    }

    return tilemap;
}

INTERNAL inline u32
GetTilemapValue(const Tilemap* tilemap, i32 tileX, i32 tileY) {
    // NOTE: no checks
    const u32 tilemapValue{ tilemap->tiles[(tilemap->columns * tileY) + tileX] };
    return tilemapValue;
}

INTERNAL bool32
IsTilemapPointEmpty(const Tilemap* tilemap, f32 x, f32 y) {
    bool32 isValid{};

    // Move to world coordinates by subtracting the possible rendering offsets
    const i32 playerTileX{ TruncateF32ToI32((x - tilemap->upperLeftX) /
                                            static_cast<i32>(tilemap->tileWidth)) };
    const i32 playerTileY{ TruncateF32ToI32((y - tilemap->upperLeftY) /
                                            static_cast<i32>(tilemap->tileHeight)) };
    if (playerTileX >= 0 && playerTileX < static_cast<i32>(tilemap->columns) && playerTileY >= 0 &&
        playerTileY < static_cast<i32>(tilemap->rows)) {
        const u32 tilemapValue{ GetTilemapValue(tilemap, playerTileX, playerTileY) };
        isValid = (tilemapValue == 0);
    }

    return isValid;
}

INTERNAL bool32
IsWorldPointEmpty(const World* world, i32 tilemapX, i32 tilemapY, f32 x, f32 y) {
    bool32 isValid{};
    const Tilemap* tilemap{ GetTileMap(world, tilemapX, tilemapY) };

    if (tilemap) {
        const i32 playerTileX{ TruncateF32ToI32((x - tilemap->upperLeftX) /
                                                static_cast<i32>(tilemap->tileWidth)) };
        const i32 playerTileY{ TruncateF32ToI32((y - tilemap->upperLeftY) /
                                                static_cast<i32>(tilemap->tileHeight)) };
        if (playerTileX >= 0 && playerTileX < static_cast<i32>(tilemap->columns) &&
            playerTileY >= 0 && playerTileY < static_cast<i32>(tilemap->rows)) {
            const u32 tilemapValue{ GetTilemapValue(tilemap, playerTileX, playerTileY) };
            isValid = (tilemapValue == 0);
        }
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
    u32 tiles00[tileMapRows][tileMapColumns]{ { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
                                              { 1, 1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1 },
                                              { 1, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 1 },
                                              { 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1 },
                                              { 1, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 1 },
                                              { 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1 },
                                              { 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1 },
                                              { 1, 1, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 1 },
                                              { 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1,
                                                1 } };
    u32 tiles10[tileMapRows][tileMapColumns]{ { 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1 },
                                              { 1, 1, 1, 0, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1 },
                                              { 1, 1, 0, 0, 1, 1, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1 },
                                              { 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1 },
                                              { 1, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0 },
                                              { 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1 },
                                              { 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1 },
                                              { 1, 1, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 1 },
                                              { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                                1 } };
    u32 tiles11[tileMapRows][tileMapColumns]{ { 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1 },
                                              { 1, 1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1 },
                                              { 1, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 1 },
                                              { 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1 },
                                              { 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 1 },
                                              { 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1 },
                                              { 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1 },
                                              { 1, 1, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 1 },
                                              { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                                1 } };
    u32 tiles01[tileMapRows][tileMapColumns]{ { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
                                              { 1, 1, 1, 0, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1 },
                                              { 1, 1, 0, 0, 1, 1, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1 },
                                              { 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1 },
                                              { 1, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 1 },
                                              { 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1 },
                                              { 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1 },
                                              { 1, 1, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 1 },
                                              { 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1,
                                                1 } };

    Tilemap tilemaps[2][2];

    Tilemap* tilemap00{ &tilemaps[0][0] };
    tilemap00->tiles = reinterpret_cast<u32*>(tiles00);
    tilemap00->rows = tileMapRows;
    tilemap00->columns = tileMapColumns;

    tilemap00->upperLeftX = -30; // Move half tileWidth right
    tilemap00->upperLeftY = 0;
    tilemap00->tileWidth = 60;
    tilemap00->tileHeight = 60;

    tilemaps[1][0].tiles = reinterpret_cast<u32*>(tiles10);
    tilemaps[0][1].tiles = reinterpret_cast<u32*>(tiles01);
    tilemaps[1][1].tiles = reinterpret_cast<u32*>(tiles11);

    Tilemap* currentTilemap{ tilemap00 };

    World world{};
    world.tilemaps = reinterpret_cast<Tilemap*>(tilemaps);

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

    const f32 playerWidth{ currentTilemap->tileWidth * 0.75f };
    const f32 playerHeight{ currentTilemap->tileHeight * 0.9f };

    if (IsTilemapPointEmpty(currentTilemap, newPlayerX, newPlayerY) &&
        IsTilemapPointEmpty(currentTilemap, newPlayerX - (playerWidth * 0.5f), newPlayerY) &&
        IsTilemapPointEmpty(currentTilemap, newPlayerX + (playerWidth * 0.5f), newPlayerY)) {
        gameState->playerPosX = newPlayerX;
        gameState->playerPosY = newPlayerY;
    }

    memory->DEBUGPrintFloat(threadContext, "playerX", gameState->playerPosX);
    memory->DEBUGPrintFloat(threadContext, "playerY", gameState->playerPosY);

    // Background
    DrawRectangle(screenBuff, 0.0f, 0.0f, static_cast<f32>(screenBuff->width),
                  static_cast<f32>(screenBuff->height), 0.0f, 0.1f, 0.2f);

    // sizeof(tilemap) / sizeof(tilemap[0])
    for (u32 row{}; row < currentTilemap->rows; ++row) {
        for (u32 column{}; column < currentTilemap->columns; ++column) {
            const u32 tileID{ currentTilemap->tiles[row * currentTilemap->columns + column] };
            f32 gray{ tileID ? 1.0f : 0.5f };

            const f32 minX{ currentTilemap->upperLeftX +
                            (static_cast<f32>(column) * currentTilemap->tileWidth) };
            const f32 minY{ currentTilemap->upperLeftY +
                            (static_cast<f32>(row) * currentTilemap->tileHeight) };
            const f32 maxX{ minX + currentTilemap->tileWidth };
            const f32 maxY{ minY + currentTilemap->tileHeight };

            DrawRectangle(screenBuff, minX, minY, maxX, maxY, .1f, gray, gray);
        }
    }

    // Drawing player
    constexpr f32 playerR{ 0.5f };
    constexpr f32 playerG{ 0.1f };
    constexpr f32 playerB{ 0.5f };

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
