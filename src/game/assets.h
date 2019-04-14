#ifndef ASSETS_H
#define ASSETS_H

#include <core/render_context.h>
#include <engine/static_mesh.h>
#include <engine/mesh_utils.h>
#include <engine/vox.h>
#include <engine/animation.h>

struct ModelWrapper {
	MeshWrapper *meshes;
	u32 mesh_count;
};

struct StaticMeshDatabase {
	char **names;
	StaticMeshData *meshes;
	int count;
};

struct Assets {
	struct {
		Shader geometry;
		Shader imgui;
		Shader skybox;
		Shader hdr_blit;
		Shader pbr;
		Shader geometry_pn;
		Shader ao_pass;
		Shader blur;
		Shader statics;
		Shader editor;
		Shader skinned;
	} shaders;

	TextureCube skybox;
	Texture2D test;
	Texture2D env_map;
	Texture2D irr_map;
	Texture2D debug;

	VoxModel chr_old;
	ModelWrapper model;
	ModelWrapper tree2;
	ModelWrapper trees;
	ModelWrapper car;
	ModelWrapper well;
	StaticMeshData test_static;
	StaticMeshDatabase static_meshes;
	SkeletalMesh skel_dude;
	AnimationClip walking_anim;
	AnimationClip running_anim;
	AnimationClip turning_anim;
};

Texture2D loadTexture(u8 *image_data, u64 image_size, RenderContext *render_context, RenderContext::Format format = RenderContext::Format::u32_unorm, int force_components = 0);
Texture2D loadTextureFromFile(Platform *platform, char *filename, RenderContext *render_context, char *extension = "png", RenderContext::Format format = RenderContext::Format::u32_unorm, int force_components = 0);
Shader loadShaderFromFile(Platform *platform, RenderContext *render_context, std::string shader_name);
TextureCube loadTextureCubeFromFolder(Platform *platform, RenderContext *render_context, std::string folder_name);
ModelWrapper loadModelFromMemory(Platform *platform, RenderContext *render_context, u8 *memory, u64 size);
ModelWrapper loadModelFromFile(Platform *platform, RenderContext *render_context, const char *filepath);
StaticMeshData loadStaticMeshFromFile(Platform *platform, RenderContext *render_context, const char *filepath);
SkeletalMesh loadSkeletalMeshFromFile(Platform *platform, RenderContext *render_context, const char *filepath);
AnimationClip loadClipFromFile(Platform *platform, Skeleton *skeleton, const char *filepath);
#endif // ASSETS_H