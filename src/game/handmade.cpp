#include "game/handmade.h"
#include "game/handmade_game.h"

// Any global variables need to be initialized after hot reload (so probably every frame)
GLOBAL ThreadContext* gThreadContext;
GLOBAL GameMemory* gMemory;

// NOTE: just a hacky way to print things from game code
// TODO: think of a better way!
#define PRINT(message) (*gMemory->exports.DEBUGPrint)(gThreadContext, message)
#define PRINT_I32(message, value) (*gMemory->exports.DEBUGPrintInt)(gThreadContext, message, value)
#define PRINT_U32(message, value) (*gMemory->exports.DEBUGPrintUInt)(gThreadContext, message, value)
#define PRINT_F32(message, value)                                                                  \
    (*gMemory->exports.DEBUGPrintFloat)(gThreadContext, message, value)

// Moved back to the original unity build style with no .h declaration and .cpp impl seperation
// That caused more headaches than actually helping development

//#include "game/handmade_intrinsics.cpp"
//#include "game/math/handmade_math.cpp"

//#include "game/handmade_memory.cpp"

//#include "game/handmade_random.cpp"

#include "game/handmade_world.cpp"

#include "game/handmade_sim_region.cpp"

#include "game/handmade_entity.cpp"

//#include "game/handmade_intrinsics.cpp"
//#include "game/handmade_memory.cpp"
//#include "game/handmade_random.cpp"
//#include "game/handmade_sim_region.cpp"
//#include "game/handmade_world.cpp"
//#include "game/math/handmade_math.cpp"

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
           f32 CAlpha = 1.0f) {
    // TODO: never have this case? use a placeholder instead?
    if (!bitmap->pixels) {
        return;
    }

    i32 roundedMinX{ RoundF32ToI32(xPos) };
    i32 roundedMinY{ RoundF32ToI32(yPos) };
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
            f32 alpha{ static_cast<f32>((*src >> 24) & 0xFF) / 255.0f };
            alpha *= CAlpha;

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
 * Maps the entity to camera space
 */
NODISCARD INTERNAL Vec2
GetCameraSpacePos(GameState* gameState, const LowEntity* lowEntity) {
    const WorldDiff diff{ SubtractWorldPos(gameState->world, &lowEntity->pos,
                                           &gameState->cameraPos) };
    const Vec2 result{ diff.x, diff.y };
    return result;
}

struct AddLowEntityResult {
    LowEntity* lowEntity;
    i32 lowIndex;
};

/**
 * Adds an entity to the low entity array
 */
NODISCARD
INTERNAL AddLowEntityResult
AddLowEntity(GameState* gameState, EntityType type, WorldPosition* pos) {
    ASSERT(gameState->lowEntityCount < gameState->lowEntities.size);

    const i32 entityIndex{ gameState->lowEntityCount++ };
    LowEntity* lowEntity{ &gameState->lowEntities[entityIndex] };

    // No need for this maybe
    *lowEntity = LowEntity{};
    lowEntity->sim.type = type;

    ChangeEntityLocation(gameState->world, &gameState->worldArena, entityIndex, lowEntity, nullptr,
                         pos);

    AddLowEntityResult result{ lowEntity, entityIndex };

    return result;
}

INTERNAL void
InitHitpoints(LowEntity* lowEntity, i32 hitPointCount) {
    ASSERT(hitPointCount < lowEntity->sim.hitPoints.size);
    lowEntity->sim.hitPointMax = hitPointCount;

    for (i32 i{}; i < lowEntity->sim.hitPointMax; ++i) {
        HitPoint* hitPoint{ &lowEntity->sim.hitPoints[i] };
        //hitPoint->flags = 0;
        hitPoint->filledAmount = hit_Point_Sub_Count;
    }
}

NODISCARD
INTERNAL AddLowEntityResult
AddSword(GameState* gameState) {
    auto entity{ AddLowEntity(gameState, EntityType::SWORD, nullptr) };
    auto* lowEntity{ entity.lowEntity };

    lowEntity->sim.height = 0.75f;
    lowEntity->sim.width = 0.3f;

    return entity;
}

NODISCARD
INTERNAL AddLowEntityResult
AddPlayer(GameState* gameState) {
    PRINT("New player!\n");

    WorldPosition pos{ gameState->cameraPos };
    auto entity{ AddLowEntity(gameState, EntityType::HERO, &pos) };
    auto* lowEntity{ entity.lowEntity };

    lowEntity->sim.height = 0.5f; // 1.4f;
    lowEntity->sim.width = 0.75f; // entity->dimension.y * 0.75f;

    lowEntity->sim.collides = true;

    InitHitpoints(lowEntity, 3);

    auto sword{ AddSword(gameState) };
    lowEntity->sim.sword.index = sword.lowIndex;

    // Not needed for now
    //EntityToHighFreq(gameState, entityIndex);

    // Camera to follow first player
    if (!gameState->cameraFollowingEntityIndex) {
        gameState->cameraFollowingEntityIndex = entity.lowIndex;
    }

    return entity;
}

NODISCARD
INTERNAL AddLowEntityResult
AddWall(GameState* gameState, i32 tileX, i32 tileY, i32 tileZ) {
    WorldPosition pos{ ChunkPositionFromTilePosition(gameState->world, tileX, tileY, tileZ) };

    auto entity{ AddLowEntity(gameState, EntityType::WALL, &pos) };
    auto* lowEntity{ entity.lowEntity };

    lowEntity->sim.height = gameState->world->tileSideInMeters;
    lowEntity->sim.width = gameState->world->tileSideInMeters;
    lowEntity->sim.collides = true;

    return entity;
}

NODISCARD
INTERNAL AddLowEntityResult
AddMonster(GameState* gameState, i32 tileX, i32 tileY, i32 tileZ) {
    WorldPosition pos{ ChunkPositionFromTilePosition(gameState->world, tileX, tileY, tileZ) };

    auto entity{ AddLowEntity(gameState, EntityType::MONSTER, &pos) };
    auto* lowEntity{ entity.lowEntity };

    lowEntity->sim.height = 0.75f;
    lowEntity->sim.width = 0.6f;
    lowEntity->sim.collides = true;

    InitHitpoints(lowEntity, 3);

    return entity;
}

NODISCARD
INTERNAL AddLowEntityResult
AddFamiliar(GameState* gameState, i32 tileX, i32 tileY, i32 tileZ) {
    WorldPosition pos{ ChunkPositionFromTilePosition(gameState->world, tileX, tileY, tileZ) };

    auto entity{ AddLowEntity(gameState, EntityType::FAMILIAR, &pos) };
    auto* lowEntity{ entity.lowEntity };

    lowEntity->sim.height = 0.5f;
    lowEntity->sim.width = 1.0f;
    lowEntity->sim.collides = true;

    return entity;
}

INTERNAL void
LoadArtAssets(ThreadContext* threadContext, GameState* gameState, GameMemory* memory) {
    // Load the original art assets if one has preordered the game
    // Although with a quick search one can find these on some public repo on Github...

    HeroBitmaps* heroBitmaps{ &gameState->heroBitmaps[0] };

    // NOTE: should come up with a better way of getting the offsets for the correct align

    const auto readFileFunc{ memory->exports.DEBUGReadFile };

    // TODO: This should be made runtime probably
#if HANDMADE_USE_REAL_ASSETS
    gameState->background =
        DEBUGLoadBMP(threadContext, readFileFunc, "original/test/test_background.bmp");

    gameState->tree = DEBUGLoadBMP(threadContext, readFileFunc, "original/test2/tree00.bmp");
    gameState->shadow =
        DEBUGLoadBMP(threadContext, readFileFunc, "original/test/test_hero_shadow.bmp");

    gameState->sword = DEBUGLoadBMP(threadContext, readFileFunc, "original/test2/rock03.bmp");

    heroBitmaps->head =
        DEBUGLoadBMP(threadContext, readFileFunc, "original/test/test_hero_front_head.bmp");
    heroBitmaps->cape =
        DEBUGLoadBMP(threadContext, readFileFunc, "original/test/test_hero_front_cape.bmp");
    heroBitmaps->torso =
        DEBUGLoadBMP(threadContext, readFileFunc, "original/test/test_hero_front_torso.bmp");
    heroBitmaps->align = Vec2{ 72, 182 };
    ++heroBitmaps;

    heroBitmaps->head =
        DEBUGLoadBMP(threadContext, readFileFunc, "original/test/test_hero_left_head.bmp");
    heroBitmaps->cape =
        DEBUGLoadBMP(threadContext, readFileFunc, "original/test/test_hero_left_cape.bmp");
    heroBitmaps->torso =
        DEBUGLoadBMP(threadContext, readFileFunc, "original/test/test_hero_left_torso.bmp");
    heroBitmaps->align = Vec2{ 72, 182 };
    ++heroBitmaps;

    heroBitmaps->head =
        DEBUGLoadBMP(threadContext, readFileFunc, "original/test/test_hero_back_head.bmp");
    heroBitmaps->cape =
        DEBUGLoadBMP(threadContext, readFileFunc, "original/test/test_hero_back_cape.bmp");
    heroBitmaps->torso =
        DEBUGLoadBMP(threadContext, readFileFunc, "original/test/test_hero_back_torso.bmp");
    heroBitmaps->align = Vec2{ 72, 182 };
    ++heroBitmaps;

    heroBitmaps->head =
        DEBUGLoadBMP(threadContext, readFileFunc, "original/test/test_hero_right_head.bmp");
    heroBitmaps->cape =
        DEBUGLoadBMP(threadContext, readFileFunc, "original/test/test_hero_right_cape.bmp");
    heroBitmaps->torso =
        DEBUGLoadBMP(threadContext, readFileFunc, "original/test/test_hero_right_torso.bmp");
    heroBitmaps->align = Vec2{ 72, 182 };

#else

    gameState->background =
        DEBUGLoadBMP(threadContext, readFileFunc, "handmade/test/test_background.bmp");

    // TODO: these just fail because we don't have a the bitmaps yet
    // Doesn't crash the game though
    gameState->tree = DEBUGLoadBMP(threadContext, readFileFunc, "handmade/test2/tree.bmp");
    gameState->shadow = DEBUGLoadBMP(threadContext, readFileFunc, "handmade/test/shadow.bmp");

    //gameState->sword = DEBUGLoadBMP(threadContext, readFileFunc, "handmade/test2/sword.bmp");

    heroBitmaps->head =
        DEBUGLoadBMP(threadContext, readFileFunc, "handmade/test/player_head_forward.bmp");

    heroBitmaps->cape =
        DEBUGLoadBMP(threadContext, readFileFunc, "handmade/test/player_cape_placeholder.bmp");
    heroBitmaps->torso =
        DEBUGLoadBMP(threadContext, readFileFunc, "handmade/test/player_torso_forward.bmp");
    heroBitmaps->align = Vec2{ 48, 100 };
    ++heroBitmaps;

    heroBitmaps->head =
        DEBUGLoadBMP(threadContext, readFileFunc, "handmade/test/player_head_left.bmp");
    heroBitmaps->cape =
        DEBUGLoadBMP(threadContext, readFileFunc, "handmade/test/player_cape_placeholder.bmp");
    heroBitmaps->torso =
        DEBUGLoadBMP(threadContext, readFileFunc, "handmade/test/player_torso_left.bmp");
    heroBitmaps->align = Vec2{ 46, 104 };
    ++heroBitmaps;

    heroBitmaps->head =
        DEBUGLoadBMP(threadContext, readFileFunc, "handmade/test/player_head_backward.bmp");
    heroBitmaps->cape =
        DEBUGLoadBMP(threadContext, readFileFunc, "handmade/test/player_cape_placeholder.bmp");
    heroBitmaps->torso =
        DEBUGLoadBMP(threadContext, readFileFunc, "handmade/test/player_torso_backward.bmp");
    heroBitmaps->align = Vec2{ 42, 100 };
    ++heroBitmaps;

    heroBitmaps->head =
        DEBUGLoadBMP(threadContext, readFileFunc, "handmade/test/player_head_right.bmp");
    heroBitmaps->cape =
        DEBUGLoadBMP(threadContext, readFileFunc, "handmade/test/player_cape_placeholder.bmp");
    heroBitmaps->torso =
        DEBUGLoadBMP(threadContext, readFileFunc, "handmade/test/player_torso_right.bmp");
    heroBitmaps->align = Vec2{ 44, 104 };
#endif
}

INTERNAL void
InitializeGameState(ThreadContext* threadContext, GameState* gameState, GameMemory* memory) {
    // TODO: maybe make platform set this
    memory->isInitialized = true;

    // NOTE: reserve slot 0 for null entity
    // TODO: consider removing if there is no use case as this has caused a bit of problems with
    // all sorts of stuff
    const auto nullEntity{ AddLowEntity(gameState, EntityType::NON_EXISTENT, nullptr) };

    // Changed to false after initializing one player
    gameState->startWithAPlayer = true;

    LoadArtAssets(threadContext, gameState, memory);

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
                    const auto wall{ AddWall(gameState, absTileX, absTileY, absTileZ) };
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
        PRINT_I32("Walls added: ", wallsAdded);
    }

    const i32 cameraTileX{ screenBaseX * tiles_Per_Width + (tiles_Per_Width / 2) };
    const i32 cameraTileY{ screenBaseY * tiles_Per_Height + (tiles_Per_Height / 2) };
    const i32 cameraTileZ{ screenBaseZ };

    // Add other entities

    AddMonster(gameState, cameraTileX + 2, cameraTileY, cameraTileZ);

    //constexpr i32 familiarCount{ 1 }; // 10

    //for (i32 i{}; i < familiarCount; ++i) {
    //    const i32 familiarOffsetX{ (hm_random::randomNumbers[randomNumIndex++] % 10) - 7 };
    //    const i32 familiarOffsetY{ (hm_random::randomNumbers[randomNumIndex++] % 10) - 3 };
    //    if (familiarOffsetX && familiarOffsetY) {
    //        AddFamiliar(gameState, cameraTileX + familiarOffsetX, cameraTileY + familiarOffsetY,
    //                    cameraTileZ);
    //    }
    //}

    AddFamiliar(gameState, cameraTileX - 2, cameraTileY + 1, cameraTileZ);

    // Atm SetCamera has to be called at the end if there is no player at the start
    // This is because we don't call SetCamera after this function if there is no player
    //WorldPosition cameraPos{ ChunkPositionFromTilePosition(world, cameraTileX, cameraTileY,
    //                                                       cameraTileZ) };
    //SetCamera(gameState, cameraPos);
}

