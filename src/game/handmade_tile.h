#ifndef HANDMADE_TILE_H
#define HANDMADE_TILE_H

#include "handmade.h"

#include "math/handmade_vec2.h"

/*
    A tilemap contains chunks. A chunk contains tiles.

    Tiles:
    - 0: out of bounds value
    - 1: blocked tile value
    - else: not "blocked"
*/

GLOBAL constexpr u32 blocked_Tile_Value{ 1 };

// This is the safe margin from the borders of the tilemap (~4 billion)
GLOBAL constexpr u32 tile_Chunk_Safe_Margin{ 256 };

// NOTE: Engine internal
struct TilechunkPosition_ {
    // We use the upper 24 bits for the chunk index and the lower 8 for the x and y position
    // relative to the chunk's tile
    u32 chunkX;
    u32 chunkY;
    u32 chunkZ;

    u32 chunkRelativeTileX;
    u32 chunkRelativeTileY;
};

struct TilemapDiff {
    f32 x;
    f32 y;
    f32 z;
};

struct TilemapPosition {
    // New way of storing the information, we don't need tilemapX and Y
    // This is the "real" tileX and tileY in the whole tilemap
    u32 absTileX;
    u32 absTileY;
    u32 absTileZ;

    // Tile-relative x and y from the center of the tile
    Vec2 tileOffset_;
};

struct Tilechunk {
    u32* tiles;

    u32 tileChunkX;
    u32 tileChunkY;
    u32 tileChunkZ;

    Tilechunk* nextInHash;
};

struct Tilemap {
    // TODO: Size must be power of two for now
    Array<Tilechunk, 4096> tileChunkHash;
    //Tilechunk tileChunkHash[4096];

    u32 chunkShift;
    u32 chunkMask;
    u32 chunkSize;

    f32 tileSideInMeters;
};

INTERNAL void InitializeTilemap(Tilemap* tilemap, f32 tileSideInMeters);

NODISCARD
INTERNAL Tilechunk* GetTilechunk(Tilemap* tilemap, u32 tileChunkX, u32 tileChunkY, u32 tileChunkZ,
                                 MemoryArena* arena);

NODISCARD
INTERNAL TilechunkPosition_ GetChunkPosition(const Tilemap* tilemap, u32 absTileX, u32 absTileY,
                                             u32 absTileZ);

NODISCARD
INTERNAL u32 GetTileValueChecked(const Tilemap* tilemap, const Tilechunk* tileChunk, u32 relX,
                                 u32 relY);

NODISCARD
INTERNAL u32 GetTileValue(Tilemap* tilemap, u32 absTileX, u32 absTileY, u32 absTileZ);

INTERNAL void SetTileValueChecked(const Tilemap* tilemap, const Tilechunk* tileChunk, u32 tileX,
                                  u32 tileY, u32 value);
INTERNAL void SetTileValue(MemoryArena* worldArena, Tilemap* tilemap, u32 absTileX, u32 absTileY,
                           u32 absTileZ, u32 value);

NODISCARD
INTERNAL bool32 IsTileValueEmpty(u32 value);

NODISCARD
INTERNAL bool32 IsTilemapPointEmpty(const Tilemap* tilemap, TilemapPosition pos);

INTERNAL void ReCanonicalizeCoordinate(const Tilemap* tilemap, u32* tileIndex, f32* relPos);

NODISCARD
INTERNAL TilemapPosition MapIntoTileSpace(const Tilemap* tilemap, TilemapPosition pos, Vec2 offset);

NODISCARD
INTERNAL bool32 AreOnSameTiles(const TilemapPosition* pos, const TilemapPosition* newPos);

NODISCARD
INTERNAL u32 TilemapPositionModifyZChecked(const Tilemap* tilemap, const TilemapPosition* pos,
                                           i32 offset);

NODISCARD
INTERNAL TilemapDiff SubtractTilemapPos(const Tilemap* tilemap, const TilemapPosition* a,
                                        const TilemapPosition* b);

#endif // HANDMADE_TILE_H
