#include "handmade_tile.h"

#include "handmade_memory.h"
#include "math/handmade_vec3.h"

INTERNAL void
InitializeTilemap(Tilemap* tilemap, f32 tileSideInMeters) {
    // chunk size is chunkSize x chunkSize (really: chunkShifft * chunkShift)
    tilemap->chunkShift = 4;
    tilemap->chunkMask = (1 << tilemap->chunkShift) - 1;
    tilemap->chunkSize = 1 << tilemap->chunkShift;

    // NOTE: This is now seperated from the rendering (tileSideInPixels)
    tilemap->tileSideInMeters = tileSideInMeters;

    for (i32 tileChunkIndex{}; tileChunkIndex < tilemap->tileChunkHash.size; ++tileChunkIndex) {
        tilemap->tileChunkHash[tileChunkIndex].tileChunkX = tile_Chunk_Uninitialized;
    }
}

NODISCARD
INTERNAL Tilechunk*
GetTilechunk(Tilemap* tilemap, i32 tileChunkX, i32 tileChunkY, i32 tileChunkZ,
             MemoryArena* arena = nullptr) {
    ASSERT(tileChunkX > -tile_Chunk_Safe_Margin);
    ASSERT(tileChunkY > -tile_Chunk_Safe_Margin);
    ASSERT(tileChunkZ > -tile_Chunk_Safe_Margin);
    ASSERT(tileChunkX < tile_Chunk_Safe_Margin);
    ASSERT(tileChunkY < tile_Chunk_Safe_Margin);
    ASSERT(tileChunkZ < tile_Chunk_Safe_Margin);

    // TODO: better hash function ;D
    const i32 hashValue{ 19 * tileChunkX + 7 * tileChunkY + 3 * tileChunkZ };
    const i32 hashSlot{ hashValue & (tilemap->tileChunkHash.size - 1) };
    ASSERT(hashSlot < (tilemap->tileChunkHash.size - 1));
    Tilechunk* chunk{ &tilemap->tileChunkHash[hashSlot] };

    do {
        if (tileChunkX == chunk->tileChunkX && tileChunkY == chunk->tileChunkY &&
            tileChunkZ == chunk->tileChunkZ) {
            break;
        }

        // Already initialized -> add next
        if (arena && chunk->tileChunkX != tile_Chunk_Uninitialized && !chunk->nextInHash) {
            chunk->nextInHash = PushSize(arena, Tilechunk);
            chunk = chunk->nextInHash;
            chunk->tileChunkX = tile_Chunk_Uninitialized;
        }

        // Initialize first
        if (arena && chunk->tileChunkX == tile_Chunk_Uninitialized) {
            const u32 tileCount{ tilemap->chunkSize * tilemap->chunkSize };

            chunk->tileChunkX = tileChunkX;
            chunk->tileChunkY = tileChunkY;
            chunk->tileChunkZ = tileChunkZ;

            chunk->tiles = PushArray(arena, tileCount, u32);
            for (u32 tileIndex{}; tileIndex < tileCount; ++tileIndex) {
                chunk->tiles[tileIndex] = 3;
            }

            chunk->nextInHash = nullptr;

            break;
        }

        chunk = chunk->nextInHash;
    } while (chunk);

    return chunk;
}

NODISCARD
INTERNAL TilechunkPosition_
GetChunkPosition(const Tilemap* tilemap, i32 absTileX, i32 absTileY, i32 absTileZ) {
    TilechunkPosition_ result{};

    // Shift down by chunkShift to get the upper bits for chunk index
    result.chunkX = absTileX >> tilemap->chunkShift;
    result.chunkY = absTileY >> tilemap->chunkShift;
    result.chunkZ = absTileZ;

    // Get the lower 8 bits for tile relative positions
    result.chunkRelativeTileX = absTileX & tilemap->chunkMask;
    result.chunkRelativeTileY = absTileY & tilemap->chunkMask;

    return result;
}

NODISCARD
INTERNAL u32
GetTileValueChecked(const Tilemap* tilemap, const Tilechunk* tileChunk, i32 relX, i32 relY) {
    ASSERT(tileChunk);
    ASSERT(relX < tilemap->chunkSize && relY < tilemap->chunkSize);
    const u32 tileValue{ tileChunk->tiles[(tilemap->chunkSize * relY) + relX] };
    return tileValue;
}

NODISCARD
INTERNAL u32
GetTileValue(Tilemap* tilemap, i32 absTileX, i32 absTileY, i32 absTileZ) {
    const TilechunkPosition_ chunkPos{ GetChunkPosition(tilemap, absTileX, absTileY, absTileZ) };
    Tilechunk* tileChunk{ GetTilechunk(tilemap, chunkPos.chunkX, chunkPos.chunkY,
                                       chunkPos.chunkZ) };

    u32 tileChunkValue{};
    if (tileChunk && tileChunk->tiles) {
        tileChunkValue = GetTileValueChecked(tilemap, tileChunk, chunkPos.chunkRelativeTileX,
                                             chunkPos.chunkRelativeTileY);
    } else {
        // invalid tilemapX or tilemapY i.e. out of bounds
        //tileChunkValue = out_Of_Bounds_Tile_Value;
        //DEBUG_PLATFORM_PRINT("Invalid tileChunkX or tileChunkY");
    }

    return tileChunkValue;
}

