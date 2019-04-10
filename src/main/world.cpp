#include <game/world.h>

ComponentArrayHandle allocComponentArray(EntityArchetype type) {
	switch(type) {
		case EntityArchetype::Transform: {
			return (ComponentArrayHandle)(new DynamicArray<TransformComponent>());
		} break;

		case EntityArchetype::Shape: {
			return (ComponentArrayHandle)(new DynamicArray<ShapeComponent>());
		} break;

		default: return 0;
	}
}

EntityArchetype operator|(EntityArchetype left, EntityArchetype right) {
	return (EntityArchetype)((u32)left | (u32)right);
}

EntityArchetype operator&(EntityArchetype left, EntityArchetype right) {
	return (EntityArchetype)((u32)left & (u32)right);
}