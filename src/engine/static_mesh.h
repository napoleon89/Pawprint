#ifndef STATIC_MESH_H
#define STATIC_MESH_H

#include <engine/math.h>
#include <core/render_context.h>

struct StaticMeshVertex {
	Vec3 position;
	Vec3 normal;
	Vec3 color;
};

struct StaticMeshData {
	StaticMeshVertex *vertices;
	u16 *indices;
	u32 vertex_count;
	u32 index_count;
	VertexBuffer vertex_buffer;
	VertexBuffer index_buffer;
	Vec3 min, max;
	void *physics_obj;
};

struct StaticMeshReference {
	Mat4 transform;
	u32 index;
};

#endif // STATIC_MESH_H