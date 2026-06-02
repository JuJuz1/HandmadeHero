#include "handmade_memory.h"

INTERNAL void
InitializeArena(MemoryArena* arena, void* base, memory_index size) {
    arena->base = static_cast<u8*>(base);
    arena->size = size;
    arena->used = 0;
}

NODISCARD
INTERNAL void*
PushSize_(MemoryArena* arena, memory_index size) {
    ASSERT(arena->used + size <= arena->size);
    void* result{ arena->base + arena->used };
    arena->used += size;

    return result;
}

#define PushSize(arena, type) (type*)PushSize_(arena, sizeof(type))
#define PushArray(arena, count, type) (type*)PushSize_(arena, (count) * sizeof(type))
#define ZeroSize(instance) ZeroMem(&(instance), sizeof(instance))

INTERNAL void
ZeroMem(void* ptr, memory_index size) {
    u8* byte{ static_cast<u8*>(ptr) };
    while (size--) {
        *byte++ = 0;
    }
}
