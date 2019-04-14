#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <engine/math.h>
#include <core/render_context.h>
#include <engine/mesh_utils.h>
#include <game/camera.h>

struct InputManager;
struct PlatformWindow;
struct Platform;
struct AudioEngine;
struct ModelWrapper;
struct StaticMeshData;
struct Assets;
class btCollisionWorld;
class btKinematicCharacterController;
struct PhysicsDebugDrawer;

struct Globals {
	static bool show_colliders;	
	static bool anim_finished;
	static bool wireframe;
};

struct ShaderLight {
	Vec4 position;
	Vec4 color;
	float constant_term;
	float linear_term;
	float quadratic_term;
	float padding[1];
};

struct Light {
	Vec3 position;
	Vec4 color;
	f32 radius;

	f32 calcLinearTerm();
	f32 calcQuadraticTerm();
	ShaderLight calcShaderLight();
};

struct PerSceneConstants {
	Mat4 projection;
	Mat4 view;
	Vec4 camera_position;
	Vec4 screen_size;
	ShaderLight lights[4];
};

struct HdrBlitConstants {
	Vec4 exposure;
};

struct BlurConstants {
	s32 blur_amount;
	f32 padding[3];
};

struct AOPassConstants {
	Mat4 projection;
	Vec2 sceen_size;
	f32 kernel_radius;
	f32 kernel_bias;
	Vec4 kernel[64];
	s32 kernel_size;
	f32 padding[3];
};

struct TexturedVertex {
	Vec3 position;
	Vec2 uv;	
};

struct PositionNormalPrePass {
	RenderTexture normal;
	RenderTexture position;

	void init(RenderContext *render_context, u32 width, u32 height);
	void bindForWriting(Platform *platform, RenderContext *render_context);
	void resize(RenderContext *render_context, u32 width, u32 height);
	void bindForReading(RenderContext *render_context);
	void clear(RenderContext *render_context, Vec4 color);
};

#define CHUNK_SIZE 32

struct GameState {
	f32 timer;
	Camera camera;
	Vec2 world_mouse_pos;
	Vec2 world_mouse_prev_pos;

	RasterState raster_state = {};
	RasterState wireframe_raster_state = {};
	RasterState no_depth_raster_state = {};
	DepthStencilState depth_state = {};
	DepthStencilState no_depth_state = {};

	BlendState alpha_blend;
	ShaderLayout skybox_layout = {};
	ShaderLayout textired_geom_layout = {};
	ShaderLayout vertex_layout = {};
	ShaderLayout static_vertex_layout = {};
	ShaderLayout skin_vertex_layout = {};
	ShaderConstant per_scene_constants = {};
	ShaderConstant per_object_constants = {};
	ShaderConstant hdr_blit_constants = {};
	ShaderConstant light_constants = {};
	ShaderConstant ao_pass_constants;
	ShaderConstant blur_constants;
	ShaderConstant skinned_mesh_constants = {};
	Sampler texture_sampler = {};
	Sampler wrap_texture_sampler = {};
	// RenderTexture hdr_render_texture = {};
	// DepthStencilTexture hdr_depth_texture = {};
	MeshWrapper test_sphere_mesh;
	MeshWrapper test_cube_mesh;
	PositionNormalPrePass pn_prepass;
	f32 exposure;
	Light lights[4];
	Texture2D ssao_noise_texture;
	HdrBlitConstants hdr_blit_constants_data;
	AOPassConstants ssao_pass_constant_data;
	RenderTexture ao_render_texture;
	RenderTexture hdr_color_render_texture;
	RenderTexture blur_render_texture;
	RenderTexture final_composite_texture;
	BlurConstants blur_constants_data;
	StaticMeshData terrain_mesh;
	StaticMeshReference *static_mesh_references;
	u32 static_mesh_reference_count;
	btCollisionWorld *collision_world;
	PhysicsDebugDrawer *physics_drawer;
	btKinematicCharacterController *char_controller;
	Vec3 draw_position = Vec3(-15.0f, 0.0f, 0.0f);
	Vec3 velocity;
	AnimationSequence char_sequence;
	Vec3 npc_position;
	Vec3 world_offset;
	Vec2i last_chunk_loc;
	
	void init(Platform *platform, PlatformWindow *window, RenderContext *render_context, Assets *assets, AudioEngine *audio);
	void onResize(InputManager *input, f32 delta, Platform *platform, PlatformWindow *window, Assets *assets, RenderContext *render_context);
	void genStaticMeshColliders(Assets *assets);
	void createRenderTextures(RenderContext *render_context);
	void update(InputManager *input, f32 delta, Platform *platform, PlatformWindow *window, Assets *assets, RenderContext *render_context);
	void renderGeometry(PlatformWindow *window, RenderContext *render_context, InputManager *input, Assets *assets, Platform *platform,  f32 delta, bool can_override_shader);
	void render(PlatformWindow *window, RenderContext *render_context, InputManager *input, Assets *assets, Platform *platform,  f32 delta);
	void renderFullScreenQuad(RenderContext *render_context);
	void renderModel(RenderContext *render_context, ModelWrapper *model, Vec3 position, Vec3 scale, Vec4 color, float roughness, float metallic);
	void renderMesh(RenderContext *render_context, MeshWrapper *mesh, Vec3 position, Vec3 scale, Vec4 color, float roughness, float metallic);
	void renderSkinnedMesh(RenderContext *render_context, SkeletalMesh *mesh, Vec3 position, Vec3 scale, Vec4 color, float roughness, float metallic);
	void renderStaticMesh(RenderContext *render_context, StaticMeshData *mesh, Mat4 transform, Vec4 color, float roughness, float metallic);
	
};


#endif // GAME_STATE_H