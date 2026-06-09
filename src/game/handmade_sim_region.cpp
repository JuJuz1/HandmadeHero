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

    SimEntity* entity{ simRegion->entities };
    for (i32 i{}; i < simRegion->entityCount; ++i, ++entity) {
        auto* stored{ GetLowEntity(gameState, entity->storageIndex) };

        ASSERT(IsSet(&stored->sim, SimEntityFlags::SIMULATING));

        stored->sim = *entity;
        ASSERT(!IsSet(&stored->sim, SimEntityFlags::SIMULATING));

        StoreEntityReference(&stored->sim.sword);

        auto newPos{ !IsSet(entity, SimEntityFlags::NON_SPATIAL)
                         ? MapIntoChunkSpace(world, simRegion->origin, entity->pos)
                         : NullWorldPos() };
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

INTERNAL void
MoveEntity(SimRegion* simRegion, SimEntity* entity, MoveSpec moveSpec, Vec2 acceleration,
           f32 delta) {
    ASSERT(!IsSet(entity, SimEntityFlags::NON_SPATIAL));
    // TODO: move player speed away from here!
    //constexpr f32 speedModifier{ 4 };

    if (moveSpec.unitMaxAccelVector) {
        // Normalize if greater than unit circle length of 1
        const f32 accelerationLengthSq{ LengthSquared(acceleration) };
        if (accelerationLengthSq > 1.0f) {
            acceleration *= (1.0f / Sqrt(accelerationLengthSq));
        }
    }

    // Other player faster for debug
    //if (controllerIndex != 0) {
    //    acceleration *= 1.5f;
    //}

    acceleration *= moveSpec.speed;

    // TODO: modify to include inputs for players
    //if (hm_input::ActionPressed(&inputButtons->shift)) {
    //    acceleration *= speedModifier;
    //}

    // TODO: ordinary differential equations
    acceleration += -moveSpec.drag * entity->velocity;
    // v' = at + v
    entity->velocity += acceleration * delta;

    // p' = 0.5 * at^2 + vt + p
    Vec2 playerDelta{ (0.5f * acceleration * SquareF32(delta)) + (entity->velocity * delta) };

    // Collision checks

    //bool32 hitWall{}; // Used to modify velocity if we hit a wall during the frame

    constexpr i32 iterationCount{ 4 };

    for (i32 iteration{}; iteration < iterationCount; ++iteration) {
        f32 tMin{ 1.0f };
        Vec2 wallNormal{};
        TestWallResult testWallResult{};
        i32 hitHighEntityIndex{};

        const Vec2 desiredPos{ entity->pos + playerDelta };

        if (IsSet(entity, SimEntityFlags::COLLIDES) &&
            !IsSet(entity, SimEntityFlags::NON_SPATIAL)) {
            for (i32 highIndex{}; highIndex < simRegion->entityCount; ++highIndex) {
                const SimEntity* testEntity{ &simRegion->entities[highIndex] };
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
                }

                testWallResult = TestWall(maxCorner.x, relPos.x, relPos.y, playerDelta.x,
                                          playerDelta.y, tMin, minCorner.y, maxCorner.y);
                if (testWallResult.hit) {
                    tMin = testWallResult.tMin;
                    wallNormal = Vec2{ 1, 0 };
                    //hitWall = true;
                    hitHighEntityIndex = highIndex;
                }

                // y
                testWallResult = TestWall(minCorner.y, relPos.y, relPos.x, playerDelta.y,
                                          playerDelta.x, tMin, minCorner.x, maxCorner.x);
                if (testWallResult.hit) {
                    tMin = testWallResult.tMin;
                    wallNormal = Vec2{ 0, -1 };
                    //hitwall = true;
                    hitHighEntityIndex = highIndex;
                }

                testWallResult = TestWall(maxCorner.y, relPos.y, relPos.x, playerDelta.y,
                                          playerDelta.x, tMin, minCorner.x, maxCorner.x);
                if (testWallResult.hit) {
                    tMin = testWallResult.tMin;
                    wallNormal = Vec2{ 0, 1 };
                    //hitwall = true;
                    hitHighEntityIndex = highIndex;
                }
            }
        }

        //PRINT_F32("tMin: ", tMin);

        entity->pos += playerDelta * tMin;
        if (hitHighEntityIndex) {
            entity->velocity -= 1.0f * Dot(entity->velocity, wallNormal) * wallNormal;
            playerDelta = desiredPos - entity->pos;
            playerDelta -= 1.0f * Dot(playerDelta, wallNormal) * wallNormal;
        } else {
            break;
        }
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
