#include "handmade_entity.h"

NODISCARD
INTERNAL MoveSpec
DefaultMoveSpec() {
    MoveSpec result{};
    result.speed = 1.0f;
    result.drag = 0.0f;
    result.unitMaxAccelVector = false;

    return result;
}

//INTERNAL void
//UpdateMonster(GameState* gameState, SimEntity* entity, f32 delta) {}

//INTERNAL void
//UpdateFamiliar(SimRegion* simRegion, SimEntity* entity, f32 delta) {}

//INTERNAL void
//UpdateSword(SimRegion* simRegion, SimEntity* entity, f32 delta) {}
