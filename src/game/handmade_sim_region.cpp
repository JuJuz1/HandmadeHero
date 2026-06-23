NODISCARD
INTERNAL SimEntityHash*
GetEntityHashFromIndex(SimRegion* simRegion, i32 index) {
    ASSERT(index);

    SimEntityHash* entityHash{};
    i32 hashValue{ index };
    for (i32 offset{}; offset < simRegion->hash.size; ++offset) {
        // TODO: or just use % for clearer result...
        auto* entry{ &simRegion->hash[(hashValue + offset) & (simRegion->hash.size - 1)] };
        // Found or add a new one
        if (entry->index == index || entry->index == 0) {
            entityHash = entry;
            break;
        }
    }

    return entityHash;
}

NODISCARD
INTERNAL SimEntity*
GetEntityByIndex(SimRegion* simRegion, i32 lowIndex) {
    ASSERT(lowIndex);

    SimEntityHash* entry{ GetEntityHashFromIndex(simRegion, lowIndex) };
    SimEntity* result{ entry->ptr };
    return result;
}

INTERNAL void
StoreEntityReference(EntityReference* ref) {
    if (ref->ptr) {
        ref->index = ref->ptr->storageIndex;
    }
}

NODISCARD
INTERNAL Vec2
GetSimSpacePos(SimRegion* region, LowEntity* stored) {
    Vec2 result{ 100000.0f, 100000.0f };
    if (!IsSet(&stored->sim, SimEntityFlags::NON_SPATIAL)) {
        const auto diff{ SubtractWorldPos(region->world, &stored->pos, &region->origin) };
        result = Vec2{ diff.x, diff.y };
    }

    return result;
}

INTERNAL SimEntity* AddEntityToSimRegion(GameState* gameState, SimRegion* simRegion, LowEntity* src,
                                         i32 lowIndex, Vec2* simPos);

INTERNAL void
LoadEntityReference(GameState* gameState, SimRegion* simRegion, EntityReference* ref) {
    if (ref->index) {
        SimEntityHash* entry{ GetEntityHashFromIndex(simRegion, ref->index) };
        if (!entry->ptr) {
            auto* lowEntity{ GetLowEntity(gameState, ref->index) };
            Vec2 pos = GetSimSpacePos(simRegion, lowEntity);
            entry->ptr = AddEntityToSimRegion(gameState, simRegion, lowEntity, ref->index, nullptr);
            entry->index = ref->index;
        }

        ref->ptr = entry->ptr;
    }
}

NODISCARD
INTERNAL SimEntity*
AddEntityToSimRegion_(GameState* gameState, SimRegion* simRegion, LowEntity* src, i32 lowIndex) {
    ASSERT(lowIndex);
    SimEntity* entity{};

    SimEntityHash* entry{ GetEntityHashFromIndex(simRegion, lowIndex) };
    if (!entry->ptr) {
        if (simRegion->entityCount < simRegion->maxEntityCount) {
            entity = &simRegion->entities[simRegion->entityCount++];

            entry->index = lowIndex;
            entry->ptr = entity;

            if (src) {
                // TODO: this should not be a copy!
                *entity = src->sim;
                LoadEntityReference(gameState, simRegion, &entity->sword);

                // Debug code
                ASSERT(!IsSet(&src->sim, SimEntityFlags::SIMULATING));
                AddFlag(&src->sim, SimEntityFlags::SIMULATING);
            }

            entity->storageIndex = lowIndex;
            entity->updatable = false;
        } else {
            INVALID_CODE_PATH;
        }
    }

    return entity;
}

NODISCARD
INTERNAL SimEntity*
AddEntityToSimRegion(GameState* gameState, SimRegion* simRegion, LowEntity* src, i32 lowIndex,
                     Vec2* simPos) {
    SimEntity* dest{ AddEntityToSimRegion_(gameState, simRegion, src, lowIndex) };

    if (dest) {
        if (simPos) {
            dest->pos = *simPos;
            dest->updatable = IsInsideRectangle(simRegion->updatableBounds, dest->pos);
        } else {
            dest->pos = GetSimSpacePos(simRegion, src);
        }
    }

    return dest;
}

