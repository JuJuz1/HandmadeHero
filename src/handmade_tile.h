#ifndef HANDMADE_TILE_H
#define HANDMADE_TILE_H

/*
    A tilemap contains chunks. A chunk contains tiles.
*/

GLOBAL constexpr u32 blocked_Tile_Value{ 1 };

// NOTE: Engine internal
struct TileChunkPosition {
    // We use the upper 24 bits for the chunk index and the lower 8 for the x and y position
    // relative to the chunk's tile
    u32 chunkX;
    u32 chunkY;

    u32 chunkRelativeX;
    u32 chunkRelativeY;
};

struct TileMapPosition {
    // New way of storing the information, we don't need tilemapX and Y
    // This is the "real" tileX and tileY in the whole tilemap
    u32 absTileX;
    u32 absTileY;

    // Tile-relative x and y, should these be from the center of the tile?
    f32 tileRelativePosX;
    f32 tileRelativePosY;
};

struct TileChunk {
    u32* tiles;
};

struct TileMap {
    TileChunk* tileChunks;
    u32 tileChunkCountX;
    u32 tileChunkCountY;

    u32 chunkShift;
    u32 chunkMask;
    u32 chunkDim;

    f32 tileSideInMeters;
    i32 tileSideInPixels;
    f32 metersToPixels;
};

#endif // HANDMADE_TILE_H
