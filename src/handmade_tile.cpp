NODISCARD
INTERNAL TileChunk*
GetTileChunk(const TileMap* tileMap, u32 tileChunkX, u32 tileChunkY, u32 tileChunkZ) {
    TileChunk* tileChunk{};
    if (tileChunkX < tileMap->tileChunkCountX && tileChunkY < tileMap->tileChunkCountY &&
        tileChunkZ < tileMap->tileChunkCountZ) {
        tileChunk =
            &tileMap
                 ->tileChunks[(tileMap->tileChunkCountX * tileMap->tileChunkCountY * tileChunkZ) +
                              (tileMap->tileChunkCountX * tileChunkY) + tileChunkX];
    }

    return tileChunk;
}

NODISCARD
INTERNAL TileChunkPosition
GetChunkPosition(const TileMap* tileMap, u32 absTileX, u32 absTileY, u32 absTileZ) {
    TileChunkPosition result{};

    // Shift down by chunkShift to get the upper bits for chunk index
    result.chunkX = absTileX >> tileMap->chunkShift;
    result.chunkY = absTileY >> tileMap->chunkShift;
    result.chunkZ = absTileZ;

    // Get the lower 8 bits for tile relative positions
    result.chunkRelativeTileX = absTileX & tileMap->chunkMask;
    result.chunkRelativeTileY = absTileY & tileMap->chunkMask;

    return result;
}

NODISCARD
INTERNAL u32
GetTileValueChecked(const TileMap* tileMap, const TileChunk* tileChunk, u32 relX, u32 relY) {
    ASSERT(tileChunk);
    ASSERT(relX < tileMap->chunkSize && relY < tileMap->chunkSize);
    const u32 tileValue{ tileChunk->tiles[(tileMap->chunkSize * relY) + relX] };
    return tileValue;
}

NODISCARD
INTERNAL u32
GetTileValue(const TileMap* tileMap, u32 absTileX, u32 absTileY, u32 absTileZ) {
    const TileChunkPosition chunkPos{ GetChunkPosition(tileMap, absTileX, absTileY, absTileZ) };
    const TileChunk* tileChunk{ GetTileChunk(tileMap, chunkPos.chunkX, chunkPos.chunkY,
                                             chunkPos.chunkZ) };

    u32 tileChunkValue{};
    if (tileChunk && tileChunk->tiles) {
        tileChunkValue = GetTileValueChecked(tileMap, tileChunk, chunkPos.chunkRelativeTileX,
                                             chunkPos.chunkRelativeTileY);
    } else {
        // invalid tileMapX or tileMapY i.e. out of bounds
        //tileChunkValue = out_Of_Bounds_Tile_Value;
        //DEBUG_PLATFORM_PRINT("Invalid tileChunkX or tileChunkY");
    }

    return tileChunkValue;
}

INTERNAL void
SetTileValueChecked(const TileMap* tileMap, const TileChunk* tileChunk, u32 tileX, u32 tileY,
                    u32 value) {
    ASSERT(tileChunk);
    ASSERT(tileX < tileMap->chunkSize && tileY < tileMap->chunkSize);
    tileChunk->tiles[(tileMap->chunkSize * tileY) + tileX] = value;
}

INTERNAL void
SetTileValue(MemoryArena* worldArena, TileMap* tileMap, u32 absTileX, u32 absTileY, u32 absTileZ,
             u32 value) {
    const TileChunkPosition chunkPos{ GetChunkPosition(tileMap, absTileX, absTileY, absTileZ) };
    TileChunk* tileChunk{ GetTileChunk(tileMap, chunkPos.chunkX, chunkPos.chunkY,
                                       chunkPos.chunkZ) };

    ASSERT(tileChunk);

    // Create tileChunk tiles if they don't exist
    if (!tileChunk->tiles) {
        const u32 tileCount{ tileMap->chunkSize * tileMap->chunkSize };
        tileChunk->tiles = PushArray(worldArena, tileCount, u32);

        for (u32 tileIndex{}; tileIndex < tileCount; ++tileIndex) {
            tileChunk->tiles[tileIndex] = 3;
        }
    }

    if (tileChunk) {
        SetTileValueChecked(tileMap, tileChunk, chunkPos.chunkRelativeTileX,
                            chunkPos.chunkRelativeTileY, value);
    }
}

NODISCARD
INTERNAL bool32
IsTileMapPointEmpty(const TileMap* tileMap, TileMapPosition pos) {
    const u32 value{ GetTileValue(tileMap, pos.absTileX, pos.absTileY, pos.absTileZ) };
    const bool32 empty{ value != 0 && value != blocked_Tile_Value };
    return empty;
}

INTERNAL void
ReCanonicalizeCoordinate(const TileMap* tileMap, u32* tileIndex, f32* relPos) {
    const i32 offset{ RoundF32ToI32(*relPos / tileMap->tileSideInMeters) };
    // NOTE: tileMap is assumed to be toroidal, if you step over the end you start at the
    // beginning
    *tileIndex += offset;
    *relPos -= static_cast<f32>(offset) * tileMap->tileSideInMeters;

    // TODO: what to do if: *relPos == tileMap->tilesideinpixels
    // This can happen because we do the divide and floor and then multiple, the player might
    // end up being on the same tile Relative positions must be within the tile size in pixels
    // TODO: fix the floating point math to not allow the case above of ==
    ASSERT(*relPos * 0.5f >= -tileMap->tileSideInMeters &&
           *relPos * 0.5f <= tileMap->tileSideInMeters);
}

NODISCARD
INTERNAL TileMapPosition
RecanonicalizePosition(const TileMap* tileMap, TileMapPosition pos) {
    TileMapPosition result{ pos };
    ReCanonicalizeCoordinate(tileMap, &result.absTileX, &result.tileOffsetX);
    ReCanonicalizeCoordinate(tileMap, &result.absTileY, &result.tileOffsetY);

    return result;
}

NODISCARD
INTERNAL bool32
AreOnSameTiles(const TileMapPosition* pos, const TileMapPosition* newPos) {
    const bool32 result{ pos->absTileX == newPos->absTileX && pos->absTileY == newPos->absTileY &&
                         pos->absTileZ == newPos->absTileZ };
    return result;
}

INTERNAL void
TileMapPositionModifyZChecked(const TileMap* tileMap, TileMapPosition* pos, i32 offset) {
    // Assert some limit to avoid UB cases in the extremes
    // Probably will never hit this as we most likely always move only 1 up or down
    ASSERT(-10 <= offset && offset <= 10);

    if (offset >= 0) {
        if (pos->absTileZ < (tileMap->tileChunkCountZ - offset)) {
            pos->absTileZ += offset;
        }
    } else {
        // Handle negative with a trick
        if (static_cast<u32>((-offset)) <= pos->absTileZ) {
            pos->absTileZ += offset;
        }
    }
}