INTERNAL SimRegion*
BeginSim(GameState* gameState, MemoryArena* simArena, World* world, WorldPosition origin,
         Rect bounds) {
    SimRegion* simRegion{ PushSize(simArena, SimRegion) };
    ZeroSize(simRegion->hash);

    simRegion->maxEntityCount = simRegion->hash.size; // 4096
    simRegion->entityCount = 0;
    simRegion->entities = PushArray(simArena, simRegion->maxEntityCount, SimEntity);

    const f32 safetyMargin{ 1.0f };

    simRegion->world = world;
    simRegion->origin = origin;
    simRegion->updatableBounds = bounds;
    simRegion->bounds = AddRadiusTo(simRegion->updatableBounds, safetyMargin, safetyMargin);

    const WorldPosition minChunk{ MapIntoChunkSpace(world, origin, bounds.min) };
    const WorldPosition maxChunk{ MapIntoChunkSpace(world, origin, bounds.max) };

    i32 movedCount{};

    // Check entities by chunk, move to high set if in the chunks close to camera
    for (i32 chunkY{ minChunk.chunkY }; chunkY <= maxChunk.chunkY; ++chunkY) {
        for (i32 chunkX{ minChunk.chunkX }; chunkX <= maxChunk.chunkX; ++chunkX) {
            WorldChunk* chunk{ GetWorldChunk(world, chunkX, chunkY, simRegion->origin.chunkZ,
                                             nullptr) };
            if (chunk) {
                for (WorldEntityBlock* block{ &chunk->firstBlock }; block; block = block->next) {
                    for (i32 entityIndex{}; entityIndex < block->entityCount; ++entityIndex) {
                        const i32 lowEntityIndex{ block->lowEntityIndexes[entityIndex] };
                        LowEntity* lowEntity{ GetLowEntity(gameState, lowEntityIndex) };
                        if (!IsSet(&lowEntity->sim, SimEntityFlags::NON_SPATIAL)) {
                            Vec2 simSpacePos{ GetSimSpacePos(simRegion, lowEntity) };
                            if (IsInsideRectangle(bounds, simSpacePos)) {
                                AddEntityToSimRegion(gameState, simRegion, lowEntity,
                                                     lowEntityIndex, &simSpacePos);
                                ++movedCount;
                            }
                        }
                    }
                }
            }
        }
    }

    //if (movedCount > 0) {
    //    PRINT_I32("Entities moved to sim: ", movedCount);
    //}

    return simRegion;
}

