#ifndef ALLOCATORS_H
#define ALLOCATORS_H

#include <engine/std.h>
#include <engine/math.h>

struct StackAllocator {
	private:
		u8 *memory;	
		u8 *top;
		u64 block_size;	
	public:
		typedef u32 Marker;
		
		void initialize(void *block, u64 size);
		Marker getMarker();
		void freeToMarker(Marker marker);
		void *alloc(u64 size);
		void clear();
		void *getTop();
};

struct VertexStack {
	private:
		StackAllocator *fs;
		Vec3 *vertices_start;
		u32 count;
		u32 marker;
	public:
		VertexStack(StackAllocator *frame_stack);
		void push(Vec3 vertex);
		void clear(); // NOTE(nathan): if anyone else uses the stack while this has it, this clear will fuck them up
		u64 getSize();
		u32 getCount();
		Vec3 *getPointer();
};

#endif // ALLOCATORS_H