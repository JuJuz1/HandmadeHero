#include "game/handmade.h"

#include "game/handmade_game.h"

// Here we just include every .cpp file (basically every file if they are not bundled into one)
// The order doesn't matter as we seperated .h files to contain every dependency they need
#include "game/handmade_intrinsics.cpp"
#include "game/handmade_memory.cpp"
#include "game/handmade_random.cpp"
#include "game/handmade_world.cpp"
#include "game/math/handmade_math.cpp"

// Any global variables need to be initialized after hot reload (so probably every frame)
GLOBAL ThreadContext* gThreadContext;
GLOBAL GameMemory* gMemory;

// NOTE: just a hacky way to print things from game code
// TODO: think of a better way!
#define PRINT(message) (*gMemory->exports.DEBUGPrint)(gThreadContext, message)
#define PRINT_INT(message, value) (*gMemory->exports.DEBUGPrintInt)(gThreadContext, message, value)
#define PRINT_UINT(message, value)                                                                 \
    (*gMemory->exports.DEBUGPrintUInt)(gThreadContext, message, value)
#define PRINT_FLOAT(message, value)                                                                \
    (*gMemory->exports.DEBUGPrintFloat)(gThreadContext, message, value)

namespace hm_input {

/**
 * Returns true if the button was just pressed during the frame
 */
NODISCARD
INTERNAL bool32
ActionJustPressed(const Button* button) {
    const bool32 result{ button->endedDown && button->halfTransitionCount > 0 };
    return result;
}

/**
 * Returns true if the button was pressed during the frame
 * As this returns true for the first frame as well, ActionJustPressed and ActionPressed both return
 * true for the first frame for the same button
 */
NODISCARD
INTERNAL bool32
ActionPressed(const Button* button) {
    const bool32 result{ button->endedDown };
    return result;
}

/**
 * Returns true if the button was just released during the frame
 */
NODISCARD
INTERNAL bool32
ActionReleased(const Button* button) {
    const bool32 result{ !button->endedDown && button->halfTransitionCount > 0 };
    return result;
}

} //namespace hm_input

/**
 * Write the sound data to buff
 */
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
                // TODO: episode 52, use rotateleft and right?
                *srcDest++ = ((((C >> alphaShift.index) & 0xFF) << 24) |
                              (((C >> redShift.index) & 0xFF) << 16) |
                              (((C >> greenShift.index) & 0xFF) << 8) |
                              (((C >> blueShift.index) & 0xFF) << 0));
            }
        }
    } else {
        PRINT("Couldn't load bmp!\n");
    }

    return result;
}