INTERNAL void
PushPiece(EntityVisiblePieceGroup* group, LoadedBitmapInfo* bitmap, Vec2 offset, f32 offsetZ,
          Vec2 align, Vec2 dimension, Vec4 color, f32 entityZC = 1.0f) {
    ASSERT(group->pieceCount < group->pieces.size);
    EntityVisiblePiece* piece{ &group->pieces[group->pieceCount++] };

    piece->bitmap = bitmap;
    piece->offset = (group->gameState->metersToPixels * Vec2{ offset.x, -offset.y }) - align;
    piece->offsetZ = group->gameState->metersToPixels * offsetZ;
    piece->entityZC = entityZC;

    piece->dimension = dimension;

    piece->r = color.r;
    piece->g = color.g;
    piece->b = color.b;
    piece->a = color.a;
}

INTERNAL void
PushBitmap(EntityVisiblePieceGroup* group, LoadedBitmapInfo* bitmap, Vec2 offset, f32 offsetZ,
           Vec2 align, f32 alpha = 1.0f, f32 entityZC = 1.0f) {
    PushPiece(group, bitmap, offset, offsetZ, align, Vec2{}, Vec4{ 1.0f, 1.0f, 1.0f, alpha },
              entityZC);
}

INTERNAL void
PushRect(EntityVisiblePieceGroup* group, Vec2 offset, f32 offsetZ, Vec2 dimension, Vec4 color,
         f32 entityZC = 1.0f) {
    PushPiece(group, 0, offset, offsetZ, Vec2{}, dimension, color, entityZC);
}

