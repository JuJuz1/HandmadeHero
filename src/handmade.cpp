#include "handmade.h"

#include "handmade_math.h"
#include "handmade_random.h"
#include "handmade_tile.cpp"

// Any global variables need to be initialized after hot reload (so probably every frame)
GLOBAL ThreadContext* gThreadContext;
GLOBAL GameMemory* gMemory;

// NOTE: just a hacky way to print things from game code
// TODO: think of a better way!
#define DEBUG_PLATFORM_PRINT(message) (*gMemory->exports.DEBUGPrint)(gThreadContext, message)

namespace hm_input {

// Returns true if the button was just pressed during the frame
NODISCARD
INTERNAL bool32
ActionJustPressed(const Button* button) {
    const bool32 result{ button->endedDown && button->halfTransitionCount > 0 };
    return result;
}

// Returns true if the button was pressed during the frame
// As this returns true for the first frame as well, ActionJustPressed and ActionPressed both return
// true for the first frame for the same button
NODISCARD
INTERNAL bool32
ActionPressed(const Button* button) {
    const bool32 result{ button->endedDown };
    return result;
}

// Returns true if the button was just released during the frame
NODISCARD
INTERNAL bool32
ActionReleased(const Button* button) {
    const bool32 result{ !button->endedDown && button->halfTransitionCount > 0 };
    return result;
}

} //namespace hm_input

// Write the sound data to buff
INTERNAL void
OutputSound(const GameState* gameState, const SoundOutputBuffer* buff) {}

INTERNAL void
DrawRectangle(const OffScreenBuffer* screenBuff, Vec2 min, Vec2 max, f32 r, f32 g, f32 b) {
    i32 roundedMinX{ RoundF32ToI32(min.x) };
    i32 roundedMinY{ RoundF32ToI32(min.y) };
    i32 roundedMaxX{ RoundF32ToI32(max.x) };
    i32 roundedMaxY{ RoundF32ToI32(max.y) };

    if (roundedMinX < 0) {
        roundedMinX = 0;
    }
    if (roundedMinY < 0) {
        roundedMinY = 0;
    }

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
        // Not including fill pixel
        u32* pixel{ reinterpret_cast<u32*>(row) };
        for (i32 x{ roundedMinX }; x < roundedMaxX; ++x) {
            *pixel++ = color;
        }

        row += screenBuff->pitch;
    }
}

// Struct packing to avoid manual work
#pragma pack(push, 1)

// https://en.wikipedia.org/wiki/BMP_file_format#Example_2
struct BitmapHeader {
    u16 type;
    u32 fileSize;
    i16 reserved1;
    i16 reserved2;
    u32 bitMapOffset;
    u32 size;
    i32 width;
    i32 height;
    u16 planes;
    u16 bitsPerPixel;
    u32 compression;
    u32 sizeOfBitmap;
    i32 horzResolution;
    i32 vertResolution;
    u32 colorsUsed;
    u32 colorsImportant;

    u32 redMask;
    u32 greenMask;
    u32 blueMask;
    u32 alphaMask;
};

#pragma pack(pop)

INTERNAL LoadedBitmapInfo
DEBUGLoadBMP(ThreadContext* threadContext, debug_read_file* readFile, const char* filename) {
    LoadedBitmapInfo result{};

    auto readFileResult{ readFile(threadContext, filename) };
    if (readFileResult.content) {
        const BitmapHeader* bitMapHeader{ static_cast<BitmapHeader*>(readFileResult.content) };
        u32* pixels{ reinterpret_cast<u32*>(static_cast<u8*>(readFileResult.content) +
                                            bitMapHeader->bitMapOffset) };

        result.pixels = pixels;
        result.width = bitMapHeader->width;
        result.height = bitMapHeader->height;

        // IMPORTANT: Byte order of bmp is determined by the header!
        // It seems we have a value of 3 for compression always, and the masks change between files!
        // NOTE: can most likely support other compression values as well!
        ASSERT(bitMapHeader->compression == 3);

        const u32 redMask{ bitMapHeader->redMask };
        const u32 greenMask{ bitMapHeader->greenMask };
        const u32 blueMask{ bitMapHeader->blueMask };
        const u32 alphaMask{ ~(redMask | greenMask | blueMask) };
        //const u32 alphaMask{ bitMapHeader->alphaMask }; commented out to investigate crash reason

        // Can also use _rotl
        // https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/rotl-rotl64-rotr-rotr64?view=msvc-170
        const BitscanResult redShift{ FindLeastSignificantBitSet(redMask) };
        const BitscanResult greenShift{ FindLeastSignificantBitSet(greenMask) };
        const BitscanResult blueShift{ FindLeastSignificantBitSet(blueMask) };
        const BitscanResult alphaShift{ FindLeastSignificantBitSet(alphaMask) };

        ASSERT(redShift.found);
        ASSERT(greenShift.found);
        ASSERT(blueShift.found);
        ASSERT(alphaShift.found);

        u32* srcDest{ pixels };

        for (i32 y{}; y < bitMapHeader->height; ++y) {
            for (i32 x{}; x < bitMapHeader->width; ++x) {
                const u32 C{ *srcDest };
                *srcDest++ = ((((C >> alphaShift.index) & 0xFF) << 24) |
                              (((C >> redShift.index) & 0xFF) << 16) |
                              (((C >> greenShift.index) & 0xFF) << 8) |
                              (((C >> blueShift.index) & 0xFF) << 0));
            }
        }
    } else {
        DEBUG_PLATFORM_PRINT("Couldn't load bmp!\n");
    }

    return result;
}

