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

INTERNAL void
UpdateMonster(GameState* gameState, SimEntity* entity, f32 delta) {
    return;
}

INTERNAL void
UpdateFamiliar(SimRegion* simRegion, SimEntity* entity, f32 delta) {
    SimEntity* closestHero{};
    constexpr f32 maxDist{ 10.0f };
    f32 closestHeroDSq{ SquareF32(maxDist) };

    // TODO: naive solution, BAD
    SimEntity* testEntity{ simRegion->entities };
    for (i32 testIndex{}; testIndex < simRegion->entityCount; ++testIndex, ++testEntity) {
        if (testEntity->type == EntityType::HERO) {
            const f32 testDSq{ LengthSquared(testEntity->pos - entity->pos) };
            if (testDSq < closestHeroDSq) {
                closestHero = testEntity;
                closestHeroDSq = testDSq;
            }
        }
    }

    const f32 stopDistSq{ SquareF32(2.25f) }; // Dist of 2.25f
    Vec2 acceleration{};

    if (closestHero && closestHeroDSq > stopDistSq) {
        constexpr f32 speed{ 0.5f };
        const f32 oneOverLength{ speed / Sqrt(closestHeroDSq) };
        acceleration = (closestHero->pos - entity->pos) * oneOverLength;
        //PRINT_F32("before: closestHeroDSq: ", closestHeroDSq);
        //PRINT_F32("before: acceleration.x: ", acceleration.x);
        //PRINT_F32("before: acceleration.y: ", acceleration.y);
        if (closestHeroDSq > 17.0f) {
            acceleration *= 1.75f;
        }

        //PRINT_F32("before: acceleration.x: ", acceleration.x);
        //PRINT_F32("before: acceleration.y: ", acceleration.y);
        //PRINT("\n");
    }

    MoveSpec moveSpec{ DefaultMoveSpec() };
    moveSpec.speed = 50.0f;
    moveSpec.drag = 8.0f;

    MoveEntity(simRegion, entity, moveSpec, acceleration, delta);
}

INTERNAL void
UpdateSword(SimRegion* simRegion, SimEntity* entity, f32 delta) {
    if (!IsSet(entity, SimEntityFlags::NON_SPATIAL)) {
        MoveSpec moveSpec{ DefaultMoveSpec() };
        // This doesn't affect the sword at all!
        moveSpec.speed = 0.0f;

        const Vec2 oldPos{ entity->pos };
        MoveEntity(simRegion, entity, moveSpec, Vec2{}, delta);

        const f32 traveled{ Length(entity->pos - oldPos) };
        entity->distanceRemaining -= traveled;
        if (entity->distanceRemaining < 0.0f) {
            MakeEntityNonSpatial(entity);
            //ASSERT(!"NEED TO MAKE ENTITIES BE ABLE TO NOT BE THERE!");
        }
    }
}