INTERNAL void
DrawBitmap(const OffScreenBuffer* screenBuff, const LoadedBitmapInfo* bitmap, f32 xPos, f32 yPos,
           i32 alignX = 0, i32 alignY = 0) {
    const Vec2 aligned{ xPos - static_cast<f32>(alignX), yPos - static_cast<f32>(alignY) };

    i32 roundedMinX{ RoundF32ToI32(aligned.x) };
    i32 roundedMinY{ RoundF32ToI32(aligned.y) };
    i32 roundedMaxX{ roundedMinX + bitmap->width };
    i32 roundedMaxY{ roundedMinY + bitmap->height };

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

/**
 * If entityIndex is out of bounds, returns a 0 pointer
 */
NODISCARD
INTERNAL LowEntity*
GetLowEntity(GameState* gameState, i32 lowIndex) {
    ASSERT(lowIndex >= 0 && lowIndex < gameState->lowEntityCount);

    LowEntity* lowEntity{};

    if (lowIndex >= 0 && lowIndex < gameState->lowEntityCount) {
        lowEntity = &gameState->lowEntities[lowIndex];
    }

    return lowEntity;
}

/**
 * Makes the entity low frequency
 * Decrements gameState->highEntityCount
 */
INTERNAL void
EntityToLowFreq(GameState* gameState, i32 entityIndex) {
    ASSERT(entityIndex);
    LowEntity* lowEntity{ GetLowEntity(gameState, entityIndex) };
    const i32 highIndex{ lowEntity->highEntityIndex };
    ASSERT(highIndex);                      // TODO: Null entity??
    ASSERT(gameState->highEntityCount > 1); // Null entity...?

    if (highIndex) {
        const i32 lastHighIndex{ gameState->highEntityCount - 1 };
        // If not last one -> fill the gap
        if (highIndex != lastHighIndex) {
            const HighEntity* lastHighEntity{ &gameState->highEntities_[lastHighIndex] };
            HighEntity* removedEntity{ &gameState->highEntities_[highIndex] };

            *removedEntity = *lastHighEntity;
            gameState->lowEntities[lastHighEntity->lowEntityIndex].highEntityIndex = highIndex;
        }
    }

    lowEntity->highEntityIndex = 0;
    --gameState->highEntityCount;
}

/**
 * Maps the entity to camera space
 */
NODISCARD
INTERNAL Vec2
GetCameraSpacePos(GameState* gameState, const LowEntity* lowEntity) {
    const WorldDiff diff{ SubtractWorldPos(gameState->world, &lowEntity->pos,
                                           &gameState->cameraPos) };
    const Vec2 result{ diff.x, diff.y };
    return result;
}

/**
 * A lower level call, assumes the entity is not in high frequency
 *
 * Makes the entity high frequency by mapping it to camera space
 * Doesn't do anything if the entity already is in the high frequency array
 */
NODISCARD
INTERNAL HighEntity*
EntityToHighFreq_(GameState* gameState, LowEntity* lowEntity, i32 lowIndex, Vec2 cameraSpacePos) {
    HighEntity* highEntity{};

    ASSERT(lowEntity->highEntityIndex == 0);

    // Already is high freq
    if (lowEntity->highEntityIndex == 0) {
        if (gameState->highEntityCount < gameState->highEntities_.size) {
            const i32 highIndex{ gameState->highEntityCount++ };
            lowEntity->highEntityIndex = highIndex;
            highEntity = &gameState->highEntities_[highIndex];

            highEntity->pos = cameraSpacePos;
            highEntity->velocity = Vec2{};
            highEntity->chunkZ = lowEntity->pos.chunkZ;
            highEntity->facingDir = 0;

            highEntity->lowEntityIndex = lowIndex;

            lowEntity->highEntityIndex = highIndex;
        } else {
            INVALID_CODE_PATH;
        }
    }

    return highEntity;
}

NODISCARD
INTERNAL HighEntity*
EntityToHighFreq(GameState* gameState, i32 lowIndex) {
    HighEntity* highEntity{};

    LowEntity* lowEntity{ GetLowEntity(gameState, lowIndex) };
    if (lowEntity->highEntityIndex) {
        highEntity = &gameState->highEntities_[lowEntity->highEntityIndex];
    } else {
        const Vec2 cameraSpacePos{ GetCameraSpacePos(gameState, lowEntity) };
        highEntity = EntityToHighFreq_(gameState, lowEntity, lowIndex, cameraSpacePos);
    }

    return highEntity;
}

/**
 * Returns an entity struct containing pointers to high and low entities
 * If entityIndex is out of bounds or 0, returns a zero-initialized entity
 */
NODISCARD
INTERNAL Entity
GetHighEntity(GameState* gameState, i32 lowIndex) {
    //ASSERT(lowIndex >= 0 && lowIndex < gameState->highEntityCount);

    Entity entity{};

    if (lowIndex >= 0 && lowIndex < gameState->lowEntityCount) {
        entity.low = GetLowEntity(gameState, lowIndex);
        entity.high = EntityToHighFreq(gameState, lowIndex);
        entity.lowIndex = lowIndex;
    }

    return entity;
}

/**
 * Adds an entity to the low entity array
 */
NODISCARD
INTERNAL i32
AddLowEntity(GameState* gameState, EntityType type, WorldPosition* pos) {
    ASSERT(gameState->lowEntityCount < gameState->lowEntities.size);

    const i32 entityIndex{ gameState->lowEntityCount++ };
    LowEntity* lowEntity{ &gameState->lowEntities[entityIndex] };

    // No need for this maybe
    *lowEntity = LowEntity{};
    lowEntity->type = type;

    if (pos) {
        lowEntity->pos = *pos;
        ChangeEntityLocation(gameState->world, &gameState->worldArena, entityIndex, nullptr, pos);
    }

    return entityIndex;
}

NODISCARD
INTERNAL i32
AddPlayer(GameState* gameState) {
    PRINT("New player!\n");

    WorldPosition pos{ gameState->cameraPos };
    const i32 entityIndex{ AddLowEntity(gameState, EntityType::HERO, &pos) };
    LowEntity* lowEntity{ GetLowEntity(gameState, entityIndex) };

    lowEntity->height = 0.5f; // 1.4f;
    lowEntity->width = 0.75f; // entity->dimensions.y * 0.75f;

    lowEntity->collides = true;

    // Not needed for now
    //EntityToHighFreq(gameState, entityIndex);

    // Camera to follow first player
    if (!gameState->cameraFollowingEntityIndex) {
        gameState->cameraFollowingEntityIndex = entityIndex;
    }

    return entityIndex;
}

NODISCARD
INTERNAL i32
AddWall(GameState* gameState, i32 tileX, i32 tileY, i32 tileZ) {
    WorldPosition pos{ ChunkPositionFromTilePosition(gameState->world, tileX, tileY, tileZ) };

    const i32 entityIndex{ AddLowEntity(gameState, EntityType::WALL, &pos) };
    LowEntity* lowEntity{ GetLowEntity(gameState, entityIndex) };

    lowEntity->height = gameState->world->tileSideInMeters;
    lowEntity->width = gameState->world->tileSideInMeters;
    lowEntity->collides = true;

    return entityIndex;
}

/**
 * Debug function
 */
NODISCARD
INTERNAL bool32
ValidateEntityPairs_(GameState* gameState) {
    bool32 valid{ true };

    for (i32 highIndex{ 1 }; highIndex < gameState->highEntityCount; ++highIndex) {
        HighEntity* highEntity{ &gameState->highEntities_[highIndex] };
        LowEntity* lowEntity{ GetLowEntity(gameState, highEntity->lowEntityIndex) };
        valid = valid && lowEntity->highEntityIndex == highIndex;
    }

    return valid;
}

INTERNAL void
OffsetAndCheckFrequencyByArea(GameState* gameState, Rect highFreqBounds,
                              Vec2 entityOffsetFromCamera) {
    i32 movedCount{};

    for (i32 highIndex{ 1 }; highIndex < gameState->highEntityCount;) {
        HighEntity* highEntity{ &gameState->highEntities_[highIndex] };
        highEntity->pos += entityOffsetFromCamera;

        if (!IsInsideRectangle(highFreqBounds, highEntity->pos)) {
            ASSERT(gameState->lowEntities[highEntity->lowEntityIndex].highEntityIndex == highIndex);
            EntityToLowFreq(gameState, highEntity->lowEntityIndex);
            ++movedCount;
        } else {
            ++highIndex;
        }
    }

    if (movedCount > 0) {
        PRINT_INT("Entities moved to low: ", movedCount);
    }
}

INTERNAL void
SetCamera(GameState* gameState, WorldPosition newCameraPos) {
    ASSERT(ValidateEntityPairs_(gameState));

    const WorldDiff diffCameraPos{ SubtractWorldPos(gameState->world, &newCameraPos,
                                                    &gameState->cameraPos) };
    const Vec2 entityOffsetFromCamera{ -diffCameraPos.x, -diffCameraPos.y };
    gameState->cameraPos = newCameraPos;

    constexpr i32 tileSpanX{ tiles_Per_Width * 3 };
    constexpr i32 tileSpanY{ tiles_Per_Height * 3 };
    const Rect cameraBounds{ RectCenterDim(
        Vec2{}, Vec2{ static_cast<f32>(tileSpanX), static_cast<f32>(tileSpanY) } *
                    gameState->world->tileSideInMeters) };

    // Add the offset to every high frequency entity when the camera moves
    // TODO: doesn't work with second player, walls work correctly
    OffsetAndCheckFrequencyByArea(gameState, cameraBounds, entityOffsetFromCamera);

    ASSERT(ValidateEntityPairs_(gameState));

    const WorldPosition minChunk{ MapIntoChunkSpace(gameState->world, newCameraPos,
                                                    cameraBounds.min) };
    const WorldPosition maxChunk{ MapIntoChunkSpace(gameState->world, newCameraPos,
                                                    cameraBounds.max) };

    i32 movedCount{};

    // Check entities by chunk, move to high set if in the chunks close to camera
    for (i32 chunkY{ minChunk.chunkY }; chunkY <= maxChunk.chunkY; ++chunkY) {
        for (i32 chunkX{ minChunk.chunkX }; chunkX <= maxChunk.chunkX; ++chunkX) {
            WorldChunk* chunk{ GetWorldChunk(gameState->world, chunkX, chunkY, newCameraPos.chunkZ,
                                             nullptr) };
            if (chunk) {
                for (WorldEntityBlock* block{ &chunk->firstBlock }; block; block = block->next) {
                    for (i32 entityIndex{}; entityIndex < block->entityCount; ++entityIndex) {
                        const i32 lowEntityIndex{ block->lowEntityIndexes[entityIndex] };
                        LowEntity* lowEntity{ GetLowEntity(gameState, lowEntityIndex) };
                        if (lowEntity->highEntityIndex) {
                            continue;
                        }

                        const Vec2 cameraSpacePos{ GetCameraSpacePos(gameState, lowEntity) };

                        if (IsInsideRectangle(cameraBounds, cameraSpacePos)) {
                            // TODO: Unused return
                            EntityToHighFreq_(gameState, lowEntity, lowEntityIndex, cameraSpacePos);
                            ++movedCount;
                        }
                    }
                }
            }
        }
    }

    if (movedCount > 0) {
        PRINT_INT("Entities moved to high: ", movedCount);
    }

    ASSERT(ValidateEntityPairs_(gameState));
}

INTERNAL void
InitializeGameState(ThreadContext* threadContext, GameState* gameState, GameMemory* memory) {
    // TODO: maybe make platform set this
    memory->isInitialized = true;

    // NOTE: reserve slot 0 for null entity
    // TODO: consider removing if there is no use case as this has caused a bit of problems with
    // all sorts of stuff
    const i32 nullEntityIndex{ AddLowEntity(gameState, EntityType::NON_EXISTENT, nullptr) };
    gameState->highEntityCount = 1;

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
    ++heroBitmaps;

    heroBitmaps->head =
        DEBUGLoadBMP(threadContext, memory->exports.DEBUGReadFile, "test/player_head_left.bmp");
    heroBitmaps->torso =
        DEBUGLoadBMP(threadContext, memory->exports.DEBUGReadFile, "test/player_torso_left.bmp");
    heroBitmaps->align = Vec2{ 46, 104 };
    ++heroBitmaps;

    heroBitmaps->head =
        DEBUGLoadBMP(threadContext, memory->exports.DEBUGReadFile, "test/player_head_backward.bmp");
    heroBitmaps->torso = DEBUGLoadBMP(threadContext, memory->exports.DEBUGReadFile,
                                      "test/player_torso_backward.bmp");
    heroBitmaps->align = Vec2{ 42, 100 };
    ++heroBitmaps;

    heroBitmaps->head =
        DEBUGLoadBMP(threadContext, memory->exports.DEBUGReadFile, "test/player_head_right.bmp");
    heroBitmaps->torso =
        DEBUGLoadBMP(threadContext, memory->exports.DEBUGReadFile, "test/player_torso_right.bmp");
    heroBitmaps->align = Vec2{ 44, 104 };

    InitializeArena(&gameState->worldArena,
                    static_cast<u8*>(memory->permanentStorage) + sizeof(GameState),
                    memory->permanentStorageSize - sizeof(GameState));

    gameState->world = PushSize(&gameState->worldArena, World);
    World* world{ gameState->world };
    InitializeWorld(world, 1.4f);

    u32 randomNumIndex{};

    bool32 doorLeft{};
    bool32 doorRight{};
    bool32 doorTop{};
    bool32 doorBottom{};

    bool32 doorUp{};
    bool32 doorDown{};

    const i32 screenBaseX{};
    const i32 screenBaseY{};
    const i32 screenBaseZ{};

    i32 screenX{ screenBaseX };
    i32 screenY{ screenBaseY };
    i32 absTileZ{ screenBaseZ };

    i32 wallsAdded{};

    // How many rooms to create
    constexpr i32 screenCount{ 50 };

    // Generating tile values
    for (u32 screen{}; screen < screenCount; ++screen) {
        ASSERT(randomNumIndex < hm_random::randomNumbers.size);
        u32 randomChoice;
        // Lateral only
        //if (doorUp || doorDown) {
        // TODO: remove comments
        randomChoice = hm_random::randomNumbers[randomNumIndex++] % 2;
        //} else {
        //    randomChoice = hm_random::randomNumbers[randomNumIndex++] % 3;
        //}

        bool32 createdZDoor{};
        // randomChoice of 2 means the room is blocked and has a door going up
        // Atm this logic means we can only have 2 layers (z of 0 or 1)
        if (randomChoice == 2) {
            createdZDoor = true;
            if (absTileZ == screenBaseZ) {
                doorUp = true;
            } else {
                doorDown = true;
            }
        } else if (randomChoice == 1) {
            doorRight = true;
        } else {
            doorTop = true;
        }

        for (i32 tileY{}; tileY < tiles_Per_Height; ++tileY) {
            for (i32 tileX{}; tileX < tiles_Per_Width; ++tileX) {
                const i32 absTileX{ (screenX * tiles_Per_Width) + tileX };
                const i32 absTileY{ (screenY * tiles_Per_Height) + tileY };

                u32 tileValue{ 2 };
                if (tileX == 0 && (!doorLeft || (tileY != (tiles_Per_Height / 2)))) {
                    tileValue = blocked_Tile_Value;
                }
                if (tileX == (tiles_Per_Width - 1) &&
                    (!doorRight || (tileY != (tiles_Per_Height / 2)))) {
                    tileValue = blocked_Tile_Value;
                }
                if (tileY == 0 && (!doorBottom || (tileX != tiles_Per_Width / 2))) {
                    tileValue = blocked_Tile_Value;
                }
                if (tileY == (tiles_Per_Height - 1) &&
                    (!doorTop || (tileX != tiles_Per_Width / 2))) {
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

                if (tileValue == blocked_Tile_Value) {
                    const i32 entityIndex{ AddWall(gameState, absTileX, absTileY, absTileZ) };
                    ++wallsAdded;
                }
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
            if (absTileZ == screenBaseZ) {
                absTileZ += 1;
            } else {
                absTileZ = screenBaseZ;
            }
        }
        // Advance screens if we didn't make a vertical floor (door)
        else if (randomChoice == 1) {
            ++screenX;
        } else {
            ++screenY;
        }
    }

    if (wallsAdded > 0) {
        PRINT_INT("Walls added: ", wallsAdded);
    }

    WorldPosition cameraPos{ ChunkPositionFromTilePosition(
        world, screenBaseX * tiles_Per_Width + (tiles_Per_Width / 2),
        screenBaseY * tiles_Per_Height + (tiles_Per_Height / 2), screenBaseZ) };
    SetCamera(gameState, cameraPos);
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
MovePlayer(GameState* gameState, Entity entity, i32 controllerIndex,
           const InputButtons* inputButtons, Vec2 acceleration, f32 delta) {
    constexpr f32 playerSpeed{ 30 };
    constexpr f32 playerSpeedModifier{ 4 };

    // Normalize if greater than unit circle length of 1
    const f32 accelerationLengthSq{ LengthSquared(acceleration) };
    if (accelerationLengthSq > 1.0f) {
        acceleration *= (1.0f / Sqrt(accelerationLengthSq));
    }

    // Other player faster for debug
    if (controllerIndex != 0) {
        acceleration *= 1.5f;
    }

    acceleration *= playerSpeed;

    if (hm_input::ActionPressed(&inputButtons->shift)) {
        acceleration *= playerSpeedModifier;
    }

    // TODO: ordinary differential equations
    acceleration += -4.0f * entity.high->velocity;
    // v' = at + v
    entity.high->velocity += acceleration * delta;

    // p' = 0.5 * at^2 + vt + p
    Vec2 playerDelta{ (0.5f * acceleration * SquareF32(delta)) + (entity.high->velocity * delta) };

    // Collision checks

    //bool32 hitWall{}; // Used to modify velocity if we hit a wall during the frame

    constexpr i32 iterationCount{ 4 };
    for (i32 iteration{}; iteration < iterationCount; ++iteration) {
        f32 tMin{ 1.0f };
        Vec2 wallNormal{};
        TestWallResult testWallResult{};
        i32 hitHighEntityIndex{};

        const Vec2 desiredPos{ entity.high->pos + playerDelta };

        for (i32 highIndex{ 1 }; highIndex < gameState->highEntityCount; ++highIndex) {
            // FIXME wrong indexing, FIXED I THINK
            const HighEntity* highEntity{ &gameState->highEntities_[highIndex] };
            const Entity testEntity{ GetHighEntity(gameState, highEntity->lowEntityIndex) };
            // Check if collides and don't compare to self!
            if (!entity.low->collides || !testEntity.low->collides ||
                testEntity.high == entity.high) {
                continue;
            }

            const f32 diameterWidth{ testEntity.low->width + entity.low->width };
            const f32 diameterHeight{ testEntity.low->height + entity.low->height };
            const Vec2 minCorner{ Vec2{ -diameterWidth, -diameterHeight } * 0.5f };
            const Vec2 maxCorner{ Vec2{ diameterWidth, diameterHeight } * 0.5f };

            const Vec2 relPos{ entity.high->pos - testEntity.high->pos };

            // Test all four walls

            // x
            testWallResult = TestWall(minCorner.x, relPos.x, relPos.y, playerDelta.x, playerDelta.y,
                                      tMin, minCorner.y, maxCorner.y);
            if (testWallResult.hit) {
                tMin = testWallResult.tMin;
                wallNormal = Vec2{ -1, 0 };
                //hitWall = true;
                hitHighEntityIndex = highIndex;
            }

            testWallResult = TestWall(maxCorner.x, relPos.x, relPos.y, playerDelta.x, playerDelta.y,
                                      tMin, minCorner.y, maxCorner.y);
            if (testWallResult.hit) {
                tMin = testWallResult.tMin;
                wallNormal = Vec2{ 1, 0 };
                //hitWall = true;
                hitHighEntityIndex = highIndex;
            }

            // y
            testWallResult = TestWall(minCorner.y, relPos.y, relPos.x, playerDelta.y, playerDelta.x,
                                      tMin, minCorner.x, maxCorner.x);
            if (testWallResult.hit) {
                tMin = testWallResult.tMin;
                wallNormal = Vec2{ 0, -1 };
                //hitwall = true;
                hitHighEntityIndex = highIndex;
            }

            testWallResult = TestWall(maxCorner.y, relPos.y, relPos.x, playerDelta.y, playerDelta.x,
                                      tMin, minCorner.x, maxCorner.x);
            if (testWallResult.hit) {
                tMin = testWallResult.tMin;
                wallNormal = Vec2{ 0, 1 };
                //hitwall = true;
                hitHighEntityIndex = highIndex;
            }
        }

        //PRINT_FLOAT("tMin: ", tMin);

        entity.high->pos += playerDelta * tMin;
        if (hitHighEntityIndex) {
            entity.high->velocity -= 1.0f * Dot(entity.high->velocity, wallNormal) * wallNormal;
            playerDelta = desiredPos - entity.high->pos;
            playerDelta -= 1.0f * Dot(playerDelta, wallNormal) * wallNormal;

            const Entity hitEntity{ GetHighEntity(gameState, hitHighEntityIndex) };
            const LowEntity* hitLow{ GetLowEntity(gameState, hitEntity.lowIndex) };

            // Door check
            const World* world{ gameState->world };
            entity.low->pos.chunkZ =
                WorldPositionModifyZChecked(world, &entity.low->pos, hitLow->dChunkZ);
        } else {
            break;
        }
    }

    // Delta independent friction using exponential decay: e^(-kt)
    //if (hitWall) {
    //    constexpr f32 frictionModifier{ 2.0f };
    //    const f32 friction{ ExpF32(-frictionModifier * delta) };
    //    entity->velocity *= friction;
    //}

    // Facing direction checks
    const Vec2 velocity{ entity.high->velocity };
    if (velocity.x == 0.0f && velocity.y == 0.0f) {
        // Keep previous
    } else if (AbsF32(velocity.x) > AbsF32(velocity.y)) {
        if (velocity.x > 0) {
            entity.high->facingDir = 3;
        } else {
            entity.high->facingDir = 1;
        }
    } else {
        if (velocity.y > 0) {
            entity.high->facingDir = 2;
        } else {
            entity.high->facingDir = 0;
        }
    }

    WorldPosition newPos{ MapIntoChunkSpace(gameState->world, gameState->cameraPos,
                                            entity.high->pos) };
    // TODO: bundle these together?
    ChangeEntityLocation(gameState->world, &gameState->worldArena, entity.lowIndex,
                         &entity.low->pos, &newPos);
    entity.low->pos = newPos;

    //entity.low->pos = MapIntoChunkSpace(gameState->world, gameState->cameraPos, entity.high->pos);
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

    const World* world{ gameState->world };

    const f32 delta{ input->frameDeltaTime };

    for (i32 controllerIndex{}; controllerIndex < ARRAY_COUNT(input->playerInputs);
         ++controllerIndex) {
        const InputButtons* inputButtons{ &input->playerInputs[controllerIndex] };
        const i32 lowIndex{ gameState->playerIndexFromController[controllerIndex] };
        if (!lowIndex) {
            if (hm_input::ActionJustPressed(&inputButtons->enter) || gameState->startWithAPlayer) {
                if (gameState->startWithAPlayer) {
                    gameState->startWithAPlayer = false;
                }

                const i32 entityIndex{ AddPlayer(gameState) };
                gameState->playerIndexFromController[controllerIndex] = entityIndex;
            }
        } else {
            const Entity controllingEntity{ GetHighEntity(gameState, lowIndex) };
            // Need to check the type as we have the null entity...
            if (controllingEntity.low->type != EntityType::NON_EXISTENT) {
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

                // The separation of handling input and moving the player is not yet clear
                MovePlayer(gameState, controllingEntity, controllerIndex, inputButtons,
                           acceleration, delta);

                // Other actions:

                // Only the first player can do certain operations
                // Switching z index
                if (controllerIndex == 0) {
                    if (hm_input::ActionJustPressed(&inputButtons->Z)) {
                        if (hm_input::ActionPressed(&inputButtons->shift)) {
                            controllingEntity.low->pos.chunkZ =
                                WorldPositionModifyZChecked(world, &controllingEntity.low->pos, -1);
                        } else {
                            controllingEntity.low->pos.chunkZ =
                                WorldPositionModifyZChecked(world, &controllingEntity.low->pos, 1);
                        }
                    }
                }
            }
        }
    }

    // IMPORTANT: This now determines the actual pixel size of the tiles!
    constexpr i32 tileSideInPixels{ 60 };
    const f32 metersToPixels{ static_cast<f32>(tileSideInPixels) / world->tileSideInMeters };

    // Camera position
    const Entity cameraFollowingEntity{ GetHighEntity(gameState,
                                                      gameState->cameraFollowingEntityIndex) };
    if (cameraFollowingEntity.high) {
        WorldPosition newCameraPos{ gameState->cameraPos };

#if 1
        if (cameraFollowingEntity.high->pos.x > (9.0f * world->tileSideInMeters)) {
            newCameraPos.chunkX += tiles_Per_Width;
        } else if (cameraFollowingEntity.high->pos.x < -(9.0f * world->tileSideInMeters)) {
            newCameraPos.chunkX -= tiles_Per_Width;
        }

        if (cameraFollowingEntity.high->pos.y > (5.0f * world->tileSideInMeters)) {
            newCameraPos.chunkY += tiles_Per_Height;
        } else if (cameraFollowingEntity.high->pos.y < -(5.0f * world->tileSideInMeters)) {
            newCameraPos.chunkY -= tiles_Per_Height;
        }
#else
        // Tile snap scrolling
        if (cameraFollowingEntity.high->pos.x > (1.0f * world->tileSideInMeters)) {
            newCameraPos.chunkX += 1;
        } else if (cameraFollowingEntity.high->pos.x < -(1.0f * world->tileSideInMeters)) {
            newCameraPos.chunkX -= 1;
        }

        if (cameraFollowingEntity.high->pos.y > (1.0f * world->tileSideInMeters)) {
            newCameraPos.chunkY += 1;
        } else if (cameraFollowingEntity.high->pos.y < -(1.0f * world->tileSideInMeters)) {
            newCameraPos.chunkY -= 1;
        }
#endif

        newCameraPos.chunkZ = cameraFollowingEntity.low->pos.chunkZ;

#if 1
        // Fully smooth scrolling
        newCameraPos = cameraFollowingEntity.low->pos;
#endif

        SetCamera(gameState, newCameraPos);

        // TODO: map new entities in and old entities out
        // TODO: map tiles and stairs (doors)
    }

// Debug printing
#if 0
    const Entity player{ cameraFollowingEntity };
    const WorldChunkPosition_ chunkPos{ GetChunkPosition(
        world, player.low->pos.chunkX, player.low->pos.chunkY, player.low->pos.chunkZ) };

    PRINT("\n");
    PRINT_UINT("tileChunkX: ", chunkPos.chunkX);
    PRINT_UINT("tileChunkY: ", chunkPos.chunkY);
    PRINT_UINT("tileChunkZ: ", chunkPos.chunkZ);
    PRINT_UINT("chunkRelativeX: ", chunkPos.chunkRelativeTileX);
    PRINT_UINT("chunkRelativeY: ", chunkPos.chunkRelativeTileY);

    PRINT_UINT("chunkX: ", player.low->pos.chunkX);
    PRINT_UINT("chunkY: ", player.low->pos.chunkY);
    PRINT_UINT("chunkZ: ", player.low->pos.chunkZ);
    PRINT_FLOAT("tileRelX: ", player.low->pos.offset_.x);
    PRINT_FLOAT("tileRelY: ", player.low->pos.offset_.y);
#endif

    // Background

    DrawBitmap(screenBuff, &gameState->background, 0, 0);

    const Vec2 screenCenter{ static_cast<f32>(screenBuff->width) * 0.5f,
                             static_cast<f32>(screenBuff->height) * 0.5f };

#if 0
    constexpr i32 relRowCount{ 10 };
    constexpr i32 relColumnCount{ 20 };
    // Drawing tiles
    for (i32 relRow{ -relRowCount }; relRow < relRowCount; ++relRow) {
        for (i32 relColumn{ -relColumnCount }; relColumn < relColumnCount; ++relColumn) {
            // Possibly wraps to U32 max
            const u32 row{ gameState->cameraPos.chunkY + relRow };
            const u32 column{ gameState->cameraPos.chunkX + relColumn };
            const u32 depth{ gameState->cameraPos.chunkZ };

            const u32 tileID{ GetTileValue(world, column, row, depth) };
            if (tileID != blocked_Tile_Value && tileID != 3 && tileID != 4 &&
                tileID != 5 //&&
                            //!(row == gameState->cameraPos.chunkY &&
                            //  column == gameState->cameraPos.chunkX)
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
            const Entity entity{ GetEntity(gameState, gameState->cameraFollowingEntityIndex,
                                           EntityResidency::LOW) };
            if (entity.residence != EntityResidency::NON_EXISTENT) {
                if (row == entity.low->pos.chunkY && column == entity.low->pos.chunkX) {
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
                                    (metersToPixels * gameState->cameraPos.offset_.x) +
                                    (static_cast<f32>(relColumn * tileSideInPixels)),
                                screenCenter.y +
                                    (metersToPixels * gameState->cameraPos.offset_.y) -
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
#endif

    // Drawing entities

    for (i32 highIndex{ 1 }; highIndex < gameState->highEntityCount; ++highIndex) {
        const HighEntity* highEntity{ &gameState->highEntities_[highIndex] };
        ASSERT(highEntity->lowEntityIndex); // Must have index to low freq if in the high freq

        const LowEntity* lowEntity{ GetLowEntity(gameState, highEntity->lowEntityIndex) };
        ASSERT(lowEntity->highEntityIndex); // Also must be in the high freq

        // Real position
        const Vec2 playerGroundPoint{ screenCenter.x + (metersToPixels * highEntity->pos.x),
                                      screenCenter.y - (metersToPixels * highEntity->pos.y) };

        // TODO: fix player graphics positioning
        const Vec2 playerPosXY{ playerGroundPoint.x - (lowEntity->width * 0.5f * metersToPixels),
                                playerGroundPoint.y - (lowEntity->height * 0.5f * metersToPixels) };

        const Vec2 maxPos{ playerPosXY.x + (lowEntity->width * metersToPixels),
                           playerPosXY.y + (lowEntity->height * metersToPixels) };

        constexpr f32 playerR{ 0.5f };
        constexpr f32 playerG{ 0.1f };
        constexpr f32 playerB{ 0.5f };
        if (lowEntity->type == EntityType::HERO) {
            // Debug collision box
            DrawRectangle(screenBuff, playerPosXY, maxPos, playerR, playerG, playerB);

            const HeroBitmaps* heroBitmaps{ &gameState->heroBitmaps[highEntity->facingDir] };
            DrawBitmap(screenBuff, &heroBitmaps->torso, playerGroundPoint.x, playerGroundPoint.y,
                       static_cast<i32>(heroBitmaps->align.x),
                       static_cast<i32>(heroBitmaps->align.y));
            DrawBitmap(screenBuff, &heroBitmaps->head, playerGroundPoint.x, playerGroundPoint.y,
                       static_cast<i32>(heroBitmaps->align.x),
                       static_cast<i32>(heroBitmaps->align.y));
        } else {
            DrawRectangle(screenBuff, playerPosXY, maxPos, playerR, playerG, playerB);
        }
    }
}

extern "C" GET_SOUND_SAMPLES(GetSoundSamples) {
    UNUSED_PARAMS(threadContext);

    const GameState* gameState{ static_cast<GameState*>(memory->permanentStorage) };
    OutputSound(gameState, soundBuff);
}