INTERNAL void
DrawBitmap(const OffScreenBuffer* screenBuff, const LoadedBitmapInfo* bitmap, f32 xPos, f32 yPos,
           i32 alignX = 0, i32 alignY = 0) {
    const Vec2 aligned{ xPos - static_cast<f32>(alignX), yPos - static_cast<f32>(alignY) };

    i32 roundedMinX{ RoundF32ToI32(aligned.x) };
    i32 roundedMinY{ RoundF32ToI32(aligned.y) };
    i32 roundedMaxX{ RoundF32ToI32(aligned.x + static_cast<f32>(bitmap->width)) };
    i32 roundedMaxY{ RoundF32ToI32(aligned.y + static_cast<f32>(bitmap->height)) };

    i32 srcOffsetX{};
    if (roundedMinX < 0) {
        srcOffsetX = -roundedMinX;
        roundedMinX = 0;
    }

    i32 srcOffsetY{};
    if (roundedMinY < 0) {
        srcOffsetY = -roundedMinY;
        roundedMinY = 0;
    }

    if (roundedMaxX > screenBuff->width) {
        roundedMaxX = screenBuff->width;
    }
    if (roundedMaxY > screenBuff->height) {
        roundedMaxY = screenBuff->height;
    }

    // Start from the last row (top row of the image) as the bitmap is stored bottom up
    u32* srcRow{ bitmap->pixels + (bitmap->width * (bitmap->height - 1)) };
    // Handle offsets to fix top and left side clipping
    srcRow += -(bitmap->width * srcOffsetY) + srcOffsetX;

    u8* destRow{ static_cast<u8*>(screenBuff->memory) + (roundedMinY * screenBuff->pitch) +
                 (roundedMinX * screenBuff->bytesPerPixel) };
    for (i32 y{ roundedMinY }; y < roundedMaxY; ++y) {
        u32* dest{ reinterpret_cast<u32*>(destRow) };
        u32* src{ srcRow };
        for (i32 x{ roundedMinX }; x < roundedMaxX; ++x) {
            // FIXME: Sometimes we crash here, but not often
            const f32 alpha{ static_cast<f32>((*src >> 24) & 0xFF) / 255.0f };
            const f32 srcRed{ static_cast<f32>((*src >> 16) & 0xFF) };
            const f32 srcGreen{ static_cast<f32>((*src >> 8) & 0xFF) };
            const f32 srcBlue{ static_cast<f32>((*src >> 0) & 0xFF) };

            const f32 destRed{ static_cast<f32>((*dest >> 16) & 0xFF) };
            const f32 destGreen{ static_cast<f32>((*dest >> 8) & 0xFF) };
            const f32 destBlue{ static_cast<f32>((*dest >> 0) & 0xFF) };

            // Linear blend
            const f32 resultRed{ ((1.0f - alpha) * destRed) + (alpha * srcRed) };
            const f32 resultGreen{ ((1.0f - alpha) * destGreen) + (alpha * srcGreen) };
            const f32 resultBlue{ ((1.0f - alpha) * destBlue) + (alpha * srcBlue) };

            *dest = { (TruncateF32ToU32(resultRed + 0.5f) << 16) |
                      (TruncateF32ToU32(resultGreen + 0.5f) << 8) |
                      (TruncateF32ToU32(resultBlue + 0.5f) << 0) };

            ++dest;
            ++src;
        }

        destRow += screenBuff->pitch;
        // Move to the start of the above row
        srcRow += -bitmap->width;
    }
}

NODISCARD
INTERNAL Entity*
GetEntity(GameState* gameState, i32 entityIndex) {
    // Or just return nullptr if not inside bounds?
    ASSERT(entityIndex >= 0 && entityIndex < ARRAY_COUNT(gameState->entities));
    Entity* result{ &gameState->entities[entityIndex] };
    return result;
}

