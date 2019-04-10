#ifndef ECS_H
#define ECS_H

#include <engine/std.h>
#include <engine/collections.h>

typedef u32 EntityHandle;

struct Entity { // keep this fucker under 16 bytes
	EntityHandle index; 
	u32 version;
};

template<typename T>
struct ComponentMap { // This should probably be a custom dynamic array rather than two 
	DynamicArray<EntityHandle> entity_indices;
	DynamicArray<T> components;

	void addOrUpdate(EntityHandle entity, const T &component) {
		for(u32 i = 0; i < entity_indices.count; i++) {
			if(entity_indices.getRefConst(i) == entity) {
				components.getRef(i) = component;
				return;
			}
		}

		entity_indices.add(entity);
		components.add(component);
	
	}

	inline const u32 getCount() {
		return entity_indices.count;
	}

	const s32 searchIndex(EntityHandle, const u32 search_start);
};

#endif // ECS_H