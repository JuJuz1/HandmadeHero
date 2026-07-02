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
    STAIRWELL,
};

NODISCARD
INTERNAL const char*
EntityTypeToStr(EntityType type) {
    const char* typeStr{};
    switch (type) {
    case EntityType::WALL: {
        typeStr = "Wall";
    } break;
    case EntityType::HERO: {
        typeStr = "Hero";
    } break;
    case EntityType::FAMILIAR: {
        typeStr = "Familiar";
    } break;
    case EntityType::MONSTER: {
        typeStr = "Monstar";
    } break;
    case EntityType::SWORD: {
        typeStr = "Sword";
    } break;
    case EntityType::STAIRWELL: {
        typeStr = "Stairwell";
    } break;
    default: {
        INVALID_CODE_PATH;
    }
    }

    return typeStr;
}

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

enum SimEntityFlags : u32 {
    COLLIDES = (1 << 0),
    NON_SPATIAL = (1 << 1),
    MOVEABLE = (1 << 2),

    SIMULATING = (1 << 30),
};

// Simulated (high)
struct SimEntity {
    // NOTE: these are only for the sim region
    bool32 updatable;
    i32 storageIndex; // Index to low entity

    EntityType type;
    i32 flags;

    Vec3 dim;

    Vec3 pos; // NOTE: This is now already relative to the camera center
    Vec3 velocity;

    f32 distanceLimit; // For every entity a max limit

    i32 facingDir;

    f32 tBob;

    i32 hitPointMax;
    Array<HitPoint, 16> hitPoints;

    EntityReference sword;

    i32 familiarIndex; // Used by hero, saved by familiar of the closest hero
    bool32 followingHero;
};

/**
 * Low frequency entity meant to be "ticked" at a slower rate compared to high frequency
 */
struct LowEntity {
    SimEntity sim;
    WorldPosition pos;

    // Used when resetting
    WorldPosition startingPos;
};

struct SimEntityHash {
    SimEntity* ptr;
    i32 index;
};

struct SimRegion {
    World* world;
    WorldPosition origin;
    Rect3 bounds;
    Rect3 updatableBounds;

    f32 maxEntityRadius;
    f32 maxEntityVelocity;

    f32 maxRecordedEntityVelocitySq; // Stored in MoveEntity
    i32 maxRecordedEntityVelocityIndex;
    EntityType maxRecordedEntityVelocityType;

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