INTERNAL void
DrawHitpoints(SimEntity* entity, EntityVisiblePieceGroup* group) {
    if (entity->hitPointMax >= 1) {
        constexpr Vec2 hitPointdimension{ 0.2f, 0.2f };
        constexpr f32 spacingX{ hitPointdimension.x * 1.5f };
        Vec2 hitPointPos{ -0.5f * (entity->hitPointMax - 1) * spacingX, -0.25f };
        const Vec2 dPos{ spacingX, 0.0f };

        for (i32 i{}; i < entity->hitPointMax; ++i) {
            HitPoint* hitPoint{ &entity->hitPoints[i] };
            Vec4 color{ 1.0f, 0.0f, 0.0f, 1.0f };
            if (hitPoint->filledAmount == 0) {
                color = Vec4{ 0.2f, 0.2f, 0.2f, 1.0f };
            }

            // TODO: Height
            PushRect(group, hitPointPos, 0, hitPointdimension, color, 0.0f);
            hitPointPos += dPos;
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

    World* world{ gameState->world };

    const f32 delta{ input->frameDeltaTime };

    for (i32 controllerIndex{}; controllerIndex < ARRAY_COUNT(input->playerInputs);
         ++controllerIndex) {
        const InputButtons* inputButtons{ &input->playerInputs[controllerIndex] };
        ControlledHero* controlled{ &gameState->controlledHeroes[controllerIndex] };
        if (controlled->entityIndex == 0) {
            if (hm_input::ActionJustPressed(&inputButtons->enter) || gameState->startWithAPlayer) {
                if (gameState->startWithAPlayer) {
                    gameState->startWithAPlayer = false;
                }

                *controlled = {};
                const auto player{ AddPlayer(gameState) };
                controlled->entityIndex = player.lowIndex;
            }
        } else {
            controlled->ddP = {};
            // Need to check the type as we have the null entity...
            //if (controllingEntity.low->type != EntityType::NON_EXISTENT) {
            if (hm_input::ActionPressed(&inputButtons->up)) {
                controlled->ddP.y = 1.0f;
            }
            if (hm_input::ActionPressed(&inputButtons->down)) {
                controlled->ddP.y = -1.0f;
            }
            if (hm_input::ActionPressed(&inputButtons->left)) {
                controlled->ddP.x = -1.0f;
            }
            if (hm_input::ActionPressed(&inputButtons->right)) {
                controlled->ddP.x = 1.0f;
            }

            // Jump
            if (hm_input::ActionJustPressed(&inputButtons->space)) {
                if (controlled->dZ == 0.0f) {
                    controlled->dZ = 3.0f;
                }
            }

            controlled->dSword = {};
            if (hm_input::ActionJustPressed(&inputButtons->actionUp)) {
                controlled->dSword.y = 1.0f;
            }
            if (hm_input::ActionJustPressed(&inputButtons->actionDown)) {
                controlled->dSword.y = -1.0f;
            }
            if (hm_input::ActionJustPressed(&inputButtons->actionLeft)) {
                controlled->dSword.x = -1.0f;
            }
            if (hm_input::ActionJustPressed(&inputButtons->actionRight)) {
                controlled->dSword.x = 1.0f;
            }

            // The separation of handling input and moving the player is not yet clear
            //MoveEntity(gameState, controllingEntity, controllerIndex, inputButtons,
            //           acceleration, delta);

            // TODO: disabled for now
            // Only the first player can do certain operations
            // Switching z index
            //if (controllerIndex == 0) {
            //    if (hm_input::ActionJustPressed(&inputButtons->Z)) {
            //        if (hm_input::ActionPressed(&inputButtons->shift)) {
            //            controllingEntity.low->pos.chunkZ =
            //                WorldPositionModifyZChecked(world, &controllingEntity.low->pos, -1);
            //        } else {
            //            controllingEntity.low->pos.chunkZ =
            //                WorldPositionModifyZChecked(world, &controllingEntity.low->pos, 1);
            //        }
            //    }
            //}
            //}
        }
    }

    // IMPORTANT: This now determines the actual pixel size of the tiles!
    constexpr i32 tileSideInPixels{ 60 };
    gameState->metersToPixels = static_cast<f32>(tileSideInPixels) / world->tileSideInMeters;

    // Simulate regions

    constexpr i32 tileSpanX{ tiles_Per_Width * 3 };
    constexpr i32 tileSpanY{ tiles_Per_Height * 3 };
    const Rect cameraBounds{ RectCenterDim(
        Vec2{}, Vec2{ static_cast<f32>(tileSpanX), static_cast<f32>(tileSpanY) } *
                    gameState->world->tileSideInMeters) };

    MemoryArena simArena;
    InitializeArena(&simArena, memory->transientStorage, memory->transientStorageSize);
    auto* simRegion{ BeginSimulation(gameState, &gameState->worldArena, gameState->world,
                                     gameState->cameraPos, cameraBounds) };

// Debug printing
#if 0
    const Entity player{ cameraFollowingEntity };
    if (player.high) {
        PRINT("\n");

        PRINT_U32("chunkX: ", player.low->pos.chunkX);
        PRINT_U32("chunkY: ", player.low->pos.chunkY);
        PRINT_U32("chunkZ: ", player.low->pos.chunkZ);

        //PRINT_U32("chunkRelativeX: ", chunkPos.chunkRelativeTileX);
        //PRINT_U32("chunkRelativeY: ", chunkPos.chunkRelativeTileY);

        PRINT_F32("tileRelX: ", player.low->pos.offset_.x);
        PRINT_F32("tileRelY: ", player.low->pos.offset_.y);
    }

#endif

    // Background

#if 0
    DrawBitmap(screenBuff, &gameState->background, 0, 0);
#else
    DrawRectangle(screenBuff, Vec2{},
                  Vec2{ static_cast<f32>(screenBuff->width), static_cast<f32>(screenBuff->height) },
                  0.5f, 0.5f, 0.5f);
#endif

    // Drawing entities

    const Vec2 screenCenter{ screenBuff->width * 0.5f, screenBuff->height * 0.5f };

    // Every entity has its own one of these
    EntityVisiblePieceGroup pieceGroup;
    pieceGroup.gameState = gameState;

    auto* entity{ simRegion->entities };
    for (i32 i{}; i < simRegion->entityCount; ++i, ++entity) {
        pieceGroup.pieceCount = 0;

        // TODO: This is wrong, compute after update
        f32 shadowAlpha{ 1.0f - 0.5f * entity->z };
        if (shadowAlpha < 0) {
            shadowAlpha = 0.0f;
        }

        HeroBitmaps* heroBitmaps{ &gameState->heroBitmaps[entity->facingDir] };

        switch (entity->type) {
        case EntityType::WALL: {
            // Tree bitmaps
            PushBitmap(&pieceGroup, &gameState->tree, Vec2{}, 0, Vec2{ 40, 80 });
        } break;

        case EntityType::HERO: {
            for (i32 controlIndex{}; controlIndex < ARRAY_COUNT(gameState->controlledHeroes);
                 ++controlIndex) {
                auto* controlled{ &gameState->controlledHeroes[controlIndex] };
                if (entity->storageIndex == controlled->entityIndex) {
                    MoveSpec moveSpec{ DefaultMoveSpec() };
                    moveSpec.speed = 30.0f;
                    moveSpec.drag = 4.0f;
                    moveSpec.unitMaxAccelVector = true;
                    MoveEntity(simRegion, entity, moveSpec, controlled->ddP, delta);

                    /// Other actions ///

                    // Sword
                    if (controlled->dSword != Vec2::ZERO) {
                        //const i32 swordIndex{ controllingEntity.low->swordIndex };
                        //auto lowSword{ GetLowEntity(gameState, swordIndex) };
                        SimEntity* sword{ entity->sword.ptr };
                        if (sword) {
                            PRINT("Used sword!\n");
                            sword->pos = entity->pos;
                            sword->distanceRemaining = 6.0f;
                            sword->velocity = controlled->dSword * 8.0f;
                        }
                    }
                }
            }

            PushBitmap(&pieceGroup, &gameState->shadow, Vec2{}, 0, heroBitmaps->align, shadowAlpha,
                       0.0f);
            PushBitmap(&pieceGroup, &heroBitmaps->torso, Vec2{}, 0, heroBitmaps->align);
            PushBitmap(&pieceGroup, &heroBitmaps->cape, Vec2{}, 0, heroBitmaps->align);
            PushBitmap(&pieceGroup, &heroBitmaps->head, Vec2{}, 0, heroBitmaps->align);

            DrawHitpoints(entity, &pieceGroup);
        } break;

        case EntityType::MONSTER: {
            UpdateMonster(gameState, entity, delta);
            PushBitmap(&pieceGroup, &gameState->shadow, Vec2{}, 0, heroBitmaps->align, shadowAlpha,
                       0.0f);
            PushBitmap(&pieceGroup, &heroBitmaps->torso, Vec2{}, 0, heroBitmaps->align);

            DrawHitpoints(entity, &pieceGroup);
        } break;
        case EntityType::FAMILIAR: {
            UpdateFamiliar(simRegion, entity, delta);

            // Head bob
            constexpr f32 bobSpeed{ 3.5f };
            entity->tBob += delta * bobSpeed;
            if (entity->tBob > (2.0f * PI32f)) {
                entity->tBob -= (2.0f * PI32f);
            }

            const f32 bobSin{ Sin(entity->tBob) };
            const f32 newShadowAlpha{ (shadowAlpha * 0.5f) + (0.15f * bobSin) };
            const f32 bobStrength{ 0.23f }; // How big the bobbing is

            PushBitmap(&pieceGroup, &gameState->shadow, Vec2{}, 0, heroBitmaps->align,
                       newShadowAlpha, 0.0f);
            PushBitmap(&pieceGroup, &heroBitmaps->head, Vec2{}, bobStrength * bobSin,
                       heroBitmaps->align);
        } break;
        case EntityType::SWORD: {
            UpdateSword(simRegion, entity, delta);

            PushBitmap(&pieceGroup, &gameState->shadow, Vec2{}, 0, heroBitmaps->align, shadowAlpha,
                       0.0f);
            PushBitmap(&pieceGroup, &gameState->sword, Vec2{}, 0, Vec2{ 29, 10 });
        } break;

        default: {
            INVALID_CODE_PATH;
        } break;
        }

        const f32 gravity{ -9.8f };
        entity->z = 0.5f * gravity * SquareF32(delta) + entity->dZ * delta + entity->z;
        entity->dZ = gravity * delta + entity->dZ;
        if (entity->z < 0.0f) {
            entity->z = 0.0f;
        }

        const Vec2 entityGroundPoint{ screenCenter.x + (gameState->metersToPixels * entity->pos.x),
                                      screenCenter.y -
                                          (gameState->metersToPixels * entity->pos.y) };
        const f32 entityZ{ -entity->z * gameState->metersToPixels };

        constexpr f32 r{ 0.5f };
        constexpr f32 g{ 0.1f };
        constexpr f32 b{ 0.5f };

        const Vec2 leftTop{
            entityGroundPoint.x - (0.5f * gameState->metersToPixels * entity->width),
            entityGroundPoint.y - (0.5f * gameState->metersToPixels * entity->height)
        };

        const Vec2 entityWidthHeight{ entity->width, entity->height };

        // Draw pieces
        for (i32 pieceIndex{}; pieceIndex < pieceGroup.pieceCount; ++pieceIndex) {
            EntityVisiblePiece* piece{ &pieceGroup.pieces[pieceIndex] };
            const Vec2 center{ entityGroundPoint.x + piece->offset.x,
                               entityGroundPoint.y + piece->offset.y + piece->offsetZ +
                                   (entityZ * piece->entityZC) };
            if (piece->bitmap) {
                DrawBitmap(screenBuff, piece->bitmap, center.x, center.y, piece->a);
            } else {
                const Vec2 halfDim{ 0.5f * piece->dimension * gameState->metersToPixels };
                DrawRectangle(screenBuff, center - halfDim, center + halfDim, piece->r, piece->g,
                              piece->b);
            }

            // Debug collision box
            //DrawRectangle(screenBuff, leftTop,
            //              leftTop + entityWidthHeight * gameState->metersToPixels // *0.95f
            //              ,
            //              r, g, b);
        }
    }

    WorldPosition worldOrigin{};
    WorldDiff diff{ SubtractWorldPos(simRegion->world, &worldOrigin, &simRegion->origin) };
    DrawRectangle(screenBuff, Vec2{ diff.x, diff.y }, Vec2{ 10.0f, 10.0f }, 1.0f, 1.0f, 1.0f);

    EndSimulation(simRegion, gameState);
}

extern "C" GET_SOUND_SAMPLES(GetSoundSamples) {
    UNUSED_PARAMS(threadContext);

    const GameState* gameState{ static_cast<GameState*>(memory->permanentStorage) };
    OutputSound(gameState, soundBuff);
}
