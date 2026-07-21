#include "game/handmade.h"
#include "game/handmade_game.h"

// Moved back to the original unity build style with no .h declaration and .cpp impl seperation
// That caused more headaches than actually helping development

//#include "game/handmade_intrinsics.cpp"
//#include "game/math/handmade_math.cpp"

//#include "game/handmade_memory.cpp"

//#include "game/handmade_random.cpp"

//#include "game/handmade_intrinsics.cpp"
//#include "game/handmade_memory.cpp"
//#include "game/handmade_random.cpp"
//#include "game/handmade_sim_region.cpp"
//#include "game/handmade_world.cpp"
//#include "game/math/handmade_math.cpp"

/// Original unity build

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

// clang-format off
#include "game/handmade_world.cpp"
#include "game/handmade_sim_region.cpp"
#include "game/handmade_entity.cpp"
// clang-format on

/**
 * Write the sound data to buff
 */
INTERNAL void
OutputSound(const GameState* gameState, const SoundOutputBuffer* buff) {
    UNUSED_PARAMS(gameState, buff);
}

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

struct AddLowEntityResult {
    LowEntity* lowEntity;
    i32 lowIndex;
};

/**
 * Adds an entity to the low entity array
 */
//NODISCARD
INTERNAL AddLowEntityResult
AddLowEntity(GameState* gameState, EntityType type, WorldPosition pos) {
    ASSERT(gameState->lowEntityCount < gameState->lowEntities.size);

    const i32 entityIndex{ gameState->lowEntityCount++ };
    auto* lowEntity{ &gameState->lowEntities[entityIndex] };

    // TODO: No need to clear maybe
    *lowEntity = LowEntity{};
    lowEntity->sim.type = type;
    lowEntity->sim.collision = gameState->nullCollision;
    lowEntity->startingPos = pos;
    lowEntity->pos = NullWorldPos();

    ChangeEntityLocation(gameState->world, &gameState->worldArena, entityIndex, lowEntity, pos);

    AddLowEntityResult result{ lowEntity, entityIndex };

    return result;
}

NODISCARD
INTERNAL AddLowEntityResult
AddGroundedEntity(GameState* gameState, EntityType type, WorldPosition pos,
                  SimEntityCollisionVolumeGroup* collision) {
    auto entity{ AddLowEntity(gameState, type, pos) };
    entity.lowEntity->sim.collision = collision;
    return entity;
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
    auto sword{ AddLowEntity(gameState, EntityType::SWORD, NullWorldPos()) };
    auto* lowEntity{ sword.lowEntity };

    PRINT_I32("New sword: ", sword.lowIndex);

    lowEntity->sim.collision = gameState->swordCollision;
    // TODO: needed?
    AddFlags(&lowEntity->sim, SimEntityFlags::NON_SPATIAL | SimEntityFlags::MOVEABLE);

    return sword;
}

NODISCARD
INTERNAL AddLowEntityResult
AddPlayer(GameState* gameState) {
    const auto pos{ gameState->cameraPos };
    const auto player{ AddGroundedEntity(gameState, EntityType::HERO, pos,
                                         gameState->heroCollision) };
    auto* lowEntity{ player.lowEntity };
    PRINT_I32("New player: ", player.lowIndex);

    AddFlags(&lowEntity->sim, SimEntityFlags::COLLIDES | SimEntityFlags::MOVEABLE);

    InitHitpoints(lowEntity, 3);

    auto sword{ AddSword(gameState) };
    lowEntity->sim.sword.index = sword.lowIndex;

    // We add the rule every time the sword is used
    //AddCollisionRule(gameState, player.lowIndex, sword.lowIndex, false);

    // Not needed for now
    //EntityToHighFreq(gameState, entityIndex);

    // Camera to follow first player
    if (!gameState->cameraFollowingEntityIndex) {
        gameState->cameraFollowingEntityIndex = player.lowIndex;
    }

    return player;
}

//NODISCARD
INTERNAL AddLowEntityResult
AddStair(GameState* gameState, i32 tileX, i32 tileY, i32 tileZ) {
    auto pos{ ChunkPositionFromTilePosition(gameState->world, tileX, tileY, tileZ) };
    auto stair{ AddGroundedEntity(gameState, EntityType::STAIRWELL, pos,
                                  gameState->stairwellCollision) };
    auto* lowEntity{ stair.lowEntity };

    AddFlags(&lowEntity->sim, SimEntityFlags::COLLIDES);
    lowEntity->sim.walkableDim = lowEntity->sim.collision->totalVolume.dim.xy;
    lowEntity->sim.walkableHeight = gameState->world->tileDepthInMeters;

    return stair;
}

