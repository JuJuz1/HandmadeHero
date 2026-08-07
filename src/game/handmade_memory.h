#ifndef HANDMADE_MEMORY_h
#define HANDMADE_MEMORY_h

//#include "game/handmade.h"

struct MemoryArena {
    u8* base;
    memory_index size;
    memory_index used;

    i32 tempCount;
};

struct TempMemory {
    MemoryArena* arena;
    memory_index used;
};

//INTERNAL void ArenaInit(MemoryArena* arena, u8* base, memory_index size);

//#define PushStruct(arena, type) (type*)PushStruct_(arena, sizeof(type))
//#define PushArray(arena, count, type) (type*)PushStruct_(arena, (count) * sizeof(type))

//NODISCARD
//INTERNAL void* PushStruct_(MemoryArena* arena, memory_index size);

#endif // HANDMADE_MEMORY_H
