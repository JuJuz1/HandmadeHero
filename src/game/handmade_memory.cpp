#include "handmade_memory.h"

INTERNAL void
InitializeArena(MemoryArena* arena, u8* base, memory_index size) {
    arena->base = base;
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
