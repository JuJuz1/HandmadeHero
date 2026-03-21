#include "handmade.h"
#include "handmade_intrinsics.h"

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

NODISCARD
INTERNAL TileChunk*
GetTileChunk(const World* world, i32 tileChunkX, i32 tileChunkY) {
    TileChunk* tileChunk{};

    if (tileChunkX >= 0 && tileChunkX < world->tileChunkCountX && tileChunkY >= 0 &&
        tileChunkY < world->tileChunkCountY) {
        tileChunk = &world->tileChunks[(world->tileChunkCountX * tileChunkY) + tileChunkX];
    }

    return tileChunk;
}

NODISCARD TileChunkPosition
GetChunkPosition(const World* world, i32 absTileX, i32 absTileY) {
    TileChunkPosition result{};

    // Shift down by chunkShift to get the upper bits for chunk index
    result.tileChunkX = absTileX >> world->chunkShift;
    result.tileChunkY = absTileY >> world->chunkShift;
    // Get the lower 8 bits for tile relative positions
    result.chunkRelativeX = absTileX & world->chunkMask;
    result.chunkRelativeY = absTileY & world->chunkMask;

    return result;
}

NODISCARD
INTERNAL u32
GetTileValueChecked(const World* world, const TileChunk* tileChunk, i32 tileX, i32 tileY) {
    ASSERT(tileChunk);
    ASSERT(tileX < world->chunkDim && tileY < world->chunkDim);
    const u32 tileValue{ tileChunk->tiles[(world->chunkDim * tileY) + tileX] };
    return tileValue;
}

NODISCARD
INTERNAL u32
GetTileValue(const World* world, i32 absTileX, i32 absTileY) {
    const TileChunkPosition chunkPos{ GetChunkPosition(world, absTileX, absTileY) };
    const TileChunk* tileChunk{ GetTileChunk(world, chunkPos.tileChunkX, chunkPos.tileChunkY) };
    if (!tileChunk) {
        // invalid tilemapX or tilemapY
        DEBUG_PLATFORM_PRINT("Invalid tileChunkX or tileChunkY");
    }

    u32 tileChunkValue =
        GetTileValueChecked(world, tileChunk, chunkPos.chunkRelativeX, chunkPos.chunkRelativeY);

    return tileChunkValue;
}

INTERNAL void
ReCanonicalizeCoordinate(const World* world, u32* tileIndex, f32* relPos) {
    const i32 offset{ FloorF32ToI32(*relPos / world->tileSideInMeters) };
    // NOTE: world is assumed to be toroidal, if you step over the end you start at the beginning
    *tileIndex += offset;
    *relPos -= offset * world->tileSideInMeters;

    // TODO: what to do if: *relPos == world->tilesideinpixels
    // This can happen because we do the divide and floor and then multiple, the player might end up
    // being on the same tile
    // Relative positions must be within the tile size in pixels
    // TODO: fix the floating point math to not allow the case above of ==
    ASSERT(*relPos >= 0 && *relPos <= world->tileSideInMeters);
}

NODISCARD
INTERNAL WorldPosition
RecanonicalizePosition(const World* world, WorldPosition pos) {
    WorldPosition result{ pos };

    ReCanonicalizeCoordinate(world, &result.absTileX, &result.tileRelativePosX);
    ReCanonicalizeCoordinate(world, &result.absTileY, &result.tileRelativePosY);

    return result;
}

