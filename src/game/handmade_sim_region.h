#ifndef HANDMADE_SIM_REGION_H
#define HANDMADE_SIM_REGION_H

//#include "handmade.h"

//#include "game/handmade_entity.h"
//#include "game/handmade_memory.h"
//#include "game/handmade_world.h"
//#include "game/math/handmade_rect.h"
//#include "game/math/handmade_vec2.h"

struct MoveSpec {
    f32 speed;
    f32 drag;
    bool32 unitMaxAccelVector;
};

/// Entities ///

enum class EntityType {
    NON_EXISTENT = 0,

    WALL,
    HERO,
    FAMILIAR,
    MONSTER,
    SWORD,
};

GLOBAL constexpr i32 hit_Point_Sub_Count{ 4 };

struct HitPoint {
    i8 flags;
    i8 filledAmount;
};

struct SimEntity;

// A reference used to address sim entities with a storage (low) index
struct EntityReference {
    union {
        SimEntity* ptr;
        i32 index;
    };
};

// Simulated (high)
struct SimEntity {
    EntityType type;

    f32 width, height;
    Vec2 velocity;

    bool32 collides;
    i32 dChunkZ; // Stairs

    Vec2 pos; // NOTE: This is now already relative to the camera center
    u32 chunkZ;

    f32 z;
    f32 dZ;

    i32 facingDir;

    f32 tBob;

    i32 hitPointMax;
    Array<HitPoint, 16> hitPoints;

    EntityReference sword;
    f32 distanceRemaining; // How far the sword will go

    i32 storageIndex; // Index to low entity
};

/**
 * Low frequency entity meant to be "ticked" at a slower rate compared to high frequency
 */
struct LowEntity {
    SimEntity sim;
    WorldPosition pos;
};

struct SimEntityHash {
    SimEntity* ptr;
    i32 index;
};

struct SimRegion {
    World* world;
    WorldPosition origin;
    Rect bounds;

    i32 maxEntityCount;
    i32 entityCount;
    SimEntity* entities;

    Array<SimEntityHash, 4096> hash;
};

struct TestWallResult {
    f32 tMin;
    bool32 hit;
};

#endif // HANDMADE_SIM_REGION_H