//NODISCARD
INTERNAL AddLowEntityResult
AddWall(GameState* gameState, i32 tileX, i32 tileY, i32 tileZ) {
    auto pos{ ChunkPositionFromTilePosition(gameState->world, tileX, tileY, tileZ) };
    auto wall{ AddGroundedEntity(gameState, EntityType::WALL, pos, gameState->wallCollision) };
    auto* lowEntity{ wall.lowEntity };

    AddFlags(&lowEntity->sim, SimEntityFlags::COLLIDES);

    return wall;
}

//NODISCARD
INTERNAL AddLowEntityResult
AddMonster(GameState* gameState, i32 tileX, i32 tileY, i32 tileZ) {
    auto pos{ ChunkPositionFromTilePosition(gameState->world, tileX, tileY, tileZ) };
    auto monster{ AddGroundedEntity(gameState, EntityType::MONSTER, pos,
                                    gameState->monsterCollision) };

    auto* lowEntity{ monster.lowEntity };
    AddFlags(&lowEntity->sim, SimEntityFlags::COLLIDES | SimEntityFlags::MOVEABLE);

    InitHitpoints(lowEntity, 5);

    return monster;
}

//NODISCARD
INTERNAL AddLowEntityResult
AddFamiliar(GameState* gameState, i32 tileX, i32 tileY, i32 tileZ) {
    auto pos{ ChunkPositionFromTilePosition(gameState->world, tileX, tileY, tileZ) };
    auto familiar{ AddGroundedEntity(gameState, EntityType::FAMILIAR, pos,
                                     gameState->familiarCollision) };

    auto* lowEntity{ familiar.lowEntity };
    AddFlags(&lowEntity->sim, SimEntityFlags::COLLIDES | SimEntityFlags::MOVEABLE);

    // TODO: change to true when we get the stop follow to work
    lowEntity->sim.followingHero = false;

    return familiar;
}