NODISCARD
INTERNAL bool32
IsWorldPointEmpty(const World* world, WorldPosition pos) {
    //return IsTileChunkTileEmpty(world, tileChunk, chunkPos.tileChunkX, chunkPos.tileChunkY);

    u32 value = GetTileValue(world, pos.absTileX, pos.absTileY);
    bool32 empty = (value == 0);
    return empty;
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

    constexpr i32 tileChunkCountX{ 256 };
    constexpr i32 tileChunkCountY{ 256 };
    u32 tiles[tileChunkCountY][tileChunkCountX]{
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
          1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
          1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1,
          1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0,
          0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 1,
          1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1,
          1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1,
          1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
          1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
          1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
        { 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
          1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
          1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
          1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1,
          1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
          1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
          1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
          1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
          1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
    };

    TileChunk tileChunk[1]{};
    tileChunk[0].tiles = reinterpret_cast<u32*>(tiles);

    World world{};
    //world.tileChunks = reinterpret_cast<TileChunk*>(tileChunk);
    world.tileChunks = reinterpret_cast<TileChunk*>(tileChunk);
    world.tileChunkCountX = 1;
    world.tileChunkCountY = 1;

    world.chunkDim = 256;
    world.chunkShift = 8;
    world.chunkMask = 0xFF;

    world.tileSideInMeters = 1.4f;
    world.tileSideInPixels = 60;
    world.metersToPixels = world.tileSideInPixels / world.tileSideInMeters;

    world.tileSideInPixels = 60;
    world.tileSideInPixels = 60;

    const f32 lowerLeftX =
        -static_cast<f32>(world.tileSideInPixels) / 2.0f; // Move half tileSideInPixels right
    const f32 lowerLeftY = static_cast<f32>(screenBuff->height);

    //TileChunk* currentTileChunk{ GetTileChunk(&world, gameState->playerPos.absTileX,
    //                                          gameState->playerPos.absTileY) };
    //ASSERT(currentTileChunk);

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

    const f32 playerHeight{ world.tileSideInMeters };
    const f32 playerWidth{ playerHeight * 0.75f };

    WorldPosition newPlayerPos{ gameState->playerPos };
    const f32 newPlayerX{ playerVelocityX * delta };
    const f32 newPlayerY{ playerVelocityY * delta };
    newPlayerPos.tileRelativePosX += newPlayerX;
    newPlayerPos.tileRelativePosY += newPlayerY;
    newPlayerPos = RecanonicalizePosition(&world, newPlayerPos);

    WorldPosition testPlayerPosLeft{ newPlayerPos };
    testPlayerPosLeft.tileRelativePosX -= playerWidth * 0.5f;
    testPlayerPosLeft = RecanonicalizePosition(&world, testPlayerPosLeft);

    WorldPosition testPlayerPosRight{ newPlayerPos };
    testPlayerPosRight.tileRelativePosX += playerWidth * 0.5f;
    testPlayerPosRight = RecanonicalizePosition(&world, testPlayerPosRight);

    if (IsWorldPointEmpty(&world, newPlayerPos) && IsWorldPointEmpty(&world, testPlayerPosLeft) &&
        IsWorldPointEmpty(&world, testPlayerPosRight)) {
        gameState->playerPos = newPlayerPos;
    }

    memory->exports.DEBUGPrintInt(threadContext, "absTileX", gameState->playerPos.absTileX);
    memory->exports.DEBUGPrintInt(threadContext, "absTileY", gameState->playerPos.absTileY);
    memory->exports.DEBUGPrintFloat(threadContext, "tileRelX",
                                    gameState->playerPos.tileRelativePosX);
    memory->exports.DEBUGPrintFloat(threadContext, "tileRelY",
                                    gameState->playerPos.tileRelativePosY);

    // Background
    DrawRectangle(screenBuff, 0.0f, 0.0f, static_cast<f32>(screenBuff->width),
                  static_cast<f32>(screenBuff->height), 0.0f, 0.1f, 0.2f);

    for (u32 row{}; row < 9; ++row) {
        for (u32 column{}; column < 17; ++column) {
            const u32 tileID{ GetTileValue(&world, column, row) };
            f32 gray{ tileID ? 1.0f : 0.5f };

            if (row == gameState->playerPos.absTileY && column == gameState->playerPos.absTileX) {
                gray = 0.0f;
            }

            const f32 minX{ lowerLeftX + (static_cast<f32>(column) * world.tileSideInPixels) };
            const f32 minY{ lowerLeftY - (static_cast<f32>(row) * world.tileSideInPixels) };
            const f32 maxX{ minX + world.tileSideInPixels };
            const f32 maxY{ minY - world.tileSideInPixels };

            DrawRectangle(screenBuff, minX, maxY, maxX, minY, 0.1f, gray, gray);
        }
    }

    // Drawing player
    constexpr f32 playerR{ 0.5f };
    constexpr f32 playerG{ 0.1f };
    constexpr f32 playerB{ 0.5f };

    const f32 playerPosLeft{ lowerLeftX + (world.tileSideInPixels * gameState->playerPos.absTileX) +
                             (world.metersToPixels * gameState->playerPos.tileRelativePosX) -
                             (playerWidth * 0.5f * world.metersToPixels) };
    const f32 playerPosTop{ lowerLeftY - (world.tileSideInPixels * gameState->playerPos.absTileY) -
                            (world.metersToPixels * gameState->playerPos.tileRelativePosY) -
                            (playerHeight * world.metersToPixels) };

    DrawRectangle(screenBuff, playerPosLeft, playerPosTop,
                  playerPosLeft + (playerWidth * world.metersToPixels),
                  playerPosTop + (playerHeight * world.metersToPixels), playerR, playerG, playerB);
}

extern "C" GET_SOUND_SAMPLES(GetSoundSamples) {
    UNUSED_PARAMS(threadContext);

    GameState* gameState{ static_cast<GameState*>(memory->permanentStorage) };
    OutputSound(gameState, soundBuff, 256);
}

} //namespace game
