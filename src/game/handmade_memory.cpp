#include "handmade_memory.h"

INTERNAL inline void
ArenaInit(MemoryArena* arena, void* base, memory_index size) {
    arena->base = static_cast<u8*>(base);
    arena->size = size;
    arena->used = 0;
}

NODISCARD
INTERNAL inline TempMemory
BeginTempMemory(MemoryArena* arena) {
    TempMemory result{};

    result.arena = arena;
    result.used = arena->used;
    ++arena->tempCount;

    return result;
}

INTERNAL inline void
EndTempMemory(TempMemory tempMem) {
    MemoryArena* arena{ tempMem.arena };
    ASSERT(arena->used >= tempMem.used);
    arena->used = tempMem.used;
    ASSERT(arena->tempCount >= 0);
    --arena->tempCount;
}

INTERNAL inline void
ArenaCheck(MemoryArena* arena) {
    ASSERT(arena->tempCount == 0);
}

// TODO: provide a better interface!!!
#define PushStruct(arena, type) (type*)PushSize_((arena), sizeof(type))
#define PushArray(arena, count, type) (type*)PushSize_((arena), (count) * sizeof(type))
// Check return type, just use u8* or no?
#define PushSize(arena, size) PushSize_((arena), (size))
#define ZeroSize(instance) ZeroMem(&(instance), sizeof(instance))

NODISCARD
INTERNAL void*
PushSize_(MemoryArena* arena, memory_index size) {
    ASSERT(arena);
    ASSERT(arena->used + size <= arena->size);
    void* result{ arena->base + arena->used };
    arena->used += size;

    return result;
}

INTERNAL void
ZeroMem(void* ptr, memory_index size) {
    u8* byte{ static_cast<u8*>(ptr) };
    while (size--) {
        *byte++ = 0;
    }
}