INTERNAL void
EndSim(SimRegion* simRegion, GameState* gameState) {
    World* world{ gameState->world };
    i32 movedCount{};

    auto* entity{ simRegion->entities };
    for (i32 i{}; i < simRegion->entityCount; ++i, ++entity) {
        auto* stored{ GetLowEntity(gameState, entity->storageIndex) };

        ASSERT(IsSet(&stored->sim, SimEntityFlags::SIMULATING));

        stored->sim = *entity;
        ASSERT(!IsSet(&stored->sim, SimEntityFlags::SIMULATING));

        StoreEntityReference(&stored->sim.sword);

        //auto newPos{ !IsSet(entity, SimEntityFlags::NON_SPATIAL)
        //                 ? MapIntoChunkSpace(world, simRegion->origin, entity->pos)
        //                 : NullWorldPos() };

        /// Reset pos

        WorldPosition newPos{};
        bool32 doReset{};
        bool32 doResetSword{};

        for (i32 controlIndex{}; controlIndex < ARRAY_COUNT(gameState->controlledHeroes);
             ++controlIndex) {
            auto* controlled{ &gameState->controlledHeroes[controlIndex] };
            if (controlled->entityIndex == entity->storageIndex) {
                if (controlled->requestReset) {
                    doReset = true;
                    //controlled->requestReset = false;
                } else if (controlled->requestResetSword) {
                    doResetSword = true;
                }

                // TODO: Only 1 hero can request reset, if multiple only the first is processed here
                break;
            }
        }

        if (doResetSword) {
            PRINT_I32("Reset sword: ", stored->sim.sword.index);
            auto* sword{ GetLowEntity(gameState, stored->sim.sword.index) };
            // TODO: sometimes hit assert inside MoveEntity because distanceRemaining is below 0
            // TODO: @Hack do we even have to do this?
            sword->sim.velocity = {};
            sword->sim.distanceLimit = {};

            // These have to be done I guess
            MakeEntityNonSpatial(&sword->sim);
            ChangeEntityLocation(world, &gameState->worldArena, stored->sim.sword.index, sword,
                                 NullWorldPos());
        }

        if (doReset) {
            PRINT_I32("Reset: ", entity->storageIndex);
            newPos = stored->startingPos;
            stored->sim.velocity = {};
        } else {
            newPos = !IsSet(entity, SimEntityFlags::NON_SPATIAL)
                         ? MapIntoChunkSpace(world, simRegion->origin, entity->pos)
                         : NullWorldPos();
        }

        ChangeEntityLocation(world, &gameState->worldArena, entity->storageIndex, stored, newPos);
        ++movedCount;

        // Camera position
        if (entity->storageIndex == gameState->cameraFollowingEntityIndex) {
            WorldPosition newCameraPos{ gameState->cameraPos };
            newCameraPos.chunkZ = stored->pos.chunkZ;
#if 1
            //if (entity->pos.x > (9.0f * world->tileSideInMeters)) {
            //    newCameraPos.chunkX += tiles_Per_Width;
            //} else if (entity->pos.x < -(9.0f * world->tileSideInMeters)) {
            //    newCameraPos.chunkX -= tiles_Per_Width;
            //}

            //if (entity->pos.y > (5.0f * world->tileSideInMeters)) {
            //    newCameraPos.chunkY += tiles_Per_Height;
            //} else if (entity->pos.y < -(5.0f * world->tileSideInMeters)) {
            //    newCameraPos.chunkY -= tiles_Per_Height;
            //}
#else
            // Tile snap scrolling
            if (cameraFollowingentity->pos.x > (1.0f * world->tileSideInMeters)) {
                newCameraPos.chunkX += 1;
            } else if (cameraFollowingentity->pos.x < -(1.0f * world->tileSideInMeters)) {
                newCameraPos.chunkX -= 1;
            }

            if (cameraFollowingentity->pos.y > (1.0f * world->tileSideInMeters)) {
                newCameraPos.chunkY += 1;
            } else if (cameraFollowingentity->pos.y < -(1.0f * world->tileSideInMeters)) {
                newCameraPos.chunkY -= 1;
            }
#endif
            // Fully smooth scrolling
            newCameraPos = stored->pos;

            gameState->cameraPos = newCameraPos;
        }
    }

    //if (movedCount > 0) {
    //    PRINT_I32("Entities moved to back to low: ", movedCount);
    //}
}

INTERNAL TestWallResult
TestWall(f32 wallX, f32 relX, f32 relY, f32 playerDeltaX, f32 playerDeltaY, f32 tMin, f32 minY,
         f32 maxY) {
    // TODO: this should be moved elsewhere and not be in playerDelta space
    constexpr f32 tEps{ 0.0001f };
    TestWallResult result{};
    f32 newTMin{ tMin };

    if (playerDeltaX != 0.0f) {
        const f32 tResult{ (wallX - relX) / playerDeltaX };
        const f32 newY{ relY + (tResult * playerDeltaY) };
        if (tResult >= 0.0f && tResult < tMin) {
            if (newY >= minY && newY <= maxY) {
                newTMin = MAX(0.0f, tResult - tEps);
                result.tMin = newTMin;
                result.hit = true;
            }
        }
    }

    return result;
}

