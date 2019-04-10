#ifndef RENDERER_H
#define RENDERER_H

#include <engine/std.h>

struct StaticMeshReference;
struct SkinnedMeshReference;

struct RenderFrame {
	StaticMeshReference *static_meshes;
	u32 static_mesh_count;

	SkinnedMeshReference *skinned_meshes;
	u32 skinned_mesh_count;
};

#endif // RENDERER_H