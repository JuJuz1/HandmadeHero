#include "handmade.h"

#include "handmade_tile.cpp"

namespace game {

// Any global variables need to be initialized after hot reload (so probably every frame)
GLOBAL ThreadContext* gThreadContext;
GLOBAL GameMemory* gMemory;

// NOTE: just a hacky way to print things from game code
#define DEBUG_PLATFORM_PRINT(message) (*gMemory->exports.DEBUGPrint)(gThreadContext, message)

namespace input {

NODISCARD
INTERNAL bool32
ActionJustPressed(const Button* button) {
    const bool32 result{ button->endedDown && button->halfTransitionCount > 0 };
    return result;
}

NODISCARD
INTERNAL bool32
ActionPressed(const Button* button) {
    const bool32 result{ button->endedDown };
    return result;
}

NODISCARD
INTERNAL bool32
ActionReleased(const Button* button) {
    const bool32 result{ !button->endedDown && button->halfTransitionCount > 0 };
    return result;
}

} //namespace input

// Write the sound data to buff
INTERNAL void
OutputSound(GameState* gameState, const SoundOutputBuffer* buff, i32 toneHz) {
    constexpr i32 toneVolume{ 3000 };
    const i32 wavePeriod{ buff->samplesPerSecond / toneHz };

    // NOTE: invalid semantics as buff is const but we copy the pointer...
    i16* sampleOut{ buff->samples };

    for (i32 i{}; i < buff->sampleCount; ++i) {
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
    const i32 color{ (RoundF32ToI32(r * 255.0f) << 16) | (RoundF32ToI32(g * 255.0f) << 8) |
                     (RoundF32ToI32(b * 255.0f) << 0) };

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

    gameState->playerPos.absTileX = 3;
    gameState->playerPos.absTileY = 3;
    //gameState->playerPos.tileRelativePosX = 1.0f;
    //gameState->playerPos.tileRelativePosY = 0.5f;
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

    gThreadContext = threadContext;
    gMemory = memory;

    constexpr u32 tileChunkCountX{ 256 };
    constexpr u32 tileChunkCountY{ 256 };
    // clang-format off
    u32 tiles[tileChunkCountY][tileChunkCountX]{
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
        {1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1},
        {1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 2, 2, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1},
        {1, 2, 2, 2, 1, 2, 1, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 2, 2, 1},
        {1, 2, 2, 2, 1, 2, 2, 2, 2, 1, 1, 2, 1, 2, 2, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1},
        {1, 2, 2, 2, 1, 1, 1, 2, 2, 2, 2, 2, 1, 2, 2, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1},
        {1, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 1, 2, 2, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1},
        {1, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1},
        {1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1},
        {1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1},
        {1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1},
        {1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1},
        {1, 2, 2, 2, 2, 2, 1, 1, 2, 2, 2, 2, 2, 2, 2, 1, 1, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 2, 2, 1},
        {1, 2, 2, 2, 2, 2, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1},
        {1, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1},
        {1, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1},
        {1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1},
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    };
    // clang-format on

    TileChunk tileChunk[1];
    tileChunk[0].tiles = reinterpret_cast<u32*>(tiles);

    TileMap tileMap{};
    //tileMap.tileChunks = reinterpret_cast<TileChunk*>(tileChunk);
    tileMap.tileChunks = reinterpret_cast<TileChunk*>(tileChunk);
    tileMap.tileChunkCountX = 1;
    tileMap.tileChunkCountY = 1;

    // chunk size is chunkDim x chunkDim
    tileMap.chunkShift = 8;
    tileMap.chunkMask = (1 << tileMap.chunkShift) - 1;
    tileMap.chunkDim = 256;

    tileMap.tileSideInMeters = 1.4f;
    tileMap.tileSideInPixels = 60;
    tileMap.metersToPixels = static_cast<f32>(tileMap.tileSideInPixels) / tileMap.tileSideInMeters;

    tileMap.tileSideInPixels = 60;
    tileMap.tileSideInPixels = 60;

    const f32 delta{ input->frameDeltaTime };
    // NOTE: if many players -> loop through input->playerInputs
    const InputButtons* input0Keyboard{ &input->playerInputs[0] };

    f32 playerVelocityX{}, playerVelocityY{};
    constexpr f32 playerSpeed{ 3 }; // Meters per second
    constexpr f32 playerSpeedModifier{ 4 };
    if (input::ActionPressed(&input0Keyboard->up)) {
        playerVelocityY += playerSpeed;
    }
    if (input::ActionPressed(&input0Keyboard->down)) {
        playerVelocityY -= playerSpeed;
    }
    if (input::ActionPressed(&input0Keyboard->left)) {
        playerVelocityX -= playerSpeed;
    }
    if (input::ActionPressed(&input0Keyboard->right)) {
        playerVelocityX += playerSpeed;
    }
    if (input::ActionPressed(&input0Keyboard->shift)) {
        playerVelocityX *= playerSpeedModifier;
        playerVelocityY *= playerSpeedModifier;
    }

    const f32 playerHeight{ tileMap.tileSideInMeters };
    const f32 playerWidth{ playerHeight * 0.75f };

    TileMapPosition newplayerPos{ gameState->playerPos };
    const f32 newPlayerX{ playerVelocityX * delta };
    const f32 newPlayerY{ playerVelocityY * delta };
    newplayerPos.tileRelativePosX += newPlayerX;
    newplayerPos.tileRelativePosY += newPlayerY;
    newplayerPos = RecanonicalizePosition(&tileMap, newplayerPos);

    TileMapPosition testplayerPosLeft{ newplayerPos };
    testplayerPosLeft.tileRelativePosX -= playerWidth * 0.5f;
    testplayerPosLeft = RecanonicalizePosition(&tileMap, testplayerPosLeft);

    TileMapPosition testplayerPosRight{ newplayerPos };
    testplayerPosRight.tileRelativePosX += playerWidth * 0.5f;
    testplayerPosRight = RecanonicalizePosition(&tileMap, testplayerPosRight);

    if (IsTileMapPointEmpty(&tileMap, newplayerPos) &&
        IsTileMapPointEmpty(&tileMap, testplayerPosLeft) &&
        IsTileMapPointEmpty(&tileMap, testplayerPosRight)) {
        gameState->playerPos = newplayerPos;
    }

    const TileChunkPosition chunkPos{ GetChunkPosition(&tileMap, gameState->playerPos.absTileX,
                                                       gameState->playerPos.absTileY) };

    memory->exports.DEBUGPrintInt(threadContext, "tileChunkX", chunkPos.chunkX);
    memory->exports.DEBUGPrintInt(threadContext, "tileChunkY", chunkPos.chunkY);
    memory->exports.DEBUGPrintInt(threadContext, "chunkRelativeX", chunkPos.chunkRelativeX);
    memory->exports.DEBUGPrintInt(threadContext, "chunkRelativeY", chunkPos.chunkRelativeY);

    memory->exports.DEBUGPrintInt(threadContext, "absTileX", gameState->playerPos.absTileX);
    memory->exports.DEBUGPrintInt(threadContext, "absTileY", gameState->playerPos.absTileY);
    memory->exports.DEBUGPrintFloat(threadContext, "tileRelX",
                                    gameState->playerPos.tileRelativePosX);
    memory->exports.DEBUGPrintFloat(threadContext, "tileRelY",
                                    gameState->playerPos.tileRelativePosY);

    // Background
    DrawRectangle(screenBuff, 0.0f, 0.0f, static_cast<f32>(screenBuff->width),
                  static_cast<f32>(screenBuff->height), 0.0f, 0.1f, 0.2f);

    const f32 screenCenterX{ static_cast<f32>(screenBuff->width) * 0.5f };
    const f32 screenCenterY{ static_cast<f32>(screenBuff->height) * 0.5f };

    // Drawing tiles
    for (i32 relRow{ -10 }; relRow < 10; ++relRow) {
        for (i32 relColumn{ -20 }; relColumn < 20; ++relColumn) {
            // Possibly wraps to U32 max
            const u32 row{ gameState->playerPos.absTileY + relRow };
            const u32 column{ gameState->playerPos.absTileX + relColumn };

            const u32 tileID{ GetTileValue(&tileMap, column, row) };

            f32 gray{ 1.0f };
            if (tileID == 0) {
                gray = 0.0f;
            } else if (tileID == blocked_Tile_Value) {
                gray = 0.5f;
            }

            if (row == gameState->playerPos.absTileY && column == gameState->playerPos.absTileX) {
                gray = 0.25f;
            }

            const f32 tileCenX{ screenCenterX -
                                (tileMap.metersToPixels * gameState->playerPos.tileRelativePosX) +
                                (static_cast<f32>(relColumn * tileMap.tileSideInPixels)) };
            const f32 tileCenY{ screenCenterY +
                                (tileMap.metersToPixels * gameState->playerPos.tileRelativePosY) -
                                (static_cast<f32>(relRow * tileMap.tileSideInPixels)) };

            const f32 minX{ tileCenX - (static_cast<f32>(tileMap.tileSideInPixels) * 0.5f) };
            const f32 minY{ tileCenY - (static_cast<f32>(tileMap.tileSideInPixels) * 0.5f) };
            const f32 maxX{ tileCenX + (static_cast<f32>(tileMap.tileSideInPixels) * 0.5f) };
            const f32 maxY{ tileCenY + (static_cast<f32>(tileMap.tileSideInPixels) * 0.5f) };

            DrawRectangle(screenBuff, minX, minY, maxX, maxY, 0.1f, gray, gray);
        }
    }

    // Drawing player
    constexpr f32 playerR{ 0.5f };
    constexpr f32 playerG{ 0.1f };
    constexpr f32 playerB{ 0.5f };

    const f32 playerPosLeft{ screenCenterX - (playerWidth * 0.5f * tileMap.metersToPixels) };
    const f32 playerPosTop{ screenCenterY - (playerHeight * tileMap.metersToPixels) };

    DrawRectangle(screenBuff, playerPosLeft, playerPosTop,
                  playerPosLeft + (playerWidth * tileMap.metersToPixels),
                  playerPosTop + (playerHeight * tileMap.metersToPixels), playerR, playerG,
                  playerB);
}

extern "C" GET_SOUND_SAMPLES(GetSoundSamples) {
    UNUSED_PARAMS(threadContext);

    GameState* gameState{ static_cast<GameState*>(memory->permanentStorage) };
    OutputSound(gameState, soundBuff, 256);
}

} //namespace game
