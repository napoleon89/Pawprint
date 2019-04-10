#ifndef MEMORY_H
#define MEMORY_H

#include <engine/std.h>
struct MemoryBlock {
	 void *memory;
	u64 size;
};

#define MEMORY_STORE_COUNT 3

struct MemoryStore {
	void *memory;
	union {
		struct {
			MemoryBlock game_memory;
			MemoryBlock asset_memory;
			MemoryBlock frame_memory;
			
		};
		MemoryBlock blocks[MEMORY_STORE_COUNT];
	};
	
	inline u64 calcTotalSize() {
		u64 mem_total_size = 0;
		for(int i = 0; i < MEMORY_STORE_COUNT; i++) {
			mem_total_size += blocks[i].size;
		}
		return mem_total_size;
	}
};

#endif // MEMORY_H