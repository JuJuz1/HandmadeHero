#ifndef HANDMADE_MEMORY_h
#define HANDMADE_MEMORY_h

#include "handmade.h"

struct MemoryArena {
    u8* base;
    memory_index size;
    memory_index used;
};

INTERNAL void InitializeArena(MemoryArena* arena, u8* base, memory_index size);

#define PushSize(arena, type) (type*)PushSize_(arena, sizeof(type))
#define PushArray(arena, count, type) (type*)PushSize_(arena, (count) * sizeof(type))

NODISCARD
INTERNAL void* PushSize_(MemoryArena* arena, memory_index size);

#endif // HANDMADE_MEMORY_H
