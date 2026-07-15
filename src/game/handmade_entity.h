#ifndef HANDMADE_ENTITY_H
#define HANDMADE_ENTITY_H

INTERNAL bool32
IsSet(const SimEntity* simEntity, i32 flag) {
    bool32 result{ simEntity->flags & flag };
    return result;
}

INTERNAL void
AddFlags(SimEntity* simEntity, i32 flag) {
    simEntity->flags |= flag;
}

INTERNAL void
ClearFlags(SimEntity* simEntity, i32 flag) {
    simEntity->flags &= ~flag;
}

GLOBAL constexpr Vec3 invalid_Pos{ 100000.0f, 100000.0f, 100000.0f };

INTERNAL void
MakeEntityNonSpatial(SimEntity* entity) {
    AddFlags(entity, SimEntityFlags::NON_SPATIAL);
    entity->pos = invalid_Pos;
}

INTERNAL void
MakeEntitySpatial(SimEntity* entity, Vec3 p, Vec3 dP) {
    ClearFlags(entity, SimEntityFlags::NON_SPATIAL);
    entity->pos = p;
    entity->velocity = dP;
}

NODISCARD
INTERNAL Vec3
GetEntityGroundPoint(SimEntity* entity) {
    const Vec3 result{ entity->pos + Vec3{ 0, 0, -entity->dim.z * 0.5f } };
    return result;
}

NODISCARD
INTERNAL f32
GetStairGround(SimEntity* entity, Vec3 atGroundPoint) {
    ASSERT(entity->type == EntityType::STAIRWELL);

    const Rect3 regionRect{ RectCenterDim(entity->pos, entity->dim) };
    const Vec3 bary{ Clamp01(GetBarycentric(regionRect, atGroundPoint)) };
    const f32 result{ regionRect.min.z + (bary.y * entity->walkableHeight) };

    return result;
}

#endif // HANDMADE_ENTITY_H