INTERNAL void
SetTileValueChecked(const Tilemap* tilemap, const Tilechunk* tileChunk, i32 tileX, i32 tileY,
                    u32 value) {
    ASSERT(tileChunk);
    ASSERT(tileX < tilemap->chunkSize && tileY < tilemap->chunkSize);
    tileChunk->tiles[(tilemap->chunkSize * tileY) + tileX] = value;
}

INTERNAL void
SetTileValue(MemoryArena* worldArena, Tilemap* tilemap, i32 absTileX, i32 absTileY, i32 absTileZ,
             u32 value) {
    const TilechunkPosition_ chunkPos{ GetChunkPosition(tilemap, absTileX, absTileY, absTileZ) };
    Tilechunk* tileChunk{ GetTilechunk(tilemap, chunkPos.chunkX, chunkPos.chunkY, chunkPos.chunkZ,
                                       worldArena) };
    ASSERT(tileChunk);

    // Create tileChunk tiles if they don't exist
    if (!tileChunk->tiles) {
        const u32 tileCount{ tilemap->chunkSize * tilemap->chunkSize };
        tileChunk->tiles = PushArray(worldArena, tileCount, u32);

        for (u32 tileIndex{}; tileIndex < tileCount; ++tileIndex) {
            tileChunk->tiles[tileIndex] = 3;
        }
    }

    if (tileChunk) {
        SetTileValueChecked(tilemap, tileChunk, chunkPos.chunkRelativeTileX,
                            chunkPos.chunkRelativeTileY, value);
    }
}

NODISCARD
INTERNAL bool32
IsTileValueEmpty(u32 value) {
    const bool32 result{ value != 0 && value != blocked_Tile_Value };
    return result;
}

NODISCARD
INTERNAL bool32
IsTilemapPointEmpty(Tilemap* tilemap, TilemapPosition pos) {
    const u32 tileValue{ GetTileValue(tilemap, pos.absTileX, pos.absTileY, pos.absTileZ) };
    const bool32 empty{ IsTileValueEmpty(tileValue) };
    return empty;
}

INTERNAL void
ReCanonicalizeCoordinate(const Tilemap* tilemap, i32* tileIndex, f32* relPos) {
    const i32 offset{ RoundF32ToI32(*relPos / tilemap->tileSideInMeters) };
    // NOTE: tilemap is assumed to be toroidal, if you step over the end you start at the
    // beginning
    *tileIndex += offset;

    *relPos -= static_cast<f32>(offset) * tilemap->tileSideInMeters;
    // TODO: what to do if: *relPos == tilemap->tilesideinpixels
    // This can happen because we do the divide and floor and then multiple, the player might
    // end up being on the same tile Relative positions must be within the tile size in pixels
    // TODO: fix the floating point math to not allow the case above of ==
    ASSERT(*relPos >= -tilemap->tileSideInMeters * 0.5f &&
           *relPos <= tilemap->tileSideInMeters * 0.5f);
}

NODISCARD
INTERNAL TilemapPosition
MapIntoTileSpace(const Tilemap* tilemap, TilemapPosition pos, Vec2 offset) {
    TilemapPosition result{ pos };
    result.tileOffset_ += offset;

    ReCanonicalizeCoordinate(tilemap, &result.absTileX, &result.tileOffset_.x);
    ReCanonicalizeCoordinate(tilemap, &result.absTileY, &result.tileOffset_.y);

    return result;
}

NODISCARD
INTERNAL bool32
AreOnSameTiles(const TilemapPosition* pos, const TilemapPosition* newPos) {
    const bool32 result{ pos->absTileX == newPos->absTileX && pos->absTileY == newPos->absTileY &&
                         pos->absTileZ == newPos->absTileZ };
    return result;
}

NODISCARD
INTERNAL i32
TilemapPositionModifyZChecked(const Tilemap* tilemap, const TilemapPosition* pos, i32 offset) {
    // Assert some limit to avoid UB cases in the extremes
    // Probably will never hit this as we most likely always move only 1 up or down
    ASSERT(-10 <= offset && offset <= 10);

    i32 newZ{ pos->absTileZ };
    if (offset >= 0) {
        // NOTE: removed check when moved to hash-based world storage
        //if (pos->absTileZ < (tilemap->tileChunkCountZ - offset)) {
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
INTERNAL TilemapDiff
SubtractTilemapPos(const Tilemap* tilemap, const TilemapPosition* a, const TilemapPosition* b) {
    TilemapDiff diff{};

    const Vec3 dTile{ static_cast<f32>(a->absTileX) - static_cast<f32>(b->absTileX),
                      static_cast<f32>(a->absTileY) - static_cast<f32>(b->absTileY),
                      static_cast<f32>(a->absTileZ) - static_cast<f32>(b->absTileZ) };

    diff.x = (tilemap->tileSideInMeters * dTile.x) + a->tileOffset_.x - b->tileOffset_.x;
    diff.y = (tilemap->tileSideInMeters * dTile.y) + a->tileOffset_.y - b->tileOffset_.y;
    diff.z = tilemap->tileSideInMeters * dTile.z;

    return diff;
}
