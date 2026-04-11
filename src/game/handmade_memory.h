#ifndef HANDMADE_MEMORY_h
#define HANDMADE_MEMORY_h

#include "handmade.h"

struct MemoryArena {
    u8* base;
    memory_index size;
    memory_index used;
};

#endif // HANDMADE_MEMORY_H
