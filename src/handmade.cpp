#include "handmade.h"
#include "handmade_intrinsics.h"

namespace game {

GLOBAL ThreadContext* gThreadContext;
GLOBAL GameMemory* gMemory;

// NOTE: just a hacky way to print things from game code
#define DEBUG_PLATFORM_PRINT(message) (*gMemory->exports.DEBUGPrint)(gThreadContext, message)

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
InitializeGameState(ThreadContext* threadContext, GameState* gameState, GameMemory* memory) {
    // TODO: maybe make platform set this
    memory->isInitialized = true;

    gThreadContext = threadContext;
    gMemory = memory;

    //gameState->playerTilemapX = 0;
    //gameState->playerTilemapY = 0;
    gameState->playerPosX = 150.0f;
    gameState->playerPosY = 200.0f;
}

INTERNAL Tilemap*
GetTileMap(const World* world, u32 tilemapX, u32 tilemapY) {
    Tilemap* tilemap{};

    if (tilemapX >= 0 && tilemapX < world->tilemapCountX && tilemapY >= 0 &&
        tilemapY < world->tilemapCountY) {
        tilemap = &world->tilemaps[(world->tilemapCountX * tilemapY) + tilemapX];
    }

    return tilemap;
}

INTERNAL u32
GetTilemapValue(const World* world, const Tilemap* tilemap, i32 tileX, i32 tileY) {
    ASSERT(tilemap);
    ASSERT(tileX >= 0 && tileX < static_cast<i32>(world->tilemapColumns) && tileY >= 0 &&
           tileY < static_cast<i32>(world->tilemapRows));

    const u32 tilemapValue{ tilemap->tiles[(world->tilemapColumns * tileY) + tileX] };
    return tilemapValue;
}

INTERNAL bool32
IsTilemapPointEmpty(const World* world, const Tilemap* tilemap, i32 testTilemapX,
                    i32 testTilemapY) {
    bool32 isEmpty{};

    if (tilemap) {
        if (testTilemapX >= 0 && testTilemapX < static_cast<i32>(world->tilemapColumns) &&
            testTilemapY >= 0 && testTilemapY < static_cast<i32>(world->tilemapRows)) {
            const u32 tilemapValue{ GetTilemapValue(world, tilemap, testTilemapX, testTilemapY) };
            isEmpty = (tilemapValue == 0);
        }
    }

    return isEmpty;
}

INTERNAL CanonicalWorldPosition
GetCanonicalPosition(const World* world, RawWorldPosition rawPos) {
    CanonicalWorldPosition canPos{};

    canPos.tilemapX = rawPos.tilemapX;
    canPos.tilemapY = rawPos.tilemapY;

    const f32 x{ rawPos.rawPlayerPosX - world->upperLeftX };
    const f32 y{ rawPos.rawPlayerPosY - world->upperLeftY };

    canPos.tileX = FloorF32ToI32(x / world->tileWidth);
    canPos.tileY = FloorF32ToI32(y / world->tileHeight);

    // Tile-relative
    canPos.tileRelativePosX = x - (canPos.tileX * world->tileWidth);
    canPos.tileRelativePosY = y - (canPos.tileY * world->tileHeight);

    // Relative positions must be within the tile size
    ASSERT(canPos.tileRelativePosX >= 0 && canPos.tileRelativePosX < world->tileWidth);
    ASSERT(canPos.tileRelativePosY >= 0 && canPos.tileRelativePosY < world->tileHeight);

    // Up
    if (canPos.tileY < 0) {
        canPos.tileY = world->tilemapRows + canPos.tileY;
        --canPos.tilemapY;
        DEBUG_PLATFORM_PRINT("Tilemap detected: up");
    }
    // Down
    if (canPos.tileY >= static_cast<i32>(world->tilemapRows)) {
        canPos.tileY = canPos.tileY - world->tilemapRows;
        ++canPos.tilemapY;
        DEBUG_PLATFORM_PRINT("Tilemap detected: down");
    }
    // Left
    if (canPos.tileX < 0) {
        canPos.tileX = world->tilemapColumns + canPos.tileX;
        --canPos.tilemapX;
        DEBUG_PLATFORM_PRINT("Tilemap detected: left");
    }
    // Right
    if (canPos.tileX >= static_cast<i32>(world->tilemapColumns)) {
        canPos.tileX = canPos.tileX - world->tilemapColumns;
        ++canPos.tilemapX;
        DEBUG_PLATFORM_PRINT("Tilemap detected: right");
    }

    return canPos;
}

INTERNAL bool32
IsWorldPointEmpty(const World* world, RawWorldPosition rawPos) {
    const auto canPos{ GetCanonicalPosition(world, rawPos) };

    const Tilemap* tilemap{ GetTileMap(world, canPos.tilemapX, canPos.tilemapY) };
    if (!tilemap) {
        // invalid tilemapX or tilemapY
        DEBUG_PLATFORM_PRINT("Invalid tilemapX or tilemapY");
    }

    return IsTilemapPointEmpty(world, tilemap, canPos.tileX, canPos.tileY);
}

// NOTE: use extern "C" to avoid name mangling
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
        InitializeGameState(threadContext, gameState, memory);
    }

    constexpr u32 tileMapRows{ 9 };
    constexpr u32 tileMapColumns{ 17 };
    u32 tiles00[tileMapRows][tileMapColumns]{ { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
                                              { 1, 1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1 },
                                              { 1, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 1 },
                                              { 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1 },
                                              { 1, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0 },
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
                                              { 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 1 },
                                              { 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1 },
                                              { 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1 },
                                              { 1, 1, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 1 },
                                              { 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1,
                                                1 } };

    Tilemap tilemaps[2][2];
    tilemaps[0][0].tiles = reinterpret_cast<u32*>(tiles00);
    tilemaps[1][0].tiles = reinterpret_cast<u32*>(tiles10);
    tilemaps[1][1].tiles = reinterpret_cast<u32*>(tiles11);
    tilemaps[0][1].tiles = reinterpret_cast<u32*>(tiles01);

    World world{};
    world.tilemaps = reinterpret_cast<Tilemap*>(tilemaps);
    world.tilemapCountX = 2;
    world.tilemapCountY = 2;

    world.tilemapRows = tileMapRows;
    world.tilemapColumns = tileMapColumns;

    world.upperLeftX = -30; // Move half tileWidth right
    world.upperLeftY = 0;
    world.tileWidth = 60;
    world.tileHeight = 60;

    Tilemap* currentTilemap{ GetTileMap(&world, gameState->playerTilemapX,
                                        gameState->playerTilemapY) };
    ASSERT(currentTilemap);

    const f32 delta{ input->frameDeltaTime };
    // NOTE: if many players -> loop through input->playerInputs
    const InputButtons* input0Keyboard{ &input->playerInputs[0] };

    f32 playerVelocityX{}, playerVelocityY{};
    constexpr f32 playerSpeed{ 120 }; // Pixels per second
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
    if (input::ActionPressed(&input0Keyboard->shift)) {
        playerVelocityX *= 5;
        playerVelocityY *= 5;
    }

    const f32 playerWidth{ world.tileWidth * 0.75f };
    const f32 playerHeight{ world.tileHeight * 0.9f };

    const f32 newPlayerX{ gameState->playerPosX + (playerVelocityX * delta) };
    const f32 newPlayerY{ gameState->playerPosY + (playerVelocityY * delta) };

    const RawWorldPosition rawPlayerPos{ gameState->playerTilemapX, gameState->playerTilemapY,
                                         newPlayerX, newPlayerY };

    RawWorldPosition rawPlayerPosLeft{ rawPlayerPos };
    rawPlayerPosLeft.rawPlayerPosX -= playerWidth * 0.5f;
    RawWorldPosition rawPlayerPosRight = rawPlayerPos;
    rawPlayerPosRight.rawPlayerPosX += playerWidth * 0.5f;

    if (IsWorldPointEmpty(&world, rawPlayerPos) && IsWorldPointEmpty(&world, rawPlayerPosLeft) &&
        IsWorldPointEmpty(&world, rawPlayerPosRight)) {
        const auto canPos{ GetCanonicalPosition(&world, rawPlayerPos) };

        gameState->playerTilemapX = canPos.tilemapX;
        gameState->playerTilemapY = canPos.tilemapY;
        gameState->playerPosX =
            world.upperLeftX + (canPos.tileX * world.tileWidth) + canPos.tileRelativePosX;
        gameState->playerPosY =
            world.upperLeftY + (canPos.tileY * world.tileHeight) + canPos.tileRelativePosY;
    }

    //memory->DEBUGPrintFloat(threadContext, "playerX", gameState->playerPosX);
    //memory->DEBUGPrintFloat(threadContext, "playerY", gameState->playerPosY);

    // Background
    DrawRectangle(screenBuff, 0.0f, 0.0f, static_cast<f32>(screenBuff->width),
                  static_cast<f32>(screenBuff->height), 0.0f, 0.1f, 0.2f);

    // sizeof(tilemap) / sizeof(tilemap[0])
    for (u32 row{}; row < world.tilemapRows; ++row) {
        for (u32 column{}; column < world.tilemapColumns; ++column) {
            const u32 tileID{ currentTilemap->tiles[row * world.tilemapColumns + column] };
            f32 gray{ tileID ? 1.0f : 0.5f };

            const f32 minX{ world.upperLeftX + (static_cast<f32>(column) * world.tileWidth) };
            const f32 minY{ world.upperLeftY + (static_cast<f32>(row) * world.tileHeight) };
            const f32 maxX{ minX + world.tileWidth };
            const f32 maxY{ minY + world.tileHeight };

            DrawRectangle(screenBuff, minX, minY, maxX, maxY, .1f, gray, gray);
        }
    }

    // Drawing player
    constexpr f32 playerR{ 0.5f };
    constexpr f32 playerG{ 0.1f };
    constexpr f32 playerB{ 0.5f };

    const f32 playerPosLeft{ gameState->playerPosX - (playerWidth * 0.5f) };
    const f32 playerPosTop{ gameState->playerPosY - playerHeight };

    DrawRectangle(screenBuff, playerPosLeft, playerPosTop, playerPosLeft + playerWidth,
                  gameState->playerPosY, playerR, playerG, playerB);
}

extern "C" GET_SOUND_SAMPLES(GetSoundSamples) {
    UNUSED_PARAMS(threadContext);

    GameState* gameState{ static_cast<GameState*>(memory->permanentStorage) };
    OutputSound(gameState, soundBuff, 256);
}

} //namespace game
