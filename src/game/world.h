#ifndef WORLD_H
#define WORLD_H

#include <engine/ecs.h>
#include <engine/math.h>

enum class EntityState {
	Unactive,
	Active,
	MAX,
};

enum class EntityArchetype : u32 {
	None = 0,
	Transform = 1 << 0, 
	Shape = 1 << 1,
	Temp = 1 << 2,
	Temp2 = 1 << 8,
};

EntityArchetype operator|(EntityArchetype left, EntityArchetype right);
EntityArchetype operator&(EntityArchetype left, EntityArchetype right);

struct Component {

};

struct TransformComponent : public Component {
	Vec3 position;
	Quat rotation;
	Vec3 scale;
};

struct ShapeComponent : public Component {
	Vec3 center;
	f32 radius;
	Vec4 color;
};

typedef void* ComponentArrayHandle;

ComponentArrayHandle allocComponentArray(EntityArchetype type);

struct ComponentBucket {
	EntityArchetype type;
	DynamicArray<Entity> entity_ids;
	DynamicArray<ComponentArrayHandle> component_arrays;

	static ComponentBucket create(EntityArchetype type);
};

struct EntityComponentDatabase {
	
};

#endif // WORLD_H