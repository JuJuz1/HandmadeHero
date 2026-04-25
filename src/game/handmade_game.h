#ifndef HANDMADE_GAME_H
#define HANDMADE_GAME_H

#include "game/handmade.h"

#include "game/handmade_array.h"
#include "game/handmade_entity.h"
#include "game/handmade_memory.h"
#include "game/handmade_world.h"
#include "game/math/handmade_vec2.h"

/*
    Game specific code
*/

GLOBAL constexpr i32 tiles_Per_Width{ 17 };
GLOBAL constexpr i32 tiles_Per_Height{ 9 };

struct LoadedBitmapInfo {
    u32* pixels;
    i32 width;
    i32 height;
};

struct HeroBitmaps {
    LoadedBitmapInfo head;
    LoadedBitmapInfo cape;
    LoadedBitmapInfo torso;
    Vec2 align;
};

struct EntityVisiblePiece {
    LoadedBitmapInfo* bitmap;
    Vec2 offset;
    f32 offsetZ;
    f32 entityZC;

    f32 r, g, b, a;
    Vec2 dimension;
};

/**
 * The game state!
 */
struct GameState {
    MemoryArena worldArena;
    World* world;
    f32 metersToPixels; // TODO: should this be here?

    WorldPosition cameraPos;
    i32 cameraFollowingEntityIndex; // By default the first player (index 1)

    Array<LowEntity, 4096> lowEntities; // Holds all entities
    i32 lowEntityCount;
    i32 highEntityCount;

    Array<i32, ARRAY_COUNT(Input::playerInputs)> playerIndexFromController;

    Array<HeroBitmaps, 4> heroBitmaps;
    LoadedBitmapInfo background;
    LoadedBitmapInfo tree;
    LoadedBitmapInfo shadow;

    LoadedBitmapInfo sword;

    bool32 startWithAPlayer;
};

// TODO: this should just be a part of the renderer...
struct EntityVisiblePieceGroup {
    GameState* gameState;
    Array<EntityVisiblePiece, 8> pieces;
    i32 pieceCount;
};

/// Here we can put functions which many other files need to call ///

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

#endif // HANDMADE_GAME_H
