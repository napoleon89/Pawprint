#include <engine/allocators.h>

void StackAllocator::initialize(void *block, u64 size) {
	this->memory = (u8 *)block;
	top = this->memory;
	this->block_size = size;
	
}

StackAllocator::Marker StackAllocator::getMarker() {
	return (u32)(top - memory);
}

void StackAllocator::freeToMarker(Marker marker) {
	top = memory + marker;
}

void *StackAllocator::alloc(u64 size) {
	Assert((u64)((top + size) - memory) <= block_size);
	void *result = (u8 *)top;
	top += size;
	return (void *)result;
}	

void StackAllocator::clear() {
	top = memory;
}

void *StackAllocator::getTop() { return (void *)top; }

// -------------------------------------------------------------------
VertexStack::VertexStack(StackAllocator *frame_stack) {
	marker = frame_stack->getMarker();
	vertices_start = (Vec3 *)frame_stack->getTop();
	fs = frame_stack;
	count = 0;
}

void VertexStack::push(Vec3 vertex) {
	Vec3 *vp = (Vec3 *)fs->alloc(sizeof(Vec3));
	*vp = vertex;
	count++;
}

void VertexStack::clear() { // NOTE(nathan): if anyone else uses the stack while this has it, this clear will fuck them up
	fs->freeToMarker(marker);
}

u64 VertexStack::getSize() {
	return count * sizeof(Vec3);
}

u32 VertexStack::getCount() {
	return count;
}

Vec3 *VertexStack::getPointer() {
	return vertices_start;
}