INTERNAL void
LoadArtAssets(ThreadContext* threadContext, GameState* gameState, GameMemory* memory) {
    // Load the original art assets if one has preordered the game
    // Although with a quick search one can find these on some public repo on Github...

    HeroBitmaps* heroBitmaps{ &gameState->heroBitmaps[0] };

    // NOTE: should come up with a better way of getting the offsets for the correct align

    const auto readFileFunc{ memory->exports.DEBUGReadFile };

    // TODO: This should really be a runtime property...
#if HANDMADE_USE_REAL_ASSETS
    gameState->background =
        DEBUGLoadBMP(threadContext, readFileFunc, "original/test/test_background.bmp");

    gameState->tree = DEBUGLoadBMP(threadContext, readFileFunc, "original/test2/tree00.bmp");
    gameState->shadow =
        DEBUGLoadBMP(threadContext, readFileFunc, "original/test/test_hero_shadow.bmp");

    gameState->stairwell = DEBUGLoadBMP(threadContext, readFileFunc, "original/test2/rock02.bmp");

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

NODISCARD
INTERNAL SimEntityCollisionVolumeGroup*
MakeSimpleCollision(GameState* gameState, f32 dimX, f32 dimY, f32 dimZ) {
    auto* collision{ PushSize(&gameState->worldArena, SimEntityCollisionVolumeGroup) };
    collision->volumeCount = 1;
    collision->volumes = PushArray(&gameState->worldArena, 1, SimEntityCollisionVolume);
    collision->totalVolume.dim = Vec3{ dimX, dimY, dimZ };
    collision->totalVolume.offsetPos = Vec3{ 0, 0, 0.5f * dimZ };
    collision->volumes[0] = collision->totalVolume;

    return collision;
}

NODISCARD
INTERNAL SimEntityCollisionVolumeGroup*
MakeNullCollision(GameState* gameState) {
    auto* collision{ PushSize(&gameState->worldArena, SimEntityCollisionVolumeGroup) };
    collision->volumeCount = 0;
    collision->volumes = nullptr;
    collision->totalVolume.offsetPos = {};
    // Negative?
    collision->totalVolume.dim = {};
    return collision;
}

INTERNAL AddLowEntityResult
AddStandardRoom(GameState* gameState, i32 absTileX, i32 absTileY, i32 absTileZ) {
    auto pos{ ChunkPositionFromTilePosition(gameState->world, absTileX, absTileY, absTileZ) };
    auto entity{ AddGroundedEntity(gameState, EntityType::SPACE, pos,
                                   gameState->standardRoomCollision) };
    AddFlags(&entity.lowEntity->sim, SimEntityFlags::TRAVERSABLE);

    return entity;
}

INTERNAL void
InitializeGameState(ThreadContext* threadContext, GameState* gameState, GameMemory* memory) {
    InitializeArena(&gameState->worldArena,
                    static_cast<u8*>(memory->permanentStorage) + sizeof(GameState),
                    memory->permanentStorageSize - sizeof(GameState));

    gameState->world = PushSize(&gameState->worldArena, World);
    World* world{ gameState->world };
    InitializeWorld(world, 1.4f, 3.0f);

    // NOTE: reserve slot 0 for null entity
    // TODO: consider removing if there is no use case as this has caused a bit of problems with
    // all sorts of stuff
    AddLowEntity(gameState, EntityType::NON_EXISTENT, NullWorldPos());

    gameState->nullCollision = MakeNullCollision(gameState);
    gameState->swordCollision = MakeSimpleCollision(gameState, 0.3f, 0.75f, 0.1f);
    gameState->stairwellCollision = MakeSimpleCollision(
        gameState, gameState->world->tileSideInMeters, gameState->world->tileSideInMeters * 2.0f,
        gameState->world->tileDepthInMeters * 1.1f);
    gameState->monsterCollision = MakeSimpleCollision(gameState, 1.0f, 0.5f, 0.5f);
    gameState->familiarCollision = MakeSimpleCollision(gameState, 1.0f, 0.5f, 0.5f);
    gameState->heroCollision = MakeSimpleCollision(gameState, 1.0f, 0.5f, 0.5f);
    gameState->wallCollision = MakeSimpleCollision(gameState, gameState->world->tileSideInMeters,
                                                   gameState->world->tileSideInMeters,
                                                   gameState->world->tileDepthInMeters);
    gameState->standardRoomCollision =
        MakeSimpleCollision(gameState, gameState->world->tileSideInMeters * tiles_Per_Width,
                            gameState->world->tileSideInMeters * tiles_Per_Height,
                            gameState->world->tileDepthInMeters * 0.9f);

    // Changed to false after initializing one player
    gameState->startWithAPlayer = true;
    gameState->allowUnlimitedJumps = true;

    LoadArtAssets(threadContext, gameState, memory);

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
    i32 stairsAdded{};

    // How many rooms to create
    constexpr i32 screenCount{ 50 };

    // Generating tile values
    for (u32 screen{}; screen < screenCount; ++screen) {
        ASSERT(randomNumIndex < hm_random::randomNumbers.size);
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

#if 1
        AddStandardRoom(gameState, (screenX * tiles_Per_Width) + (tiles_Per_Width / 2),
                        (screenY * tiles_Per_Height) + (tiles_Per_Height / 2), absTileZ);
#endif

        for (i32 tileY{}; tileY < tiles_Per_Height; ++tileY) {
            for (i32 tileX{}; tileX < tiles_Per_Width; ++tileX) {
                const i32 absTileX{ (screenX * tiles_Per_Width) + tileX };
                const i32 absTileY{ (screenY * tiles_Per_Height) + tileY };

                // Door
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

                if (tileValue == blocked_Tile_Value) {
                    const auto wall{ AddWall(gameState, absTileX, absTileY, absTileZ) };
                    ++wallsAdded;
                } else if (createdZDoor) {
                    if (tileX == 10 && tileY == 5) {
                        AddStair(gameState, absTileX, absTileY, doorDown ? absTileZ - 1 : absTileZ);
                        ++stairsAdded;
                    }
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

    PRINT_I32("Walls added: ", wallsAdded);
    PRINT_I32("Stairs added: ", stairsAdded);

    const i32 cameraTileX{ screenBaseX * tiles_Per_Width + (tiles_Per_Width / 2) };
    const i32 cameraTileY{ screenBaseY * tiles_Per_Height + (tiles_Per_Height / 2) };
    const i32 cameraTileZ{ screenBaseZ };
    WorldPosition newCameraPos{ ChunkPositionFromTilePosition(world, cameraTileX, cameraTileY,
                                                              cameraTileZ) };
    gameState->cameraPos = newCameraPos;

    // Add other entities

    AddMonster(gameState, cameraTileX + 4, cameraTileY, cameraTileZ);

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

    // TODO: maybe make platform set this
    memory->isInitialized = true;
}

INTERNAL void
PushPiece(EntityVisiblePieceGroup* group, LoadedBitmapInfo* bitmap, Vec2 offset, f32 offsetZ,
          Vec2 align, Vec2 dimension, Vec4 color, f32 entityZC = 1.0f) {
    ASSERT(group->pieceCount < group->pieces.size);
    EntityVisiblePiece* piece{ &group->pieces[group->pieceCount++] };

    piece->bitmap = bitmap;
    piece->offset = (group->gameState->metersToPixels * Vec2{ offset.x, -offset.y }) - align;
    piece->offsetZ = offsetZ;
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
PushRect(EntityVisiblePieceGroup* group, Vec2 offset, f32 offsetZ, Vec2 dim, Vec4 color,
         f32 entityZC = 1.0f) {
    PushPiece(group, nullptr, offset, offsetZ, Vec2{}, dim, color, entityZC);
}

INTERNAL void
PushRectOutline(EntityVisiblePieceGroup* group, Vec2 offset, f32 offsetZ, Vec2 dim, Vec4 color,
                f32 entityZC = 1.0f) {
    const f32 thickness{ 0.1f };

    // Top bottom
    PushPiece(group, 0, offset - Vec2{ 0, dim.y * 0.5f }, offsetZ, Vec2{}, Vec2{ dim.x, thickness },
              color, entityZC);
    PushPiece(group, 0, offset + Vec2{ 0, dim.y * 0.5f }, offsetZ, Vec2{}, Vec2{ dim.x, thickness },
              color, entityZC);

    // Left right
    PushPiece(group, 0, offset - Vec2{ dim.x * 0.5f, 0 }, offsetZ, Vec2{}, Vec2{ thickness, dim.y },
              color, entityZC);
    PushPiece(group, 0, offset + Vec2{ dim.x * 0.5f, 0 }, offsetZ, Vec2{}, Vec2{ thickness, dim.y },
              color, entityZC);
}

INTERNAL void
ClearCollisionRulesFor(GameState* gameState, i32 storageIndex) {
    PRINT("ClearCollisionRulesFor\n");
    PRINT_I32("Index: ", storageIndex);

    // @Speed
    // TODO: better way to remove collision rather than walking through the whole map!
    // Storing A,B and B,A would allow easy removal, see ep 70 for full explanation
    for (i32 hashBucket{}; hashBucket < gameState->collisionRuleHash.size; ++hashBucket) {
        for (auto** rule{ &gameState->collisionRuleHash[hashBucket] }; *rule;) {
            if ((*rule)->storageIndexA == storageIndex || (*rule)->storageIndexB == storageIndex) {
                auto* removedRule{ *rule };
                *rule = (*rule)->nextInHash;

                removedRule->nextInHash = gameState->firstFreeCollisionRule;
                gameState->firstFreeCollisionRule = removedRule;
            } else {
                rule = &(*rule)->nextInHash;
            }
        }
    }
}

INTERNAL void
AddCollisionRule(GameState* gameState, i32 storageIndexA, i32 storageIndexB, bool32 canCollide) {
    PRINT("AddCollisionRule\n");
    PRINT_I32("A: ", storageIndexA);
    PRINT_I32("B: ", storageIndexB);
    PRINT_I32("Collide: ", canCollide);

    if (storageIndexA > storageIndexB) {
        const i32 temp{ storageIndexA };
        storageIndexA = storageIndexB;
        storageIndexB = temp;
    }

    PairWiseCollisionRule* found{};
    // TODO: Better hash func
    const i32 hashBucket{ static_cast<i32>(storageIndexA &
                                           (gameState->collisionRuleHash.size - 1)) };
    for (auto* rule{ gameState->collisionRuleHash[hashBucket] }; rule; rule = rule->nextInHash) {
        if (rule->storageIndexA == storageIndexA && rule->storageIndexB == storageIndexB) {
            found = rule;
            break;
        }
    }

    if (!found) {
        found = gameState->firstFreeCollisionRule;
        if (found) {
            gameState->firstFreeCollisionRule = found->nextInHash;
        } else {
            // Push to head always
            found = PushSize(&gameState->worldArena, PairWiseCollisionRule);
        }

        found->nextInHash = gameState->collisionRuleHash[hashBucket];
        gameState->collisionRuleHash[hashBucket] = found;
    }

    ASSERT(found);
    // Out of memory handling?
    // Update if found earlier
    if (found) {
        found->storageIndexA = storageIndexA;
        found->storageIndexB = storageIndexB;
        found->canCollide = canCollide;
    }
}

INTERNAL void
DrawHitpoints(const SimEntity* entity, EntityVisiblePieceGroup* group) {
    if (entity->hitPointMax >= 1) {
        constexpr Vec2 hitPointdimension{ 0.2f, 0.2f };
        constexpr f32 spacingX{ hitPointdimension.x * 1.5f };
        Vec2 hitPointPos{ -0.5f * (entity->hitPointMax - 1) * spacingX, -0.25f };
        constexpr Vec2 dPos{ spacingX, 0.0f };

        for (i32 i{}; i < entity->hitPointMax; ++i) {
            const HitPoint* hitPoint{ &entity->hitPoints[i] };
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
    ASSERT(&input->playerInputs[0].terminator - &input->playerInputs[0].buttons[0] ==
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

    for (i32 controllerIndex{}; controllerIndex < ARRAY_COUNT(input->playerInputs);
         ++controllerIndex) {
        // Why are we namespacing if we end up doing this...
        using namespace hm_input;

        const auto* buttons{ &input->playerInputs[controllerIndex] };
        auto* controlled{ &gameState->controlledHeroes[controllerIndex] };
        if (controlled->entityIndex == 0) {
            if (ActionJustPressed(&buttons->enter) || gameState->startWithAPlayer) {
                if (gameState->startWithAPlayer) {
                    gameState->startWithAPlayer = false;
                }

                *controlled = {};
                const auto player{ AddPlayer(gameState) };
                controlled->entityIndex = player.lowIndex;
            }
        } else {
            // Be careful not to clear the index
            const i32 entityIndex{ controlled->entityIndex };
            *controlled = {};
            controlled->entityIndex = entityIndex;

            /// Input

            // Need to check the type as we have the null entity...
            //if (controllingEntity.low->type != EntityType::NON_EXISTENT) {
            if (ActionPressed(&buttons->up)) {
                controlled->ddP.y = 1.0f;
            }
            if (ActionPressed(&buttons->down)) {
                controlled->ddP.y = -1.0f;
            }
            if (ActionPressed(&buttons->left)) {
                controlled->ddP.x = -1.0f;
            }
            if (ActionPressed(&buttons->right)) {
                controlled->ddP.x = 1.0f;
            }

            // Jump
            if (ActionJustPressed(&buttons->space)) {
                if (ActionPressed(&buttons->ctrl)) {
                    gameState->allowUnlimitedJumps = !gameState->allowUnlimitedJumps;
                    if (gameState->allowUnlimitedJumps) {
                        PRINT("Unlimited jumps!\n");
                    } else {
                        PRINT("No multiple jumps!\n");
                    }
                } else {
                    if (controlled->dZ == 0.0f) {
                        controlled->dZ = 3.0f;
                    }
                }
            }

            // Sprint
            if (ActionPressed(&buttons->shift)) {
                controlled->sprint = true;
            }

            // Reset position if we get stuck
            if (ActionJustPressed(&buttons->R)) {
                // Shift means resetting the sword
                if (ActionPressed(&buttons->shift)) {
                    controlled->requestResetSword = true;
                } else {
                    controlled->requestReset = true;
                }
            }

            if (ActionJustPressed(&buttons->F)) {
                if (ActionPressed(&buttons->shift)) {
                    controlled->requestFamiliarReset = true;
                } else {
                    controlled->requestFamiliarStopFollow = true;
                }
            }

            // Sword
            if (ActionJustPressed(&buttons->actionUp)) {
                controlled->dSword.y = 1.0f;
            }
            if (ActionJustPressed(&buttons->actionDown)) {
                controlled->dSword.y = -1.0f;
            }
            if (ActionJustPressed(&buttons->actionLeft)) {
                controlled->dSword.x = -1.0f;
            }
            if (ActionJustPressed(&buttons->actionRight)) {
                controlled->dSword.x = 1.0f;
            }

            // @Debug
            if (ActionJustPressed(&buttons->right)) {
                if (ActionPressed(&buttons->ctrl)) {
                    gameState->showCollisionBoxes = !gameState->showCollisionBoxes;
                }
            }

            // The separation of handling input and moving the player is not yet clear
            //MoveEntity(gameState, controllingEntity, controllerIndex, inputButtons,
            //           acceleration, delta);

            // TODO: disabled for now
            // Only the first player can do certain operations
            // Switching z index
            //if (controllerIndex == 0) {
            //    if (ActionJustPressed(&inputButtons->Z)) {
            //        if (ActionPressed(&inputButtons->shift)) {
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

    const f32 delta{ input->frameDeltaTime };

    const World* world{ gameState->world };

    // IMPORTANT: This now determines the actual pixel size of the tiles!
    constexpr i32 tileSideInPixels{ 60 };
    gameState->metersToPixels = static_cast<f32>(tileSideInPixels) / world->tileSideInMeters;

    constexpr i32 tileSpanX{ tiles_Per_Width * 3 };
    constexpr i32 tileSpanY{ tiles_Per_Height * 3 };
    constexpr i32 tileSpanZ{ 1 };
    const Rect3 cameraBounds{ RectCenterDim(Vec3{}, gameState->world->tileSideInMeters *
                                                        Vec3{ static_cast<f32>(tileSpanX),
                                                              static_cast<f32>(tileSpanY),
                                                              static_cast<f32>(tileSpanZ) }) };

    MemoryArena simArena;
    InitializeArena(&simArena, memory->transientStorage, memory->transientStorageSize);
    auto* simRegion{ BeginSim(gameState, &simArena, gameState->world, gameState->cameraPos,
                              cameraBounds, delta) };

    // @Debug printing

#if 0
    const auto player{ GetLowEntity(gameState, gameState->cameraFollowingEntityIndex) };
    if (IsValidWorldPos(player->pos)) {
        PRINT("\n");

        PRINT_U32("chunkX: ", player->pos.chunkX);
        PRINT_U32("chunkY: ", player->pos.chunkY);
        PRINT_U32("chunkZ: ", player->pos.chunkZ);

        //PRINT_U32("chunkRelativeX: ", chunkPos.chunkRelativeTileX);
        //PRINT_U32("chunkRelativeY: ", chunkPos.chunkRelativeTileY);

        PRINT_F32("tileRelX: ", player->pos.offset_.x);
        PRINT_F32("tileRelY: ", player->pos.offset_.y);
    }

    PRINT("\n");
    PRINT_I32("Camera pos X: ", gameState->cameraPos.chunkX);
    PRINT_I32("Camera pos Y: ", gameState->cameraPos.chunkY);
    PRINT_I32("Camera pos Z: ", gameState->cameraPos.chunkZ);
    PRINT("\n");

    PRINT_F32("Camera pos offset X: ", gameState->cameraPos.offset_.y);
    PRINT_F32("Camera pos offset Y: ", gameState->cameraPos.offset_.x);
#endif

    /// Background
#if 0
    DrawBitmap(screenBuff, &gameState->background, 0, 0);
#else
    DrawRectangle(screenBuff, Vec2{},
                  Vec2{ static_cast<f32>(screenBuff->width), static_cast<f32>(screenBuff->height) },
                  0.5f, 0.5f, 0.5f);
#endif

    /// Drawing and processing entities

    // Every entity has its own one of these
    EntityVisiblePieceGroup pieceGroup;
    pieceGroup.gameState = gameState;

    /// Simulation

    auto* entity{ simRegion->entities };
    for (i32 i{}; i < simRegion->entityCount; ++i, ++entity) {
        if (!entity->updatable) {
            continue;
        }

        pieceGroup.pieceCount = 0;

        // TODO: This is wrong, compute after update
        f32 shadowAlpha{ 1.0f - (0.5f * entity->pos.z) };
        if (shadowAlpha < 0) {
            shadowAlpha = 0.0f;
        }

        // Gets set for each entity
        MoveSpec moveSpec{ DefaultMoveSpec() };
        Vec3 ddP{};

        HeroBitmaps* heroBitmaps{ &gameState->heroBitmaps[entity->facingDir] };

        switch (entity->type) {
        case EntityType::WALL: {
            // Tree bitmaps
            PushBitmap(&pieceGroup, &gameState->tree, Vec2{}, 0, Vec2{ 40, 80 });
        } break;

        case EntityType::STAIRWELL: {
            PushRect(&pieceGroup, Vec2{}, 0, entity->walkableDim, Vec4{ 1, 1, 0, 1 }, 0.0f);
            PushRect(&pieceGroup, Vec2{}, entity->walkableHeight, entity->walkableDim,
                     Vec4{ 1, 0.5f, 0, 1 }, 0.0f);
        } break;

        case EntityType::HERO: {
            for (i32 controlIndex{}; controlIndex < ARRAY_COUNT(gameState->controlledHeroes);
                 ++controlIndex) {
                auto* controlled{ &gameState->controlledHeroes[controlIndex] };
                // Confirm we are the one controlling
                if (entity->storageIndex == controlled->entityIndex) {
                    // Reset, done in EndSim as we kind of have to for now
                    //if (controlled->requestReset) {
                    //    PRINT("Request reset!\n");
                    //    auto* lowEntity{ GetLowEntity(gameState, entity->storageIndex) };
                    //    ChangeEntityLocation(world, &gameState->worldArena, entity->storageIndex,
                    //                         lowEntity, lowEntity->startingPos);
                    //
                    //    continue;
                    //}

                    // Don't allow jumping if not on ground
                    // We don't touch anything here because MoveEntity handles all the flags
                    // when doing the simulation
                    // A plain read is sufficient here
                    if (controlled->dZ != 0.0f) {
                        if (IsSet(entity, SimEntityFlags::Z_SUPPORTED) ||
                            gameState->allowUnlimitedJumps) { // && entity->pos.z == 0.0f
                            entity->velocity.z = controlled->dZ;
                        }
                    }

                    moveSpec.speed = 30.0f;
                    if (controlled->sprint) {
                        moveSpec.speed *= 2.0f;
                    }

                    moveSpec.unitMaxAccelVector = true;
                    moveSpec.drag = 4.0f;
                    ddP = Vec3{ controlled->ddP.x, controlled->ddP.y, 0 };

                    /// Other actions ///

                    // Sword
                    if (controlled->dSword != Vec3::ZERO) {
                        auto* sword{ entity->sword.ptr };
                        if (sword && IsSet(sword, SimEntityFlags::NON_SPATIAL)) {
                            PRINT("Used sword!\n");
                            sword->distanceLimit = 6.0f;
                            const f32 swordVel{ 8.0f };
                            MakeEntitySpatial(sword, entity->pos, controlled->dSword * swordVel);
                            AddCollisionRule(gameState, sword->storageIndex, entity->storageIndex,
                                             false);
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
            PushBitmap(&pieceGroup, &gameState->shadow, Vec2{}, 0, heroBitmaps->align, shadowAlpha,
                       0.0f);
            PushBitmap(&pieceGroup, &heroBitmaps->torso, Vec2{}, 0, heroBitmaps->align);

            DrawHitpoints(entity, &pieceGroup);
        } break;

        case EntityType::FAMILIAR: {
            if (entity->followingHero) {
                SimEntity* closestHero{};
                constexpr f32 maxDist{ 10.0f };
                f32 closestHeroDSq{ SquareF32(maxDist) };

                // TODO: naive solution, BAD
                SimEntity* testEntity{ simRegion->entities };
                for (i32 testIndex{}; testIndex < simRegion->entityCount;
                     ++testIndex, ++testEntity) {
                    if (testEntity->type == EntityType::HERO) {
                        const f32 testDSq{ LengthSq(testEntity->pos - entity->pos) };
                        if (testDSq < closestHeroDSq) {
                            closestHero = testEntity;
                            closestHeroDSq = testDSq;
                            // Updated every frame we find the hero
                            closestHero->familiarIndex = closestHero->storageIndex;
                        }
                    }
                }

                const f32 stopDistSq{ SquareF32(2.25f) }; // Dist of 2.25f
                Vec3 acceleration{};

                if (closestHero && closestHeroDSq > stopDistSq) {
                    constexpr f32 speed{ 0.5f };
                    const f32 oneOverLength{ speed / Sqrt(closestHeroDSq) };
                    acceleration = (closestHero->pos - entity->pos) * oneOverLength;
                    //PRINT_F32("before: closestHeroDSq: ", closestHeroDSq);
                    //PRINT_F32("before: acceleration.x: ", acceleration.x);
                    //PRINT_F32("before: acceleration.y: ", acceleration.y);
                    if (closestHeroDSq > 17.0f) {
                        acceleration *= 1.75f;
                    }

                    //PRINT_F32("before: acceleration.x: ", acceleration.x);
                    //PRINT_F32("before: acceleration.y: ", acceleration.y);
                    //PRINT("\n");
                }

                moveSpec.speed = 50.0f;
                moveSpec.drag = 8.0f;

                ddP = acceleration;
            }

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

            // FIXME: this seems to not get called if we stand still and use the sword at the start
            // of the game, the sword stops working then
        case EntityType::SWORD: {
            // This doesn't affect the sword at all!
            moveSpec.speed = 0.0f;

            if (entity->distanceLimit == 0.0f) {
                MakeEntityNonSpatial(entity);
                ClearCollisionRulesFor(gameState, entity->storageIndex);
            }

            PushBitmap(&pieceGroup, &gameState->shadow, Vec2{}, 0, heroBitmaps->align, shadowAlpha,
                       0.0f);
            PushBitmap(&pieceGroup, &gameState->sword, Vec2{}, 0, Vec2{ 29, 10 });
        } break;

        case EntityType::SPACE: {
            for (i32 volumeIndex{}; volumeIndex < entity->collision->volumeCount; ++volumeIndex) {
                const auto* volume{ &entity->collision->volumes[volumeIndex] };
                PushRectOutline(&pieceGroup, volume->offsetPos.xy, 0, volume->dim.xy,
                                Vec4{ 0.0f, 0.25f, 1.0f, 1.0f }, 0.0f);
            }
        } break;

        default: {
            INVALID_CODE_PATH;
        } break;
        }

        //if (entity->velocity != Vec2::ZERO || ddP != Vec2::ZERO) {
        if (!IsSet(entity, SimEntityFlags::NON_SPATIAL) &&
            IsSet(entity, SimEntityFlags::MOVEABLE)) {
            MoveEntity(gameState, simRegion, entity, moveSpec, ddP, delta);
        }

        //if (entity->type == EntityType::HERO) {
        //    PRINT_F32("Z", entity->z);
        //}

        // Draw pieces
        for (i32 pieceIndex{}; pieceIndex < pieceGroup.pieceCount; ++pieceIndex) {
            const EntityVisiblePiece* piece{ &pieceGroup.pieces[pieceIndex] };

            const Vec3 entityBasePos{ GetEntityGroundPoint(entity) };
            const f32 zFudge{ 1.0f + 0.1f * (entityBasePos.z + piece->offsetZ) };

            const Vec2 screenCenter{ screenBuff->width * 0.5f, screenBuff->height * 0.5f };

            //const Vec2 entityGroundPoint{ screenCenter.x + (gameState->metersToPixels *
            //entity->pos.x),
            //                              screenCenter.y -
            //                                  (gameState->metersToPixels * entity->pos.y) };
            const Vec2 entityGroundPoint{
                screenCenter.x + (gameState->metersToPixels * entityBasePos.x * zFudge),
                screenCenter.y - (gameState->metersToPixels * entityBasePos.y * zFudge)
            };

            const f32 entityZ{ -entityBasePos.z * gameState->metersToPixels };

            const Vec2 center{ entityGroundPoint.x + piece->offset.x,
                               entityGroundPoint.y + piece->offset.y +
                                   //(gameState->metersToPixels * piece->offsetZ) +
                                   (entityZ * piece->entityZC) };

            if (piece->bitmap) {
                DrawBitmap(screenBuff, piece->bitmap, center.x, center.y, piece->a);
            } else {
                const Vec2 halfDim{ 0.5f * piece->dimension * gameState->metersToPixels };
                DrawRectangle(screenBuff, center - halfDim, center + halfDim, piece->r, piece->g,
                              piece->b);
            }

            // @Debug collision box
            if (gameState->showCollisionBoxes) {
                // Don't draw for room space as it blocks the whole screen
                if (entity->type != EntityType::SPACE) {
                    const Vec2 leftTop{
                        entityGroundPoint.x - (0.5f * gameState->metersToPixels *
                                               entity->collision->totalVolume.dim.x),
                        entityGroundPoint.y - (0.5f * gameState->metersToPixels *
                                               entity->collision->totalVolume.dim.y)
                    };

                    DrawRectangle(screenBuff, leftTop,
                                  leftTop + entity->collision->totalVolume.dim.xy *
                                                gameState->metersToPixels // *0.95f
                                  ,
                                  0.5f, 0.1f, 0.5f);
                }
            }
        }
    }

    // @Debug
#if 0
    PRINT_F32("Max velocity: ", Sqrt(simRegion->maxRecordedEntityVelocitySq));
    PRINT_I32("Max index: ", simRegion->maxRecordedEntityVelocityIndex);

    const char* typeStr{ EntityTypeToStr(simRegion->maxRecordedEntityVelocityType) };
    PRINT("Max type: ");
    PRINT(typeStr);
    PRINT("\n");
#endif

    WorldPosition worldOrigin{};
    const Vec3 diff{ SubtractWorldPos(simRegion->world, &worldOrigin, &simRegion->origin) };
    DrawRectangle(screenBuff, diff.xy, Vec2{ 10.0f, 10.0f }, 1.0f, 1.0f, 1.0f);

    EndSim(simRegion, gameState);
}

extern "C" GET_SOUND_SAMPLES(GetSoundSamples) {
    UNUSED_PARAMS(threadContext);

    const GameState* gameState{ static_cast<GameState*>(memory->permanentStorage) };
    OutputSound(gameState, soundBuff);
}
