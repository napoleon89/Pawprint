#ifndef COLLECTIONS_H
#define COLLECTIONS_H

#include <engine/std.h>

template<typename T>
struct DynamicArray {
	T *data = 0;
	u32 count = 0;
	u32 capacity = 0;

	T &getRef(u32 i) const {
		return data[i];
	}

	T *getPtr(u32 i) const {
		return &data[i];
	}

	const T &getRefConst(u32 i) const {
		return data[i];
	}

	const T *getPtrConst(u32 i) const {
		return &data[i];
	}

	T getCopy(u32 i) {
		return data[i];
	}
	

	void add(const T &element) {
		if(count == capacity) {
			capacity += 16;
			data = (T *)realloc(data, capacity * sizeof(T));
			printf("Reallocing\n");
		}
		
		data[count] = element; // copy
		count++;
	}
};

template<typename T>
struct RingBufferHeap {
	T* buffer;
	s32 head;
	s32 tail;
	s32 max;
	bool full;

	
};

#endif // COLLECTIONS_H