// TODO: make this so it doesn't rely on the order of the types
INTERNAL void
HandleCollision(SimEntity* a, SimEntity* b) {
    if (a->type == EntityType::MONSTER && b->type == EntityType::SWORD) {
        PRINT_I32("Monster hit, curr hp: ", a->hitPointMax);
        --a->hitPointMax;
        MakeEntityNonSpatial(b);
    }
}

INTERNAL void
MoveEntity(SimRegion* simRegion, SimEntity* entity, MoveSpec moveSpec, Vec2 acceleration,
           f32 delta) {
    ASSERT(!IsSet(entity, SimEntityFlags::NON_SPATIAL));

    // Normalize if greater than unit circle length of 1
    if (moveSpec.unitMaxAccelVector) {
        const f32 accelerationLengthSq{ LengthSquared(acceleration) };
        if (accelerationLengthSq > 1.0f) {
            acceleration *= (1.0f / Sqrt(accelerationLengthSq));
        }
    }

    // Other player faster for debug
    //if (controllerIndex != 0) {
    //    acceleration *= 1.5f;
    //}

    // acceleration <=> ddP
    acceleration *= moveSpec.speed;

    // TODO: ordinary differential equations
    acceleration += -moveSpec.drag * entity->velocity;
    // v' = at + v
    entity->velocity += acceleration * delta;

    // TODO: rename playerDelta as now we use this function for all entities
    // p' = 0.5 * at^2 + vt + p
    Vec2 playerDelta{ (0.5f * acceleration * SquareF32(delta)) + (entity->velocity * delta) };

    const f32 gravity{ -9.8f };
    entity->z = 0.5f * gravity * SquareF32(delta) + entity->dZ * delta + entity->z;
    entity->dZ = gravity * delta + entity->dZ;
    if (entity->z < 0.0f) {
        entity->z = 0.0f;
    }

    /// Collision checks

    //bool32 hitWall{}; // Used to modify velocity if we hit a wall during the frame

    // Now we limit distance moved
    f32 distanceRemaining{ entity->distanceLimit };
    if (distanceRemaining == 0.0f) {
        // TODO: make this not a magic number?
        distanceRemaining = 10000.0f;
    }

    constexpr i32 iterationCount{ 4 };

    for (i32 iteration{}; iteration < iterationCount; ++iteration) {
        f32 tMin{ 1.0f };

        f32 playerDeltaLength{ Length(playerDelta) };
        // TODO: epsilon!!! we shouldn't allow lengths of 0.001 or so
        if (playerDeltaLength == 0) {
            break;
        }

        // Calculate new tMin for the remaining length allowed to move
        if (playerDeltaLength > distanceRemaining) {
            tMin = distanceRemaining / playerDeltaLength;
        }

        Vec2 wallNormal{};
        TestWallResult testWallResult{};

        const bool32 stopsOnCollision{ IsSet(entity, SimEntityFlags::COLLIDES) };

        i32 hitHighEntityIndex{}; // Probably not needed
        SimEntity* hitEntity{};

        const Vec2 desiredPos{ entity->pos + playerDelta };

        if (!IsSet(entity, SimEntityFlags::NON_SPATIAL)) {
            for (i32 highIndex{}; highIndex < simRegion->entityCount; ++highIndex) {
                SimEntity* testEntity{ &simRegion->entities[highIndex] };
                // Check if testEntity collides and don't compare to self!
                if (!IsSet(testEntity, SimEntityFlags::COLLIDES) ||
                    IsSet(testEntity, SimEntityFlags::NON_SPATIAL) || testEntity == entity) {
                    continue;
                }

                const f32 diameterWidth{ testEntity->width + entity->width };
                const f32 diameterHeight{ testEntity->height + entity->height };
                const Vec2 minCorner{ Vec2{ -diameterWidth, -diameterHeight } * 0.5f };
                const Vec2 maxCorner{ Vec2{ diameterWidth, diameterHeight } * 0.5f };

                const Vec2 relPos{ entity->pos - testEntity->pos };

                // Test all four walls

                // x
                testWallResult = TestWall(minCorner.x, relPos.x, relPos.y, playerDelta.x,
                                          playerDelta.y, tMin, minCorner.y, maxCorner.y);
                if (testWallResult.hit) {
                    tMin = testWallResult.tMin;
                    wallNormal = Vec2{ -1, 0 };
                    //hitWall = true;
                    hitHighEntityIndex = highIndex;
                    hitEntity = testEntity;
                }

                testWallResult = TestWall(maxCorner.x, relPos.x, relPos.y, playerDelta.x,
                                          playerDelta.y, tMin, minCorner.y, maxCorner.y);
                if (testWallResult.hit) {
                    tMin = testWallResult.tMin;
                    wallNormal = Vec2{ 1, 0 };
                    //hitWall = true;
                    hitHighEntityIndex = highIndex;
                    hitEntity = testEntity;
                }

                // y
                testWallResult = TestWall(minCorner.y, relPos.y, relPos.x, playerDelta.y,
                                          playerDelta.x, tMin, minCorner.x, maxCorner.x);
                if (testWallResult.hit) {
                    tMin = testWallResult.tMin;
                    wallNormal = Vec2{ 0, -1 };
                    //hitwall = true;
                    hitHighEntityIndex = highIndex;
                    hitEntity = testEntity;
                }

                testWallResult = TestWall(maxCorner.y, relPos.y, relPos.x, playerDelta.y,
                                          playerDelta.x, tMin, minCorner.x, maxCorner.x);
                if (testWallResult.hit) {
                    tMin = testWallResult.tMin;
                    wallNormal = Vec2{ 0, 1 };
                    //hitwall = true;
                    hitHighEntityIndex = highIndex;
                    hitEntity = testEntity;
                }
            }
        }

        //PRINT_F32("tMin: ", tMin);

        entity->pos += playerDelta * tMin;
        distanceRemaining -= playerDeltaLength * tMin;
        distanceRemaining = MAX(distanceRemaining, 0.0f); // Do this just for safety?
        // This is sometimes hit when the sword is reset
        //ASSERT(distanceRemaining >= 0);

        if (hitEntity) {
            playerDelta = desiredPos - entity->pos;
            // Slide along
            if (stopsOnCollision) {
                entity->velocity -= 1.0f * Dot(entity->velocity, wallNormal) * wallNormal;
                playerDelta -= 1.0f * Dot(playerDelta, wallNormal) * wallNormal;
            }

            // TODO: collision table

            // Make sure a is always before b in terms of the type so we avoid duplication
            SimEntity* a{ entity->type < hitEntity->type ? entity : hitEntity };
            SimEntity* b{ entity->type < hitEntity->type ? hitEntity : entity };
            HandleCollision(a, b);
        } else {
            break;
        }
    }

    if (entity->distanceLimit != 0.0f) {
        entity->distanceLimit = distanceRemaining;
    }

    // Delta independent friction using exponential decay: e^(-kt)
    //if (hitWall) {
    //    constexpr f32 frictionModifier{ 2.0f };
    //    const f32 friction{ ExpF32(-frictionModifier * delta) };
    //    entity->velocity *= friction;
    //}

    // Facing direction checks
    const Vec2 velocity{ entity->velocity };
    if (velocity == Vec2::ZERO) {
        // Keep previous
    } else if (AbsF32(velocity.x) > AbsF32(velocity.y)) {
        if (velocity.x > 0) {
            entity->facingDir = 3;
        } else {
            entity->facingDir = 1;
        }
    } else {
        if (velocity.y > 0) {
            entity->facingDir = 2;
        } else {
            entity->facingDir = 0;
        }
    }
}