NODISCARD
INTERNAL i32
AddEntity(GameState* gameState) {
    ASSERT(gameState->entityCount < ARRAY_COUNT(gameState->entities));
    const i32 entityIndex{ gameState->entityCount++ };
    Entity* entity{ GetEntity(gameState, entityIndex) };
    *entity = Entity{};

    return entityIndex;
}

INTERNAL void
InitializeGameState(ThreadContext* threadContext, GameState* gameState, GameMemory* memory) {
    // TODO: maybe make platform set this
    memory->isInitialized = true;

    // NOTE: reserve slot 0 for null entity
    const i32 nullEntityIndex{ AddEntity(gameState) };
    //gameState->cameraFollowingEntityIndex = 1;

    // Changed to false after initializing one player
    gameState->startWithAPlayer = true;

    gameState->background =
        DEBUGLoadBMP(threadContext, memory->exports.DEBUGReadFile, "test/test_background.bmp");

    // NOTE: should come up with a better way of doing this rather than looking at the images
    // Offsets: align x and y
    // 48 100 forward
    // 46 104 left
    // 42 100 backward
    // 44 104 right

    HeroBitmaps* heroBitmaps{ &gameState->heroBitmaps[0] };

    heroBitmaps->head =
        DEBUGLoadBMP(threadContext, memory->exports.DEBUGReadFile, "test/player_head_forward.bmp");
    heroBitmaps->torso =
        DEBUGLoadBMP(threadContext, memory->exports.DEBUGReadFile, "test/player_torso_forward.bmp");
    heroBitmaps->align = Vec2{ 48, 100 };
    heroBitmaps++;

    heroBitmaps->head =
        DEBUGLoadBMP(threadContext, memory->exports.DEBUGReadFile, "test/player_head_left.bmp");
    heroBitmaps->torso =
        DEBUGLoadBMP(threadContext, memory->exports.DEBUGReadFile, "test/player_torso_left.bmp");
    heroBitmaps->align = Vec2{ 46, 104 };
    heroBitmaps++;

    heroBitmaps->head =
        DEBUGLoadBMP(threadContext, memory->exports.DEBUGReadFile, "test/player_head_backward.bmp");
    heroBitmaps->torso = DEBUGLoadBMP(threadContext, memory->exports.DEBUGReadFile,
                                      "test/player_torso_backward.bmp");
    heroBitmaps->align = Vec2{ 42, 100 };
    heroBitmaps++;

    heroBitmaps->head =
        DEBUGLoadBMP(threadContext, memory->exports.DEBUGReadFile, "test/player_head_right.bmp");
    heroBitmaps->torso =
        DEBUGLoadBMP(threadContext, memory->exports.DEBUGReadFile, "test/player_torso_right.bmp");
    heroBitmaps->align = Vec2{ 44, 104 };

    gameState->cameraPos.absTileX = 17 / 2;
    gameState->cameraPos.absTileY = 9 / 2;
    gameState->cameraPos.absTileZ = 0;

    InitializeArena(&gameState->worldArena,
                    static_cast<u8*>(memory->permanentStorage) + sizeof(GameState),
                    memory->permanentStorageSize - sizeof(GameState));

    gameState->world = PushSize(&gameState->worldArena, World);
    World* world{ gameState->world };
    world->tilemap = PushSize(&gameState->worldArena, Tilemap);

    Tilemap* tilemap{ world->tilemap };
    tilemap->tileChunkCountX = 128;
    tilemap->tileChunkCountY = 128;
    tilemap->tileChunkCountZ = 2; // Limited to 2 atm

    // chunk size is chunkSize x chunkSize (really: chunkShift * chunkShift)
    tilemap->chunkShift = 4;
    tilemap->chunkMask = (1 << tilemap->chunkShift) - 1;
    tilemap->chunkSize = 1 << tilemap->chunkShift;

    tilemap->tileChunks = PushArray(
        &gameState->worldArena,
        tilemap->tileChunkCountX * tilemap->tileChunkCountY * tilemap->tileChunkCountZ, Tilechunk);

    // NOTE: This is now seperated from the rendering (tileSideInPixels)
    tilemap->tileSideInMeters = 1.4f;

    u32 randomNumIndex{};

    bool32 doorLeft{};
    bool32 doorRight{};
    bool32 doorTop{};
    bool32 doorBottom{};

    bool32 doorUp{};
    bool32 doorDown{};

    u32 absTileZ{};

    // How many screens widths of chunks to generate
    constexpr u32 screenCount{ 100 };
    u32 screenY{}, screenX{};
    constexpr u32 tilesPerHeight{ 9 };
    constexpr u32 tilesPerWidth{ 17 };

    // Generating tile values
    for (u32 screen{}; screen < screenCount; ++screen) {
        ASSERT(randomNumIndex < ARRAY_COUNT(hm_random::randomNumbers));
        u32 randomChoice;
        // Lateral only
        if (doorUp || doorDown) {
            randomChoice = hm_random::randomNumbers[randomNumIndex++] % 2;
        } else {
            randomChoice = hm_random::randomNumbers[randomNumIndex++] % 3;
        }

        bool32 createdZDoor{};
        // randomChoice of 2 means the room is blocked and has a door going up
        // Atm this logic means we can only have 2 layers (z of 0 or 1)
        if (randomChoice == 2) {
            createdZDoor = true;
            if (absTileZ == 0) {
                doorUp = true;
            } else {
                doorDown = true;
            }
        } else if (randomChoice == 1) {
            doorRight = true;
        } else {
            doorTop = true;
        }

        for (u32 tileY{}; tileY < tilesPerHeight; ++tileY) {
            for (u32 tileX{}; tileX < tilesPerWidth; ++tileX) {
                const u32 absTileX{ (screenX * tilesPerWidth) + tileX };
                const u32 absTileY{ (screenY * tilesPerHeight) + tileY };

                u32 tileValue{ 2 };
                if (tileX == 0 && (!doorLeft || (tileY != (tilesPerHeight / 2)))) {
                    tileValue = blocked_Tile_Value;
                }
                if (tileX == (tilesPerWidth - 1) &&
                    (!doorRight || (tileY != (tilesPerHeight / 2)))) {
                    tileValue = blocked_Tile_Value;
                }
                if (tileY == 0 && (!doorBottom || (tileX != tilesPerWidth / 2))) {
                    tileValue = blocked_Tile_Value;
                }
                if (tileY == (tilesPerHeight - 1) && (!doorTop || (tileX != tilesPerWidth / 2))) {
                    tileValue = blocked_Tile_Value;
                }

                if (tileX == 10 && tileY == 6) {
                    if (doorUp) {
                        tileValue = 4;
                    }
                    if (doorDown) {
                        tileValue = 5;
                    }
                }

                SetTileValue(&gameState->worldArena, tilemap, absTileX, absTileY, absTileZ,
                             tileValue);
            }
        }

        doorLeft = doorRight;
        doorBottom = doorTop;

        doorRight = false;
        doorTop = false;

        if (createdZDoor) {
            doorDown = !doorDown;
            doorUp = !doorUp;
        } else {
            doorUp = false;
            doorDown = false;
        }

        if (randomChoice == 2) {
            if (absTileZ == 0) {
                absTileZ = 1;
            } else {
                absTileZ = 0;
            }
        }
        // Advance screens if we didn't make a vertical floor (door)
        else if (randomChoice == 1) {
            ++screenX;
        } else {
            ++screenY;
        }
    }
}

