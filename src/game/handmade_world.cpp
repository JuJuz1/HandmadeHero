#include "handmade_world.h"

#include "game/handmade_intrinsics.h"
#include "game/handmade_memory.h"
#include "game/math/handmade_vec3.h"

INTERNAL void
InitializeWorld(World* world, f32 tileSideInMeters) {
    // chunk size is chunkSize x chunkSize (really: chunkShifft * chunkShift)
    world->chunkShift = 4;
    world->chunkMask = (1 << world->chunkShift) - 1;
    world->chunkSize = 1 << world->chunkShift;

    // NOTE: This is now seperated from the rendering (tileSideInPixels)
    world->tileSideInMeters = tileSideInMeters;

    for (i32 tileChunkIndex{}; tileChunkIndex < world->worldChunkHash.size; ++tileChunkIndex) {
        world->worldChunkHash[tileChunkIndex].chunkX = tile_Chunk_Uninitialized;
    }
}

NODISCARD
INTERNAL WorldChunk*
GetWorldChunk(World* world, i32 chunkX, i32 chunkY, i32 chunkZ, MemoryArena* arena = nullptr) {
    ASSERT(chunkX > -tile_Chunk_Safe_Margin);
    ASSERT(chunkY > -tile_Chunk_Safe_Margin);
    ASSERT(chunkZ > -tile_Chunk_Safe_Margin);
    ASSERT(chunkX < tile_Chunk_Safe_Margin);
    ASSERT(chunkY < tile_Chunk_Safe_Margin);
    ASSERT(chunkZ < tile_Chunk_Safe_Margin);

    // TODO: better hash function ;D
    const i32 hashValue{ 19 * chunkX + 7 * chunkY + 3 * chunkZ };
    const i32 hashSlot{ static_cast<i32>(hashValue & (world->worldChunkHash.size - 1)) };
    ASSERT(hashSlot < (world->worldChunkHash.size - 1));
    WorldChunk* chunk{ &world->worldChunkHash[hashSlot] };

    do {
        if (chunkX == chunk->chunkX && chunkY == chunk->chunkY && chunkZ == chunk->chunkZ) {
            break;
        }

        // Already initialized -> add next
        if (arena && chunk->chunkX != tile_Chunk_Uninitialized && !chunk->nextInHash) {
            chunk->nextInHash = PushSize(arena, WorldChunk);
            chunk = chunk->nextInHash;
            chunk->chunkX = tile_Chunk_Uninitialized;
        }

        // Initialize first
        if (arena && chunk->chunkX == tile_Chunk_Uninitialized) {
            const i32 tileCount{ world->chunkSize * world->chunkSize };

            chunk->chunkX = chunkX;
            chunk->chunkY = chunkY;
            chunk->chunkZ = chunkZ;

            chunk->nextInHash = nullptr;

            break;
        }

        chunk = chunk->nextInHash;
    } while (chunk);

    return chunk;
}

NODISCARD
INTERNAL WorldChunkPosition_
GetChunkPosition(const World* world, i32 absTileX, i32 absTileY, i32 absTileZ) {
    WorldChunkPosition_ result{};

    // Shift down by chunkShift to get the upper bits for chunk index
    result.chunkX = absTileX >> world->chunkShift;
    result.chunkY = absTileY >> world->chunkShift;
    result.chunkZ = absTileZ;

    // Get the lower 8 bits for tile relative positions
    result.chunkRelativeTileX = absTileX & world->chunkMask;
    result.chunkRelativeTileY = absTileY & world->chunkMask;

    return result;
}

NODISCARD
INTERNAL bool32
IsTileValueEmpty(u32 value) {
    const bool32 result{ value != 0 && value != blocked_Tile_Value };
    return result;
}

INTERNAL void
ReCanonicalizeCoordinate(const World* world, i32* tileIndex, f32* relPos) {
    const i32 offset{ RoundF32ToI32(*relPos / world->tileSideInMeters) };
    // NOTE: world is assumed to be toroidal, if you step over the end you start at the
    // beginning
    *tileIndex += offset;

    *relPos -= static_cast<f32>(offset) * world->tileSideInMeters;
    // TODO: what to do if: *relPos == world->tilesideinpixels
    // This can happen because we do the divide and floor and then multiple, the player might
    // end up being on the same tile Relative positions must be within the tile size in pixels
    // TODO: fix the floating point math to not allow the case above of ==
    ASSERT(*relPos >= -world->tileSideInMeters * 0.5f && *relPos <= world->tileSideInMeters * 0.5f);
}

NODISCARD
INTERNAL WorldPosition
MapIntoTileSpace(const World* world, WorldPosition pos, Vec2 offset) {
    WorldPosition result{ pos };
    result.tileOffset_ += offset;

    ReCanonicalizeCoordinate(world, &result.absTileX, &result.tileOffset_.x);
    ReCanonicalizeCoordinate(world, &result.absTileY, &result.tileOffset_.y);

    return result;
}

NODISCARD
INTERNAL bool32
AreOnSameTiles(const WorldPosition* pos, const WorldPosition* newPos) {
    const bool32 result{ pos->absTileX == newPos->absTileX && pos->absTileY == newPos->absTileY &&
                         pos->absTileZ == newPos->absTileZ };
    return result;
}

NODISCARD
INTERNAL i32
WorldPositionModifyZChecked(const World* world, const WorldPosition* pos, i32 offset) {
    // Assert some limit to avoid UB cases in the extremes
    // Probably will never hit this as we most likely always move only 1 up or down
    ASSERT(-10 <= offset && offset <= 10);

    i32 newZ{ pos->absTileZ };
    if (offset >= 0) {
        // NOTE: removed check when moved to hash-based world storage
        //if (pos->absTileZ < (world->tileChunkCountZ - offset)) {
        newZ += offset;
        //}
    } else {
        // NOTE: use signed values for tiles so we probably don't need the check
        // Handle negative with a trick
        //if (static_cast<u32>((-offset)) <= pos->absTileZ) {
        newZ += offset;
        //}
    }

    return newZ;
}

NODISCARD
INTERNAL WorldDiff
SubtractWorldPos(const World* world, const WorldPosition* a, const WorldPosition* b) {
    WorldDiff diff{};

    const Vec3 dTile{ static_cast<f32>(a->absTileX) - static_cast<f32>(b->absTileX),
                      static_cast<f32>(a->absTileY) - static_cast<f32>(b->absTileY),
                      static_cast<f32>(a->absTileZ) - static_cast<f32>(b->absTileZ) };

    diff.x = (world->tileSideInMeters * dTile.x) + a->tileOffset_.x - b->tileOffset_.x;
    diff.y = (world->tileSideInMeters * dTile.y) + a->tileOffset_.y - b->tileOffset_.y;
    diff.z = world->tileSideInMeters * dTile.z;

    return diff;
}
