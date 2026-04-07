#ifndef HANDMADE_TILE_H
#define HANDMADE_TILE_H

/*
    A tilemap contains chunks. A chunk contains tiles.

    Tiles:
    - 0: out of bounds value
    - 1: blocked tile value
    - else: not "blocked"
*/

GLOBAL constexpr u32 blocked_Tile_Value{ 1 };

// NOTE: Engine internal
struct TilechunkPosition {
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
};

struct Tilemap {
    Tilechunk* tileChunks;
    u32 tileChunkCountX;
    u32 tileChunkCountY;
    u32 tileChunkCountZ;

    u32 chunkShift;
    u32 chunkMask;
    u32 chunkSize;

    f32 tileSideInMeters;
};

#endif // HANDMADE_TILE_H
