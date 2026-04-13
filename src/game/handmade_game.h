#ifndef HANDMADE_GAME_H
#define HANDMADE_GAME_H

#include "handmade.h"

#include "handmade_array.h"
#include "handmade_memory.h"
#include "handmade_tile.h"
#include "math/handmade_vec2.h"

/*
    Game specific code
*/

GLOBAL constexpr i32 tiles_Per_Width{ 17 };
GLOBAL constexpr i32 tiles_Per_Height{ 9 };

struct World {
    Tilemap* tilemap;
};

struct LoadedBitmapInfo {
    u32* pixels;
    i32 width;
    i32 height;
};

struct HeroBitmaps {
    LoadedBitmapInfo head;
    LoadedBitmapInfo torso;
    Vec2 align;
};

/// Entities ///

enum class EntityType {
    NON_EXISTENT = 0,
    HERO,
    WALL,
};

/**
 * Low frequency entity meant to be "ticked" at a slower rate compared to high frequency
 */
struct LowEntity {
    EntityType type;

    TilemapPosition pos;
    f32 width, height;

    bool32 collides;
    i32 dAbsTileZ; // Stairs

    i32 highEntityIndex;
};

/**
 * High frequency
 */
struct HighEntity {
    Vec2 pos; // NOTE: This is now already relative to the camera center
    u32 absTileZ;
    Vec2 velocity;

    i32 facingDir;

    i32 lowEntityIndex;
};

struct Entity {
    LowEntity* low;
    HighEntity* high;
    i32 lowIndex;
};

/**
 * The game state!
 */
struct GameState {
    MemoryArena worldArena;
    World* world;

    TilemapPosition cameraPos;
    i32 cameraFollowingEntityIndex; // By default the first player (index 1)

    Array<LowEntity, 4096> lowEntities;   // Holds all entities
    Array<HighEntity, 256> highEntities_; // Holds the entities marked as high frequency
    i32 lowEntityCount;
    i32 highEntityCount;

    Array<i32, ARRAY_COUNT(Input::playerInputs)> playerIndexFromController;

    LoadedBitmapInfo background;
    Array<HeroBitmaps, 4> heroBitmaps;

    bool32 startWithAPlayer;
};

#endif // HANDMADE_GAME_H
