#ifndef HANDMADE_ENTITY_H
#define HANDMADE_ENTITY_H

INTERNAL bool32
IsSet(const SimEntity* simEntity, i32 flag) {
    bool32 result{ simEntity->flags & flag };
    return result;
}

INTERNAL void
AddFlag(SimEntity* simEntity, i32 flag) {
    simEntity->flags |= flag;
}

INTERNAL void
ClearFlag(SimEntity* simEntity, i32 flag) {
    simEntity->flags &= ~flag;
}

GLOBAL constexpr Vec2 invalid_Pos{ 10000.0f, 10000.0f };

INTERNAL void
MakeEntityNonSpatial(SimEntity* entity) {
    AddFlag(entity, SimEntityFlags::NON_SPATIAL);
    entity->pos = invalid_Pos;
}

INTERNAL void
MakeEntitySpatial(SimEntity* entity, Vec2 p, Vec2 dP) {
    ClearFlag(entity, SimEntityFlags::NON_SPATIAL);
    entity->pos = p;
    entity->velocity = dP;
}

#endif // HANDMADE_ENTITY_H
