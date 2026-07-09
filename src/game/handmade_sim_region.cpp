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
INTERNAL Vec3
GetSimSpacePos(SimRegion* region, LowEntity* stored) {
    Vec3 result{ invalid_Pos };
    if (!IsSet(&stored->sim, SimEntityFlags::NON_SPATIAL)) {
        result = SubtractWorldPos(region->world, &stored->pos, &region->origin);
    }

    return result;
}

INTERNAL SimEntity* AddEntityToSimRegion(GameState* gameState, SimRegion* simRegion, LowEntity* src,
                                         i32 lowIndex, Vec3* simPos);

INTERNAL void
LoadEntityReference(GameState* gameState, SimRegion* simRegion, EntityReference* ref) {
    if (ref->index) {
        SimEntityHash* entry{ GetEntityHashFromIndex(simRegion, ref->index) };
        if (!entry->ptr) {
            auto* lowEntity{ GetLowEntity(gameState, ref->index) };
            Vec3 pos{ GetSimSpacePos(simRegion, lowEntity) };
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
                AddFlags(&src->sim, SimEntityFlags::SIMULATING);
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
INTERNAL bool32
EntityOverlapsRect(Vec3 p, Vec3 dim, Rect3 rect) {
    const Rect3 grown{ AddRadiusTo(rect, 0.5f * dim) };
    const bool32 result{ IsInsideRectangle(grown, p) };
    return result;
}

NODISCARD
INTERNAL SimEntity*
AddEntityToSimRegion(GameState* gameState, SimRegion* simRegion, LowEntity* src, i32 lowIndex,
                     Vec3* simPos) {
    SimEntity* dest{ AddEntityToSimRegion_(gameState, simRegion, src, lowIndex) };

    if (dest) {
        if (simPos) {
            dest->pos = *simPos;
            dest->updatable = EntityOverlapsRect(dest->pos, dest->dim, simRegion->updatableBounds);
        } else {
            dest->pos = GetSimSpacePos(simRegion, src);
        }
    }

    return dest;
}

NODISCARD
INTERNAL SimRegion*
BeginSim(GameState* gameState, MemoryArena* simArena, World* world, WorldPosition origin,
         Rect3 bounds, f32 delta) {
    SimRegion* simRegion{ PushSize(simArena, SimRegion) };
    ZeroSize(simRegion->hash);

    simRegion->maxEntityCount = simRegion->hash.size; // 4096
    simRegion->entityCount = 0;
    simRegion->entities = PushArray(simArena, simRegion->maxEntityCount, SimEntity);

    // Makes the gathering region larger to include entities just outside of the region but some
    // of their part is inside the region
    simRegion->maxEntityRadius = 5.0f;
    simRegion->maxEntityVelocity = 30.0f; // TODO: revise more

    simRegion->maxRecordedEntityVelocitySq = {};
    simRegion->maxRecordedEntityVelocityIndex = {};
    simRegion->maxRecordedEntityVelocityType = {};

    // Take into account max velocity of any entity and the delta time!
    const f32 safetyMargin{ simRegion->maxEntityRadius + (simRegion->maxEntityVelocity * delta) };
    constexpr f32 safetyMarginZ{ 1.0f };

    simRegion->world = world;
    simRegion->origin = origin;
    simRegion->updatableBounds =
        AddRadiusTo(bounds, Vec3{ simRegion->maxEntityRadius, simRegion->maxEntityRadius,
                                  simRegion->maxEntityRadius });
    simRegion->bounds =
        AddRadiusTo(simRegion->updatableBounds, Vec3{ safetyMargin, safetyMargin, safetyMarginZ });

    const WorldPosition minChunk{ MapIntoChunkSpace(
        world, origin,
        Vec3{ simRegion->bounds.min.x, simRegion->bounds.min.y, simRegion->bounds.min.z }) };
    const WorldPosition maxChunk{ MapIntoChunkSpace(
        world, origin,
        Vec3{ simRegion->bounds.max.x, simRegion->bounds.max.y, simRegion->bounds.max.z }) };

    i32 movedCount{};

    // Check entities by chunk, move to high set if in the chunks close to camera
    for (i32 chunkZ{ minChunk.chunkZ }; chunkZ <= maxChunk.chunkZ; ++chunkZ) {
        for (i32 chunkY{ minChunk.chunkY }; chunkY <= maxChunk.chunkY; ++chunkY) {
            for (i32 chunkX{ minChunk.chunkX }; chunkX <= maxChunk.chunkX; ++chunkX) {
                WorldChunk* chunk{ GetWorldChunk(world, chunkX, chunkY, chunkZ, nullptr) };
                if (chunk) {
                    for (WorldEntityBlock* block{ &chunk->firstBlock }; block;
                         block = block->next) {
                        for (i32 entityIndex{}; entityIndex < block->entityCount; ++entityIndex) {
                            const i32 lowEntityIndex{ block->lowEntityIndexes[entityIndex] };
                            LowEntity* lowEntity{ GetLowEntity(gameState, lowEntityIndex) };
                            if (!IsSet(&lowEntity->sim, SimEntityFlags::NON_SPATIAL)) {
                                Vec3 simSpacePos{ GetSimSpacePos(simRegion, lowEntity) };
                                if (EntityOverlapsRect(simSpacePos, lowEntity->sim.dim,
                                                       simRegion->bounds)) {
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
        bool32 doFamiliarStopFollow{};
        bool32 doFamiliarReset{};

        for (i32 controlIndex{}; controlIndex < ARRAY_COUNT(gameState->controlledHeroes);
             ++controlIndex) {
            auto* controlled{ &gameState->controlledHeroes[controlIndex] };
            if (controlled->entityIndex == entity->storageIndex) {
                if (controlled->requestReset) {
                    doReset = true;
                    //controlled->requestReset = false;
                } else if (controlled->requestResetSword) {
                    doResetSword = true;
                } else if (controlled->requestFamiliarStopFollow) {
                    doFamiliarStopFollow = true;
                } else if (controlled->requestFamiliarReset) {
                    doFamiliarReset = true;
                }

                // TODO: Only 1 hero can request reset, if multiple only the first is processed here
                break;
            }
        }

        // TODO: doesn't work by rawdogging as we overwrite these changes when we process the
        // familiar... figure it out
        if (doFamiliarStopFollow && stored->sim.familiarIndex) {
            PRINT_I32("Familiar follow swap: ", stored->sim.familiarIndex);
            auto* familiar{ GetLowEntity(gameState, stored->sim.familiarIndex) };
            familiar->sim.followingHero = !familiar->sim.followingHero;
        } else if (doFamiliarReset && stored->sim.familiarIndex) {
            // TODO: use the EntityReference for this instead of rawdogging?
            auto* familiar{ GetLowEntity(gameState, stored->sim.familiarIndex) };
            PRINT_I32("Reset familiar: ", stored->sim.storageIndex);
            ChangeEntityLocation(world, &gameState->worldArena, familiar->sim.storageIndex,
                                 familiar, familiar->startingPos);
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
            const f32 camOffsetZ{ newCameraPos.offset_.z };
            newCameraPos = stored->pos;
            newCameraPos.offset_.z = camOffsetZ;

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
    constexpr f32 tEps{ 0.001f };
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

NODISCARD
INTERNAL bool32
CanCollide(const GameState* gameState, SimEntity* a, SimEntity* b) {
    bool32 result{};

    if (a == b) {
        return false;
    }

    if (a->storageIndex > b->storageIndex) {
        SimEntity* temp{ a };
        a = b;
        b = temp;
    }

    if (!IsSet(a, SimEntityFlags::NON_SPATIAL) && !IsSet(b, SimEntityFlags::NON_SPATIAL)) {
        result = true;
    }

    //if (a->type == EntityType::STAIRWELL || b->type == EntityType::STAIRWELL) {
    //    result = false;
    //}

    // TODO: Better hash func
    const i32 hashBucket{ static_cast<i32>(a->storageIndex &
                                           (gameState->collisionRuleHash.size - 1)) };
    for (auto* rule{ gameState->collisionRuleHash[hashBucket] }; rule; rule = rule->nextInHash) {
        if (rule->storageIndexA == a->storageIndex && rule->storageIndexB == b->storageIndex) {
            result = rule->canCollide;
            break;
        }
    }

    return result;
}

NODISCARD
INTERNAL bool32
HandleCollision(GameState* gameState, SimEntity* entity, SimEntity* hitEntity) {
    bool32 stopsOnCollision{};

    if (entity->type == EntityType::SWORD) {
        AddCollisionRule(gameState, entity->storageIndex, hitEntity->storageIndex, false);
        stopsOnCollision = false;
    } else {
        stopsOnCollision = true;
    }

    // TODO: make this so it doesn't rely on the order of the types
    SimEntity* a{ entity->type < hitEntity->type ? entity : hitEntity };
    SimEntity* b{ entity->type < hitEntity->type ? hitEntity : entity };

    if (a->type == EntityType::MONSTER && b->type == EntityType::SWORD) {
        if (a->hitPointMax > 0) {
            --a->hitPointMax;
        }

        PRINT_I32("Monster hit, curr hp: ", a->hitPointMax);
    }

    return stopsOnCollision;
}

NODISCARD
INTERNAL bool32
CanOverlap(GameState* gameState, SimEntity* mover, SimEntity* region) {
    bool32 result{};

    if (mover != region) {
        if (region->type == EntityType::STAIRWELL) {
            result = true;
        }
    }

    return result;
}

NODISCARD
INTERNAL void
HandleOverlap(GameState* gameState, SimEntity* mover, SimEntity* region, f32 delta, f32* ground) {
    if (region->type == EntityType::STAIRWELL) {
        const Rect3 regionRect{ RectCenterDim(region->pos, region->dim) };
        const Vec3 bary{ Clamp01(GetBarycentric(regionRect, mover->pos)) };

        *ground = Lerp(regionRect.min.z, regionRect.max.z, bary.y);
    }
}

NODISCARD
INTERNAL bool32
SpeculativeCollide(SimEntity* mover, SimEntity* region) {
    bool32 result{ true };
    if (region->type == EntityType::STAIRWELL) {
        const Rect3 regionRect{ RectCenterDim(region->pos, region->dim) };
        const Vec3 bary{ Clamp01(GetBarycentric(regionRect, mover->pos)) };

        const f32 ground{ Lerp(regionRect.min.z, regionRect.max.z, bary.y) };
        const f32 stepHeight{ 0.1f };
        // TODO: why these values?
        result =
            (AbsF32(mover->pos.z - ground) > stepHeight) || ((bary.y > 0.1f) && (bary.y < 0.9f));
    }

    return result;
}

INTERNAL void
MoveEntity(GameState* gameState, SimRegion* simRegion, SimEntity* entity, MoveSpec moveSpec,
           Vec3 acceleration, f32 delta) {
    ASSERT(!IsSet(entity, SimEntityFlags::NON_SPATIAL));

    // Normalize if greater than unit circle length of 1
    if (moveSpec.unitMaxAccelVector) {
        const f32 accelerationLengthSq{ LengthSq(acceleration) };
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

    const f32 gravity{ -9.8f };
    if (!IsSet(entity, SimEntityFlags::Z_SUPPORTED)) {
        acceleration += Vec3{ 0, 0, gravity };
    }

    // TODO: rename playerDelta as now we use this function for all entities
    // p' = 0.5 * at^2 + vt + p
    Vec3 playerDelta{ (0.5f * acceleration * SquareF32(delta)) + (entity->velocity * delta) };

    // v' = at + v
    entity->velocity += acceleration * delta;

    // TODO: clamp the max velocity?
    const f32 velocitySq{ LengthSq(entity->velocity) };
    ASSERT(velocitySq <= SquareF32(simRegion->maxEntityVelocity));

    // Debug code
    if (velocitySq > simRegion->maxRecordedEntityVelocitySq) {
        simRegion->maxRecordedEntityVelocitySq = velocitySq;
        simRegion->maxRecordedEntityVelocityIndex = entity->storageIndex;
        simRegion->maxRecordedEntityVelocityType = entity->type;
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
        const f32 playerDeltaLength{ Length(playerDelta) };
        // TODO: epsilon!!! we shouldn't allow lengths of 0.001 or so
        if (playerDeltaLength == 0) {
            break;
        }

        f32 tMin{ 1.0f };

        // Calculate new tMin for the remaining length allowed to move
        if (playerDeltaLength > distanceRemaining) {
            tMin = distanceRemaining / playerDeltaLength;
        }

        Vec3 wallNormal{};
        TestWallResult testWallResult{};

        SimEntity* hitEntity{};

        const Vec3 desiredPos{ entity->pos + playerDelta };

        if (!IsSet(entity, SimEntityFlags::NON_SPATIAL)) {
            for (i32 highIndex{}; highIndex < simRegion->entityCount; ++highIndex) {
                SimEntity* testEntity{ &simRegion->entities[highIndex] };
                if (CanCollide(gameState, entity, testEntity)) {
                    const Vec3 minkowskiDiameter{ testEntity->dim.x + entity->dim.x,
                                                  testEntity->dim.y + entity->dim.y,
                                                  testEntity->dim.z + entity->dim.z };

                    const Vec3 minCorner{ minkowskiDiameter * -0.5f };
                    const Vec3 maxCorner{ minkowskiDiameter * 0.5f };

                    const Vec3 relPos{ entity->pos - testEntity->pos };
                    // TODO: inclusive test on the max end?
                    if ((relPos.z < minCorner.z) && (relPos.z > maxCorner.z)) {
                        break;
                    }

                    // Test all four "walls", used for other entities as well

                    Vec3 testWallNormal{};
                    SimEntity* testHitEntity{};
                    f32 testTMin{ tMin };

                    // x
                    testWallResult = TestWall(minCorner.x, relPos.x, relPos.y, playerDelta.x,
                                              playerDelta.y, tMin, minCorner.y, maxCorner.y);
                    if (testWallResult.hit) {
                        testTMin = testWallResult.tMin;
                        testWallNormal = Vec3{ -1, 0 };
                        //hitWall = true;
                        testHitEntity = testEntity;
                    }

                    testWallResult = TestWall(maxCorner.x, relPos.x, relPos.y, playerDelta.x,
                                              playerDelta.y, tMin, minCorner.y, maxCorner.y);
                    if (testWallResult.hit) {
                        testTMin = testWallResult.tMin;
                        testWallNormal = Vec3{ 1, 0 };
                        //hitWall = true;
                        testHitEntity = testEntity;
                    }

                    // y
                    testWallResult = TestWall(minCorner.y, relPos.y, relPos.x, playerDelta.y,
                                              playerDelta.x, tMin, minCorner.x, maxCorner.x);
                    if (testWallResult.hit) {
                        testTMin = testWallResult.tMin;
                        testWallNormal = Vec3{ 0, -1 };
                        //hitwall = true;
                        testHitEntity = testEntity;
                    }

                    testWallResult = TestWall(maxCorner.y, relPos.y, relPos.x, playerDelta.y,
                                              playerDelta.x, tMin, minCorner.x, maxCorner.x);
                    if (testWallResult.hit) {
                        testTMin = testWallResult.tMin;
                        testWallNormal = Vec3{ 0, 1 };
                        //hitwall = true;
                        testHitEntity = testEntity;
                    }

                    if (testHitEntity) {
                        const Vec3 testP{ entity->pos + (playerDelta * testTMin) };
                        if (SpeculativeCollide(entity, testEntity)) {
                            tMin = testTMin;
                            wallNormal = testWallNormal;
                            hitEntity = testHitEntity;
                        }
                    }
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

            const bool32 stopsOnCollision{ HandleCollision(gameState, entity, hitEntity) };

            // Slide along
            if (stopsOnCollision) {
                playerDelta -= 1.0f * Dot(playerDelta, wallNormal) * wallNormal;
                entity->velocity -= 1.0f * Dot(entity->velocity, wallNormal) * wallNormal;
            }
        } else {
            break;
        }
    }

    // Handle events based on overlapping
    // Imagine like walking on lava, we want to affect movement and other stuff most likely!
    // Previously we added every overlapping entity to a overlap array and used that at the end
    // of this function to do stuff

    // TODO: this is based on the camera position...
    // so we need a solid concept of ground levels
    f32 ground{};

    // For debug
    i32 overLappingCount{};
    {
        const Rect3 entityRect{ RectCenterDim(entity->pos, entity->dim) };
        for (i32 highIndex{}; highIndex < simRegion->entityCount; ++highIndex) {
            SimEntity* testEntity{ &simRegion->entities[highIndex] };
            if (CanOverlap(gameState, entity, testEntity)) {
                Rect3 testEntityRect{ RectCenterDim(testEntity->pos, testEntity->dim) };
                if (RectsIntersect(entityRect, testEntityRect)) {
                    HandleOverlap(gameState, entity, testEntity, delta, &ground);
                }
            }
        }
    }

    if (entity->pos.z <= ground ||
        (IsSet(entity, SimEntityFlags::Z_SUPPORTED) && entity->velocity.z == 0.0f)) {
        entity->pos.z = ground;
        entity->velocity.z = 0;
        AddFlags(entity, SimEntityFlags::Z_SUPPORTED);
    } else {
        ClearFlags(entity, SimEntityFlags::Z_SUPPORTED);
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
    const Vec3 velocity{ entity->velocity };
    if (velocity == Vec3::ZERO) {
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
