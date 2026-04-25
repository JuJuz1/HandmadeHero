#ifndef HANDMADE_ENTITY_H
#define HANDMADE_ENTITY_H

#include "game/handmade.h"

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
struct WorldPosition;

/**
 * Low frequency entity meant to be "ticked" at a slower rate compared to high frequency
 */
struct LowEntity {
    SimEntity sim;
    WorldPosition pos;
};

#endif // HANDMADE_ENTITY_H