INTERNAL void
InitializePlayer(Entity* entity) {
    entity->exists = true;

    entity->pos.absTileX = 3;
    entity->pos.absTileY = 3;
    entity->dimensions.y = 0.5f;  // 1.4f;
    entity->dimensions.x = 0.75f; // entity->dimensions.y * 0.75f;
}

struct TestWallResult {
    f32 tMin;
    bool32 hit;
};

INTERNAL TestWallResult
TestWall(f32 wallX, f32 relX, f32 relY, f32 playerDeltaX, f32 playerDeltaY, f32 tMin, f32 minY,
         f32 maxY) {
    // TODO: this should be moved elsewhere and not be in playerDelta space
    constexpr f32 tEps{ 0.0001f };
    TestWallResult result{};
    f32 newTMin{ tMin };

    if (playerDeltaX != 0.0f) {
        const f32 tResult{ (wallX - relX) / playerDeltaX };
        const f32 newY{ relY + (tResult * playerDeltaY) };
        if (tResult >= 0.0f && tResult < tMin) {
            if (newY >= minY && newY <= maxY) {
                newTMin = MAX(0.0f, tResult - tEps);
                result.tMin = newTMin;
                result.hit = true;
            }
        }
    }

    return result;
}

INTERNAL void
MovePlayer(const Tilemap* tilemap, Entity* entity, const InputButtons* inputButtons,
           Vec2 acceleration, f32 delta) {
    constexpr f32 playerSpeed{ 30 };
    constexpr f32 playerSpeedModifier{ 4 };

    // Normalize if greater than unit circle length of 1
    const f32 accelerationLengthSq{ LengthSquared(acceleration) };
    if (accelerationLengthSq > 1.0f) {
        acceleration *= (1.0f / Sqrt(accelerationLengthSq));
    }

    acceleration *= playerSpeed;

    if (hm_input::ActionPressed(&inputButtons->shift)) {
        acceleration *= playerSpeedModifier;
    }

    // TODO: ordinary differential equations
    acceleration += -4.0f * entity->velocity;
    // v' = at + v
    entity->velocity += acceleration * delta;

    // p' = 0.5 * at^2 + vt + p
    Vec2 playerDelta{ (0.5f * acceleration * SquareF32(delta)) + (entity->velocity * delta) };

    const TilemapPosition oldPlayerPos{ entity->pos };
    const TilemapPosition newPlayerPos{ OffsetTilemapPosition(tilemap, oldPlayerPos, playerDelta) };

    // Search in t

    // Take the "starting" tile and the "ending" tile
    // Taking MIN and MAX takes into account
    // all possible directions (left to right, right to left, ...)
    u32 minTileX{ MIN(oldPlayerPos.absTileX, newPlayerPos.absTileX) };
    u32 minTileY{ MIN(oldPlayerPos.absTileY, newPlayerPos.absTileY) };
    u32 maxTileX{ MAX(oldPlayerPos.absTileX, newPlayerPos.absTileX) };
    u32 maxTileY{ MAX(oldPlayerPos.absTileY, newPlayerPos.absTileY) };

    const i32 entityTileWidth{ CeilF32ToI32(entity->dimensions.x / tilemap->tileSideInMeters) };
    const i32 entityTileHeight{ CeilF32ToI32(entity->dimensions.y / tilemap->tileSideInMeters) };

    // Extend bounds for Minkowkski collision detection
    minTileX -= entityTileWidth;
    minTileY -= entityTileHeight;
    maxTileX += entityTileWidth;
    maxTileY += entityTileHeight;

    const u32 absTileZ{ entity->pos.absTileZ };

    f32 tRemaining{ 1.0f }; // Keeps track of how much t we moved per iteration
    bool32 hitWall{};       // Used to modify velocity if we hit a wall during the frame

    constexpr i32 iterationCount{ 4 };
    for (i32 iteration{}; (iteration < iterationCount) && (tRemaining > 0.0f); ++iteration) {
        f32 tMin{ 1.0f };
        Vec2 wallNormal{};
        TestWallResult result{};

        // Assert that we are never wrapping
        // We should never be moving more than 1 or 2 tiles per frame? so 32 is enough when taking
        // into account the Minkowski sum
        ASSERT((maxTileX - minTileX) < 32);
        ASSERT((maxTileY - minTileY) < 32);
        ASSERT(maxTileX < (UINT32_MAX - 100));
        ASSERT(maxTileY < (UINT32_MAX - 100));

        for (u32 absTileY{ minTileY }; absTileY <= maxTileY; ++absTileY) {
            for (u32 absTileX{ minTileX }; absTileX <= maxTileX; ++absTileX) {
                const TilemapPosition testPos{ absTileX, absTileY, absTileZ };
                if (!IsTilemapPointEmpty(tilemap, testPos)) {
                    const f32 diameterWidth{ tilemap->tileSideInMeters + entity->dimensions.x };
                    const f32 diameterHeight{ tilemap->tileSideInMeters + entity->dimensions.y };
                    const Vec2 minCorner{ Vec2{ -diameterWidth, -diameterHeight } * 0.5f };
                    const Vec2 maxCorner{ Vec2{ diameterWidth, diameterHeight } * 0.5f };

                    const TilemapDiff relNewPlayerPos{ SubtractTilemapPos(tilemap, &entity->pos,
                                                                          &testPos) };
                    const f32 relPosX{ relNewPlayerPos.x };
                    const f32 relPosY{ relNewPlayerPos.y };

                    // Test all four walls

                    // x
                    result = TestWall(minCorner.x, relPosX, relPosY, playerDelta.x, playerDelta.y,
                                      tMin, minCorner.y, maxCorner.y);
                    if (result.hit) {
                        tMin = result.tMin;
                        wallNormal = Vec2{ -1, 0 };
                        hitWall = true;
                    }

                    result = TestWall(maxCorner.x, relPosX, relPosY, playerDelta.x, playerDelta.y,
                                      tMin, minCorner.y, maxCorner.y);
                    if (result.hit) {
                        tMin = result.tMin;
                        wallNormal = Vec2{ 1, 0 };
                        hitWall = true;
                    }

                    // y
                    result = TestWall(minCorner.y, relPosY, relPosX, playerDelta.y, playerDelta.x,
                                      tMin, minCorner.x, maxCorner.x);
                    if (result.hit) {
                        tMin = result.tMin;
                        wallNormal = Vec2{ 0, -1 };
                        hitWall = true;
                    }

                    result = TestWall(maxCorner.y, relPosY, relPosX, playerDelta.y, playerDelta.x,
                                      tMin, minCorner.x, maxCorner.x);
                    if (result.hit) {
                        tMin = result.tMin;
                        wallNormal = Vec2{ 0, 1 };
                        hitWall = true;
                    }
                }
            }
        }

        //gMemory->exports.DEBUGPrintFloat(gThreadContext, "tMin", tMin);
        entity->pos = OffsetTilemapPosition(tilemap, entity->pos, playerDelta * tMin);
        entity->velocity -= 1.0f * Dot(entity->velocity, wallNormal) * wallNormal;
        playerDelta -= 1.0f * Dot(playerDelta, wallNormal) * wallNormal;
        tRemaining -= tMin * tRemaining;
    }

    // Delta independent friction using exponential decay: e^(-kt)
    if (hitWall) {
        constexpr f32 frictionModifier{ 2.0f };
        const f32 friction{ ExpF32(-frictionModifier * delta) };
        entity->velocity *= friction;
    }

    // Door checks
    if (!AreOnSameTiles(&oldPlayerPos, &entity->pos)) {
        const u32 newTileValue{ GetTileValue(tilemap, entity->pos.absTileX, entity->pos.absTileY,
                                             entity->pos.absTileZ) };
        if (newTileValue == 4) {
            entity->pos.absTileZ = TilemapPositionModifyZChecked(tilemap, &entity->pos, 1);
        } else if (newTileValue == 5) {
            entity->pos.absTileZ = TilemapPositionModifyZChecked(tilemap, &entity->pos, -1);
        }
    }

    // Facing direction checks
    const Vec2 velocity{ entity->velocity };
    if (velocity.x == 0.0f && velocity.y == 0.0f) {
        // Keep previous
    } else if (AbsF32(velocity.x) > AbsF32(velocity.y)) {
        if (velocity.x > 0) {
            entity->facingDir = 3;
        } else {
            entity->facingDir = 1;
        }
    } else {
        if (velocity.y > 0) {
            entity->facingDir = 2;
        } else {
            entity->facingDir = 0;
        }
    }
}

