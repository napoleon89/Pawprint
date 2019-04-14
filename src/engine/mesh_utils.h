#ifndef MESH_UTILS_H
#define MESH_UTILS_H

#include <engine/std.h>
#include <core/render_context.h>
#include <engine/math.h>
#include <engine/animation.h>

struct Platform;

struct Vertex {
	Vec3 position;
	Vec3 normal;
	Vec2 uv;
};

struct MeshWrapper {
	Vertex *vertices;
	u16 *indices;
	u32 vertex_count;
	u32 index_count;
	RenderContext::Topology topology_format;
	VertexBuffer vertex_buffer;
	VertexBuffer index_buffer;
	Mat4 local_matrix;
	Vec4 color;
};

struct SkinVertex {
	Vec3 position;
	Vec3 normal;
	Vec2 uv;
	Vec4 weights;
	s8 joint_ids[4];
};

struct SkeletalMesh {
	Skeleton skeleton;
	SkinVertex *vertices;
	u16 *indices;	
	u32 vertex_count;
	u32 index_count;
	VertexBuffer vertex_buffer;
	VertexBuffer index_buffer;
};

namespace MeshUtils {
	MeshWrapper genCubeMesh(Platform *platform, RenderContext *render_context);
	MeshWrapper genSphereMesh(Platform *platform, RenderContext *render_context, Vec3 center, f32 radius);
}


#endif // MESH_UTILS_H