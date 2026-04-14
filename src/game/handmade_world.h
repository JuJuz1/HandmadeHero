#ifndef HANDMADE_WORLD_H
#define HANDMADE_WORLD_H

#include "game/handmade.h"

#include "game/handmade_array.h"
#include "math/handmade_vec2.h"

/*
    A world contains chunks. A chunk contains tiles.

    Tiles:
    - 0: out of bounds value
    - 1: blocked tile value
    - else: not "blocked"
*/

GLOBAL constexpr u32 blocked_Tile_Value{ 1 };

// This is the safe margin from the borders of the world (~4 billion)
GLOBAL constexpr i32 tile_Chunk_Safe_Margin{ INT32_MAX / 64 };
GLOBAL constexpr i32 tile_Chunk_Uninitialized{ INT32_MAX };

GLOBAL constexpr i32 tiles_Per_Chunk{ 16 };

// NOTE: Engine internal
struct WorldChunkPosition_ {
    // We use the upper 24 bits for the chunk index and the lower 8 for the x and y position
    // relative to the chunk's tile
    i32 chunkX;
    i32 chunkY;
    i32 chunkZ;

    i32 chunkRelativeTileX;
    i32 chunkRelativeTileY;
};

struct WorldDiff {
    f32 x;
    f32 y;
    f32 z;
};

struct WorldPosition {
    i32 chunkX;
    i32 chunkY;
    i32 chunkZ;

    // Tile-relative x and y from the center of the chunk
    Vec2 offset_;
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

    WorldPosition pos;
    f32 width, height;

    bool32 collides;
    i32 dChunkZ; // Stairs

    i32 highEntityIndex;
};

/**
 * High frequency
 */
struct HighEntity {
    Vec2 pos; // NOTE: This is now already relative to the camera center
    u32 chunkZ;
    Vec2 velocity;

    i32 facingDir;

    i32 lowEntityIndex;
};

struct Entity {
    LowEntity* low;
    HighEntity* high;
    i32 lowIndex;
};

struct WorldEntityBlock {
    Array<i32, 16> lowEntityIndexes;
    WorldEntityBlock* next;
    i32 entityCount;
};

struct WorldChunk {
    i32 chunkY;
    i32 chunkX;
    i32 chunkZ;

    // Episode 56 tiles to entities
    WorldEntityBlock firstBlock;

    WorldChunk* nextInHash;
};

struct World {
    // TODO: Size must be power of two for now
    Array<WorldChunk, 4096> worldChunkHash;
    WorldEntityBlock* firstFree;

    f32 tileSideInMeters;
    f32 chunkSideInMeters;
};

INTERNAL void InitializeWorld(World* world, f32 tileSideInMeters);

NODISCARD
INTERNAL WorldChunk* GetWorldChunk(World* world, i32 chunkX, i32 chunkY, i32 chunkZ,
                                   MemoryArena* arena);

NODISCARD
INTERNAL bool32 IsTileValueEmpty(u32 value);

NODISCARD
INTERNAL bool32 IsCanonical(World* world, f32 tileRel);

NODISCARD
INTERNAL bool32 IsCanonical(World* world, Vec2 offset);

INTERNAL void ReCanonicalizeCoordinate(const World* world, i32* tileIndex, f32* relPos);

NODISCARD
INTERNAL WorldPosition MapIntoChunkSpace(const World* world, WorldPosition pos, Vec2 offset);

NODISCARD
INTERNAL WorldPosition ChunkPositionFromTilePosition(World* world, i32 tileX, i32 tileY, i32 tileZ);

NODISCARD
INTERNAL bool32 AreOnSameChunk(const World* world, const WorldPosition* A, const WorldPosition* B);

NODISCARD
INTERNAL i32 WorldPositionModifyZChecked(const World* world, const WorldPosition* pos, i32 offset);

NODISCARD
INTERNAL WorldDiff SubtractWorldPos(const World* world, const WorldPosition* a,
                                    const WorldPosition* b);

INTERNAL WorldEntityBlock* FreeBlock(WorldEntityBlock* block);

INTERNAL void ChangeEntityLocation(World* world, MemoryArena* arena, i32 lowIndex,
                                   WorldPosition* oldPos, WorldPosition* newPos);

#endif // HANDMADE_WORLD_H
