#include "handmade_world.h"

#include "game/handmade_intrinsics.h"
#include "game/handmade_memory.h"
#include "game/math/handmade_vec3.h"

NODISCARD
INTERNAL WorldPosition
NullWorldPos() {
    WorldPosition pos{};
    pos.chunkX = tile_Chunk_Uninitialized;
    return pos;
}

NODISCARD
INTERNAL bool32
IsValidWorldPos(WorldPosition pos) {
    const bool32 result{ pos.chunkX != tile_Chunk_Uninitialized };
    return result;
}

INTERNAL void
InitializeWorld(World* world, f32 tileSideInMeters) {
    // NOTE: This is now seperated from the rendering (tileSideInPixels)
    world->tileSideInMeters = tileSideInMeters;

    world->chunkSideInMeters = tileSideInMeters * tiles_Per_Chunk;
    world->firstFree = nullptr;

    for (i32 tileChunkIndex{}; tileChunkIndex < world->worldChunkHash.size; ++tileChunkIndex) {
        world->worldChunkHash[tileChunkIndex].chunkX = tile_Chunk_Uninitialized;
        world->worldChunkHash[tileChunkIndex].firstBlock.entityCount = 0;
    }
}

NODISCARD
INTERNAL WorldChunk*
GetWorldChunk(World* world, i32 chunkX, i32 chunkY, i32 chunkZ, MemoryArena* arena) {
    ASSERT(chunkX > -tile_Chunk_Safe_Margin);
    ASSERT(chunkY > -tile_Chunk_Safe_Margin);
    ASSERT(chunkZ > -tile_Chunk_Safe_Margin);
    ASSERT(chunkX < tile_Chunk_Safe_Margin);
    ASSERT(chunkY < tile_Chunk_Safe_Margin);
    ASSERT(chunkZ < tile_Chunk_Safe_Margin);

    // TODO: better hash function ;D
    const i32 hashValue{ 19 * chunkX + 7 * chunkY + 3 * chunkZ };
    const i32 hashSlot{ static_cast<i32>(hashValue & (world->worldChunkHash.size - 1)) };
    ASSERT(hashSlot < world->worldChunkHash.size);
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
INTERNAL bool32
IsTileValueEmpty(u32 value) {
    const bool32 result{ value != 0 && value != blocked_Tile_Value };
    return result;
}

NODISCARD
INTERNAL bool32
IsCanonical(const World* world, f32 relPos) {
    // TODO: fix the floating point math to not allow the case above of ==
    ASSERT(relPos >= -world->chunkSideInMeters * 0.5f && relPos <= world->chunkSideInMeters * 0.5f);
    const bool32 result{ relPos >= -world->chunkSideInMeters * 0.5f &&
                         relPos <= world->chunkSideInMeters * 0.5f };
    return result;
}

NODISCARD
INTERNAL bool32
IsCanonical(const World* world, Vec2 offset) {
    const bool32 result{ IsCanonical(world, offset.x) && IsCanonical(world, offset.y) };
    return result;
}

INTERNAL void
ReCanonicalizeCoordinate(const World* world, i32* tileIndex, f32* relPos) {
    const i32 offset{ RoundF32ToI32(*relPos / world->chunkSideInMeters) };
    // NOTE: Wrapping is not allowed so all coordinates are assumed to be within the safe margins
    *tileIndex += offset;

    *relPos -= static_cast<f32>(offset) * world->chunkSideInMeters;
    // TODO: what to do if: *relPos == world->tilesideinpixels
    // This can happen because we do the divide and floor and then multiple, the player might
    // end up being on the same tile Relative positions must be within the tile size in pixels
    ASSERT(IsCanonical(world, *relPos));
}

NODISCARD
INTERNAL WorldPosition
MapIntoChunkSpace(const World* world, WorldPosition pos, Vec2 offset) {
    WorldPosition result{ pos };
    result.offset_ += offset;

    ReCanonicalizeCoordinate(world, &result.chunkX, &result.offset_.x);
    ReCanonicalizeCoordinate(world, &result.chunkY, &result.offset_.y);

    return result;
}

NODISCARD
INTERNAL WorldPosition
ChunkPositionFromTilePosition(World* world, i32 tileX, i32 tileY, i32 tileZ) {
    WorldPosition result{};

    result.chunkX = tileX / tiles_Per_Chunk;
    result.chunkY = tileY / tiles_Per_Chunk;
    result.chunkZ = tileZ / tiles_Per_Chunk;

    // TODO: Align tiles with chunks accurately to prevent errors
    result.offset_.x = ((tileX - tiles_Per_Chunk / 2) - (result.chunkX * tiles_Per_Chunk)) *
                       world->tileSideInMeters;
    result.offset_.y = ((tileY - tiles_Per_Chunk / 2) - (result.chunkY * tiles_Per_Chunk)) *
                       world->tileSideInMeters;
    // TODO: Move to 3D Z

    ASSERT(IsCanonical(world, result.offset_));

    return result;
}

NODISCARD
INTERNAL bool32
AreOnSameChunk(const World* world, const WorldPosition* A, const WorldPosition* B) {
    ASSERT(IsCanonical(world, A->offset_));
    ASSERT(IsCanonical(world, B->offset_));

    const bool32 result{ A->chunkX == B->chunkX && A->chunkY == B->chunkY &&
                         A->chunkZ == B->chunkZ };
    return result;
}

NODISCARD
INTERNAL i32
WorldPositionModifyZChecked(const World* world, const WorldPosition* pos, i32 offset) {
    // Assert some limit to avoid UB cases in the extremes
    // Probably will never hit this as we most likely always move only 1 up or down
    ASSERT(-10 <= offset && offset <= 10);

    i32 newZ{ pos->chunkZ };
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

    const Vec3 dTile{ static_cast<f32>(a->chunkX) - static_cast<f32>(b->chunkX),
                      static_cast<f32>(a->chunkY) - static_cast<f32>(b->chunkY),
                      static_cast<f32>(a->chunkZ) - static_cast<f32>(b->chunkZ) };

    diff.x = (world->chunkSideInMeters * dTile.x) + a->offset_.x - b->offset_.x;
    diff.y = (world->chunkSideInMeters * dTile.y) + a->offset_.y - b->offset_.y;
    diff.z = world->chunkSideInMeters * dTile.z;

    return diff;
}

INTERNAL WorldEntityBlock*
FreeBlock(WorldEntityBlock* block) {}

INTERNAL void
ChangeEntityLocationRaw(World* world, MemoryArena* arena, i32 lowEntityIndex, WorldPosition* oldPos,
                        WorldPosition* newPos) {
    // TODO: should this move entity to high set if it is in camera bounds

    ASSERT(!oldPos || IsValidWorldPos(*oldPos));
    ASSERT(!newPos || IsValidWorldPos(*newPos));

    if (oldPos && AreOnSameChunk(world, oldPos, newPos)) {
        // Leave entity where it is
    } else {
        if (oldPos) {
            WorldChunk* chunk{ GetWorldChunk(world, oldPos->chunkX, oldPos->chunkY, oldPos->chunkZ,
                                             nullptr) };
            ASSERT(chunk);
            if (chunk) {
                // Pull the entity out of its current entity block
                // Modify blocks to match the new structure if changed

                WorldEntityBlock* firstBlock{ &chunk->firstBlock };
                bool32 notFound{ true };

                for (WorldEntityBlock* block{ firstBlock }; block && notFound;
                     block = block->next) {
                    for (i32 lowIndex{}; lowIndex < block->entityCount; ++lowIndex) {
                        // Found the block
                        if (block->lowEntityIndexes[lowIndex] == lowEntityIndex) {
                            ASSERT(firstBlock->entityCount > 0);

                            // Move the last towards the head
                            block->lowEntityIndexes[lowIndex] =
                                firstBlock->lowEntityIndexes[--firstBlock->entityCount];

                            // First block would be deleted
                            if (firstBlock->entityCount == 0) {
                                if (firstBlock->next) {
                                    WorldEntityBlock* nextBlock{ firstBlock->next };
                                    *firstBlock = *firstBlock->next;

                                    nextBlock->next = world->firstFree;
                                    //world->firstFree = FreeBlock(nextBlock);
                                    world->firstFree = nextBlock;
                                }
                            }

                            notFound = false;
                            break;
                        }
                    }
                }
            } else {
                INVALID_CODE_PATH;
            }
        }

        if (newPos) {
            WorldChunk* chunk{ GetWorldChunk(world, newPos->chunkX, newPos->chunkY, newPos->chunkZ,
                                             arena) };
            ASSERT(chunk);

            WorldEntityBlock* block{ &chunk->firstBlock };
            ASSERT(block->entityCount <= block->lowEntityIndexes.size);
            if (block->entityCount == block->lowEntityIndexes.size) {
                // Out of room, make a new one
                WorldEntityBlock* oldBlock{ world->firstFree };
                if (oldBlock) {
                    world->firstFree = oldBlock->next;
                } else {
                    oldBlock = PushSize(arena, WorldEntityBlock);
                }

                *oldBlock = *block;
                block->next = oldBlock;
                block->entityCount = 0;
            }

            // Insert into new block
            ASSERT(block->entityCount < block->lowEntityIndexes.size);
            block->lowEntityIndexes[block->entityCount++] = lowEntityIndex;
        }
    }
}

INTERNAL void
ChangeEntityLocation(World* world, MemoryArena* arena, i32 lowIndex, LowEntity* lowEntity,
                     WorldPosition* oldPos, WorldPosition* newPos) {
    ChangeEntityLocationRaw(world, arena, lowIndex, oldPos, newPos);
    if (newPos) {
        lowEntity->pos = *newPos;
    } else {
        lowEntity->pos = NullWorldPos();
    }
}