// NOTE: use extern "C" to avoid name mangling
extern "C" UPDATE_AND_RENDER(UpdateAndRender) {
    ASSERT(sizeof(GameState) <= memory->permanentStorageSize);
    // NOTE: this macro depends on the order of the buttons inside InputButtons
    ASSERT(&input->playerInputs[0].Z - &input->playerInputs[0].buttons[0] ==
           ARRAY_COUNT(input->playerInputs[0].buttons) - 1);
    ASSERT(&input->mouseButtons.x2 - &input->mouseButtons.buttons[0] ==
           ARRAY_COUNT(input->mouseButtons.buttons) - 1);

    // TODO: Find another way preferrably
    gThreadContext = threadContext;
    gMemory = memory;

    GameState* gameState{ static_cast<GameState*>(memory->permanentStorage) };
    if (!memory->isInitialized) {
        InitializeGameState(threadContext, gameState, memory);
    }

    const Tilemap* tilemap{ gameState->world->tilemap };

    const f32 delta{ input->frameDeltaTime };

    for (i32 controllerIndex{}; controllerIndex < ARRAY_COUNT(input->playerInputs);
         ++controllerIndex) {
        const InputButtons* inputButtons{ &input->playerInputs[controllerIndex] };
        Entity* controllingEntity{ GetEntity(
            gameState, gameState->playerIndexForController[controllerIndex]) };

        if (controllingEntity->exists) {
            Vec2 acceleration{};

            if (hm_input::ActionPressed(&inputButtons->up)) {
                acceleration.y = 1.0f;
            }
            if (hm_input::ActionPressed(&inputButtons->down)) {
                acceleration.y = -1.0f;
            }
            if (hm_input::ActionPressed(&inputButtons->left)) {
                acceleration.x = -1.0f;
            }
            if (hm_input::ActionPressed(&inputButtons->right)) {
                acceleration.x = 1.0f;
            }

            // Make other players faster
            if (controllerIndex != 0) {
                acceleration *= static_cast<f32>(controllerIndex) * 1.5f;
            }

            // The separation of handling input and moving the player is not yet clear
            MovePlayer(tilemap, controllingEntity, inputButtons, acceleration, delta);

            // Other actions:

            // Only the first player can do certain operations
            // Switching z index
            if (controllerIndex == 0) {
                if (hm_input::ActionJustPressed(&inputButtons->Z)) {
                    if (hm_input::ActionPressed(&inputButtons->shift)) {
                        controllingEntity->pos.absTileZ =
                            TilemapPositionModifyZChecked(tilemap, &controllingEntity->pos, -1);
                    } else {
                        controllingEntity->pos.absTileZ =
                            TilemapPositionModifyZChecked(tilemap, &controllingEntity->pos, 1);
                    }
                }
            }
        } else {
            if (hm_input::ActionJustPressed(&inputButtons->enter) || gameState->startWithAPlayer) {
                if (gameState->startWithAPlayer) {
                    gameState->startWithAPlayer = false;
                }

                DEBUG_PLATFORM_PRINT("New player!\n");
                const i32 entityIndex{ AddEntity(gameState) };
                controllingEntity = GetEntity(gameState, entityIndex);
                InitializePlayer(controllingEntity);
                gameState->playerIndexForController[controllerIndex] = entityIndex;

                // Camera to follow first player
                if (!gameState->cameraFollowingEntityIndex) {
                    gameState->cameraFollowingEntityIndex = entityIndex;
                }
            }
        }
    }

    // IMPORTANT: This now determines the actual pixel size of the tiles!
    constexpr i32 tileSideInPixels{ 60 };
    const f32 metersToPixels{ static_cast<f32>(tileSideInPixels) / tilemap->tileSideInMeters };

    const Entity* cameraFollowingEntity{ GetEntity(gameState,
                                                   gameState->cameraFollowingEntityIndex) };
    if (cameraFollowingEntity->exists) {
        // Camera position
        const TilemapDiff diff{ SubtractTilemapPos(tilemap, &cameraFollowingEntity->pos,
                                                   &gameState->cameraPos) };

        if (diff.x > (9.0f * tilemap->tileSideInMeters)) {
            gameState->cameraPos.absTileX += 17;
        } else if (diff.x < -(9.0f * tilemap->tileSideInMeters)) {
            gameState->cameraPos.absTileX -= 17;
        }

        if (diff.y > (5.0f * tilemap->tileSideInMeters)) {
            gameState->cameraPos.absTileY += 9;
        } else if (diff.y < -(5.0f * tilemap->tileSideInMeters)) {
            gameState->cameraPos.absTileY -= 9;
        }

        gameState->cameraPos.absTileZ = cameraFollowingEntity->pos.absTileZ;
    }

// Debug printing
#if 1
    const Entity* player{ cameraFollowingEntity };
    const TilechunkPosition chunkPos{ GetChunkPosition(
        tilemap, player->pos.absTileX, player->pos.absTileY, player->pos.absTileZ) };

    DEBUG_PLATFORM_PRINT("\n");
    memory->exports.DEBUGPrintUInt(threadContext, "tileChunkX", chunkPos.chunkX);
    memory->exports.DEBUGPrintUInt(threadContext, "tileChunkY", chunkPos.chunkY);
    memory->exports.DEBUGPrintUInt(threadContext, "tileChunkZ", chunkPos.chunkZ);
    memory->exports.DEBUGPrintUInt(threadContext, "chunkRelativeX", chunkPos.chunkRelativeTileX);
    memory->exports.DEBUGPrintUInt(threadContext, "chunkRelativeY", chunkPos.chunkRelativeTileY);

    memory->exports.DEBUGPrintUInt(threadContext, "absTileX", player->pos.absTileX);
    memory->exports.DEBUGPrintUInt(threadContext, "absTileY", player->pos.absTileY);
    memory->exports.DEBUGPrintUInt(threadContext, "absTileZ", player->pos.absTileZ);
    memory->exports.DEBUGPrintFloat(threadContext, "tileRelX", player->pos.tileOffset_.x);
    memory->exports.DEBUGPrintFloat(threadContext, "tileRelY", player->pos.tileOffset_.y);
#endif

    // Background

    DrawBitmap(screenBuff, &gameState->background, 0, 0);

    const Vec2 screenCenter{ static_cast<f32>(screenBuff->width) * 0.5f,
                             static_cast<f32>(screenBuff->height) * 0.5f };

    constexpr i32 relRowCount{ 10 };
    constexpr i32 relColumnCount{ 20 };
    // Drawing tiles
    for (i32 relRow{ -relRowCount }; relRow < relRowCount; ++relRow) {
        for (i32 relColumn{ -relColumnCount }; relColumn < relColumnCount; ++relColumn) {
            // Possibly wraps to U32 max
            const u32 row{ gameState->cameraPos.absTileY + relRow };
            const u32 column{ gameState->cameraPos.absTileX + relColumn };
            const u32 depth{ gameState->cameraPos.absTileZ };

            const u32 tileID{ GetTileValue(tilemap, column, row, depth) };
            if (tileID != blocked_Tile_Value && tileID != 3 && tileID != 4 &&
                tileID != 5 //&&
                            //!(row == gameState->cameraPos.absTileY &&
                            //  column == gameState->cameraPos.absTileX)
            ) {
                continue;
            }

            f32 green{ 1.0f };
            if (tileID == 0) {
                green = 0.0f;
            } else if (tileID == blocked_Tile_Value) {
                green = 0.5f;
            }

            // Highlight the tile for the player controlling the camera
            const Entity* entity{ GetEntity(gameState, gameState->cameraFollowingEntityIndex) };
            if (entity->exists) {
                if (row == entity->pos.absTileY && column == entity->pos.absTileX) {
                    green = 0.25f;
                }
            }

            f32 blue{ 1.0f };
            // On-demand generated
            if (tileID == 3) {
                blue = 0.0f;
            }
            // Door up
            if (tileID == 4) {
                blue = 0.25f;
            }
            // Door down
            if (tileID == 5) {
                blue = 0.5f;
            }

            f32 red{ 0.1f };
            if (depth == 2) {
                red = 0.2f;
            }

            const Vec2 tileCen{ screenCenter.x -
                                    (metersToPixels * gameState->cameraPos.tileOffset_.x) +
                                    (static_cast<f32>(relColumn * tileSideInPixels)),
                                screenCenter.y +
                                    (metersToPixels * gameState->cameraPos.tileOffset_.y) -
                                    (static_cast<f32>(relRow * tileSideInPixels)) };

            const Vec2 tileSide{ (static_cast<f32>(tileSideInPixels) * 0.5f),
                                 (static_cast<f32>(tileSideInPixels) * 0.5f) };

            const Vec2 min{ tileCen - tileSide * 0.9f };
            const Vec2 max{ tileCen + tileSide * 0.9f };

            // Fix a 1 pixel wide vertical black bar that appears sometimes
            const Vec2 minFlooredX{ static_cast<f32>(FloorF32ToI32(min.x)), min.y };
            const Vec2 maxCeiledX{ static_cast<f32>(CeilF32ToI32(max.x)), max.y };

            DrawRectangle(screenBuff, minFlooredX, maxCeiledX, red, green, blue);
        }
    }

    // Drawing players

    Entity* entity{ gameState->entities };
    for (i32 entityIndex{}; entityIndex < gameState->entityCount; ++entityIndex, ++entity) {
        if (!entity->exists) {
            continue;
        }

        const TilemapDiff diff{ SubtractTilemapPos(tilemap, &entity->pos, &gameState->cameraPos) };

        // Real position
        const Vec2 playerGroundPoint{ screenCenter.x + (metersToPixels * diff.x),
                                      screenCenter.y - (metersToPixels * diff.y) };

        constexpr f32 playerR{ 0.5f };
        constexpr f32 playerG{ 0.1f };
        constexpr f32 playerB{ 0.5f };
        // TODO: fix player graphics positioning

        const Vec2 playerPosXY{
            playerGroundPoint.x - (entity->dimensions.x * 0.5f * metersToPixels),
            playerGroundPoint.y - (entity->dimensions.y * 0.5f * metersToPixels)
        };

        const Vec2 maxPos{ playerPosXY.x + (entity->dimensions.x * metersToPixels),
                           playerPosXY.y + (entity->dimensions.y * metersToPixels) };
        // Debug collision box
        DrawRectangle(screenBuff, playerPosXY, maxPos, playerR, playerG, playerB);

        const HeroBitmaps* heroBitmaps{ &gameState->heroBitmaps[entity->facingDir] };
        DrawBitmap(screenBuff, &heroBitmaps->torso, playerGroundPoint.x, playerGroundPoint.y,
                   static_cast<i32>(heroBitmaps->align.x), static_cast<i32>(heroBitmaps->align.y));
        DrawBitmap(screenBuff, &heroBitmaps->head, playerGroundPoint.x, playerGroundPoint.y,
                   static_cast<i32>(heroBitmaps->align.x), static_cast<i32>(heroBitmaps->align.y));
    }
}

extern "C" GET_SOUND_SAMPLES(GetSoundSamples) {
    UNUSED_PARAMS(threadContext);

    GameState* gameState{ static_cast<GameState*>(memory->permanentStorage) };
    OutputSound(gameState, soundBuff);
}
