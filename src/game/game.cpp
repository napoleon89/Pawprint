#include <game/game.h>
#include <engine/std.h>
#include <stdlib.h>
#include <stdio.h>
#include <engine/math.h>
#include <imgui/ImGuizmo.h>
#include <engine/input.h>
#include <float.h>
#include <string.h>
#include <vector>
#include <engine/allocators.h>
#include <time.h>
#include <engine/audio.h>
#include <game/assets.h>
#include <engine/mesh_utils.h>
#include <btBulletDynamicsCommon.h>
#include <BulletDynamics/Character/btKinematicCharacterController.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <game/game_state.h>
#include <engine/debug_renderer.h>
#include <game/FastNoise.h>
#include <time.h>

global_variable StackAllocator* g_frame_stack;

bool Globals::show_colliders = false;
bool Globals::anim_finished = false;
bool Globals::wireframe = false;

struct PerObjectConstants {
	Mat4 model;
	Vec4 color;
	f32 metallic;
	f32 roughness;
	f32 padding[2];
};

struct SkinnedMeshConstants : public PerObjectConstants {
	Mat4 skining_matrices[128];
};


Vec3 b2V(btVector3 input) {
	return Vec3(input.x(), input.y(), input.z());
}

f32 Light::calcLinearTerm() {
	f32 result = 0.004679856f + (1873519.0f - 0.004679856f)/(1.0f + Math::pow((radius/0.000009322118f), 1.094728f));
	return result;
}

f32 Light::calcQuadraticTerm() {
	f32 result = 0.003874546f + (3449678.0f - 0.003874546f)/(1.0f + Math::pow((radius/0.01044441f), 2.223432f));
	return result;
}

ShaderLight Light::calcShaderLight() {
	ShaderLight result = {};
	result.position = Vec4(position, 1.0f);
	result.color = color;
	result.constant_term = 1.0f;
	result.linear_term = calcLinearTerm();
	result.quadratic_term = calcQuadraticTerm();
	return result;
}

void PositionNormalPrePass::init(RenderContext *render_context, u32 width, u32 height) {
	normal = render_context->createRenderTexture(width, height, RenderContext::Format::Vec4);
	position = render_context->createRenderTexture(width, height, RenderContext::Format::Vec4);
}

void PositionNormalPrePass::bindForWriting(Platform *platform, RenderContext *render_context) {
	RenderTexture *rts[] = {
		&normal,
		&position,
	};
	render_context->bindRenderTextures(platform, rts, ArrayCount(rts), 0, true);
}

void PositionNormalPrePass::resize(RenderContext *render_context, u32 width, u32 height) {
	render_context->destroyRenderTexture(&normal);
	render_context->destroyRenderTexture(&position);
	init(render_context, width, height);
}

void PositionNormalPrePass::bindForReading(RenderContext *render_context) {
	render_context->bindTexture2D(&normal.texture, 0);
	render_context->bindTexture2D(&position.texture, 1);
}

void PositionNormalPrePass::clear(RenderContext *render_context, Vec4 color) {
	render_context->clearRenderTexture(&normal, color.xyzw);
	render_context->clearRenderTexture(&position, color.xyzw);
}

struct PhysicsDebugDrawer : public btIDebugDraw {

	void drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &color) override {
		DebugRenderQueue::addLine(
			Vec3(from.x(), from.y(), from.z()),
			Vec3(to.x(), to.y(), to.z()),
			Vec4(color.x(), color.y(), color.z(), 1.0f)
		);
		
	}

	int getDebugMode() const override {
		return btIDebugDraw::DebugDrawModes::DBG_DrawWireframe;
	}

	void setDebugMode (int debugMode) {

	}
	
	void draw3dText (const btVector3 &location, const char *textString) override {

	}

	void drawContactPoint (const btVector3 &PointOnB, const btVector3 &normalOnB, btScalar distance, int lifeTime, const btVector3 &color) override {

	}

	void reportErrorWarning (const char *warningString) {
		printf("Physics error: %s\n", warningString);
	}

	void clearLines() override {
		
	}

	void freeMemory() {
		
	}

	void flushLines() override {
		
	}
};

global_variable Vec4 g_color_lookups[] = {
	Vec4(0.98f, 0.87f, 0.64f, 1.0f),
	Vec4(1.00f, 0.96f, 0.76f, 1.0f),
	Vec4(1.00f, 0.93f, 0.69f, 1.0f),
	Vec4(0.96f, 0.86f, 0.56f, 1.0f),
	Vec4(0.91f, 0.82f, 0.49f, 1.0f),
	Vec4(0.82f, 0.78f, 0.36f, 1.0f),
	Vec4(0.85f, 0.82f, 0.35f, 1.0f),
	Vec4(0.94f, 0.91f, 0.44f, 1.0f),
	Vec4(0.82f, 0.81f, 0.36f, 1.0f),
	Vec4(0.71f, 0.74f, 0.30f, 1.0f),
	Vec4(0.73f, 0.76f, 0.31f, 1.0f),
	Vec4(0.76f, 0.78f, 0.34f, 1.0f),
};

#define MAP_SIZE 64
global_variable u8 g_map[MAP_SIZE][MAP_SIZE];

internal_func void generateMap() {
	u32 map_height = ArrayCount(g_color_lookups);
	FastNoise noise_gen;
	noise_gen.SetSeed(time(0));
	for(int z = 0; z < MAP_SIZE; z++) {
		for(int x = 0; x < MAP_SIZE; x++) {
			float noise = (noise_gen.GetValueFractal(x, z) + 1.0f) * 0.5f;
			u8 y = (u8)(noise * map_height);
			g_map[z][x] = y;
		}
	}
}

void GameState::init(Platform *platform, PlatformWindow *window, RenderContext *render_context, Assets *assets, AudioEngine *audio) {
	DebugRenderQueue::init(render_context, &assets->shaders.editor);
	timer = 0.0f;
	exposure = 1.0f;
	camera.position = Vec3(0.0f, 0.0f, -5.0f);
	camera.yaw = 90.0f;
	camera.beginFrame(platform, window);

	npc_position = Vec3(10.0f, 0.0f, 0.0f);

	f32 light_value = 300.0f;
	lights[0] = {
		Vec3(-10.0f,  10.0f, -10.0f),
		Vec4(light_value, light_value, light_value, 10.0f),
		10.0f,
	};
	
	
	lights[1] = {
		Vec3( 10.0f,  10.0f, -10.0f),
		Vec4(light_value, light_value, light_value, 1.0f),
		10.0f,
	};
	
	lights[2] = {
		Vec3(-10.0f, -10.0f, -10.0f),
		Vec4(light_value, light_value, light_value, 1.0f),
		10.0f,
	};
	
	lights[3] = {
		Vec3( 10.0f, -10.0f, -10.0f),
		Vec4(light_value, light_value, light_value, 1.0f),
		10.0f,
	};

	test_sphere_mesh = MeshUtils::genSphereMesh(platform, render_context, Vec3(), 1.0f);
	test_cube_mesh = MeshUtils::genCubeMesh(platform, render_context);

	alpha_blend = render_context->createBlendState();
	raster_state = render_context->createRasterState(false, true, true);
	wireframe_raster_state = render_context->createRasterState(false, true, true, true);
	depth_state = render_context->createDepthStencilState(true);
	no_depth_state = render_context->createDepthStencilState(false);
	no_depth_raster_state = render_context->createRasterState(false, false, true);

	RenderContext::LayoutElement layout[] = {
		{"POSITION", RenderContext::Format::Vec3}
	};

	RenderContext::LayoutElement static_layout[] = {
		{"POSITION", RenderContext::Format::Vec3},
		{"NORMAL", RenderContext::Format::Vec3},
		{"COLOR", RenderContext::Format::Vec3},
	};

	RenderContext::LayoutElement textured_layout[] = {
		{"POSITION", RenderContext::Format::Vec3},
		{"TEXCOORD", RenderContext::Format::Vec2}
	};

	RenderContext::LayoutElement vertex_layout_desc[] = {
		{"POSITION", RenderContext::Format::Vec3},
		{"NORMAL", RenderContext::Format::Vec3},
		{"TEXCOORD", RenderContext::Format::Vec2},
	};

	RenderContext::LayoutElement skin_vertex_layout_desc[] = {
		{"POSITION", RenderContext::Format::Vec3}, // 
		{"NORMAL", RenderContext::Format::Vec3},
		{"TEXCOORD", RenderContext::Format::Vec2},
		{"BLENDWEIGHT", RenderContext::Format::Vec4}, // weights 
		{"BLENDINDICES", RenderContext::Format::Int4}, // weights 
	};

	skybox_layout = render_context->createShaderLayout(layout, ArrayCount(layout), &assets->shaders.skybox);
	textired_geom_layout = render_context->createShaderLayout(textured_layout, ArrayCount(textured_layout), &assets->shaders.hdr_blit);
	vertex_layout = render_context->createShaderLayout(vertex_layout_desc, ArrayCount(vertex_layout_desc), &assets->shaders.geometry);
	static_vertex_layout = render_context->createShaderLayout(static_layout, ArrayCount(static_layout), &assets->shaders.statics);
	skin_vertex_layout = render_context->createShaderLayout(skin_vertex_layout_desc, ArrayCount(skin_vertex_layout_desc), &assets->shaders.skinned);

	per_scene_constants = render_context->createShaderConstant(sizeof(PerSceneConstants));
	per_object_constants = render_context->createShaderConstant(sizeof(PerObjectConstants));
	hdr_blit_constants = render_context->createShaderConstant(sizeof(HdrBlitConstants));
	light_constants = render_context->createShaderConstant(sizeof(ShaderLight));
	ao_pass_constants = render_context->createShaderConstant(sizeof(AOPassConstants));
	blur_constants = render_context->createShaderConstant(sizeof(BlurConstants));
	skinned_mesh_constants = render_context->createShaderConstant(sizeof(SkinnedMeshConstants));

	texture_sampler = render_context->createSampler();
	wrap_texture_sampler = render_context->createSampler(true);
	pn_prepass.init(render_context, camera.window_dimensions.x, camera.window_dimensions.y);
	// hdr_render_texture = render_context->createRenderTexture(camera.window_dimensions.x, camera.window_dimensions.y, RenderContext::Format::Vec4, 1);
	// hdr_depth_texture = render_context->createDepthStencilTexture(camera.window_dimensions.x, camera.window_dimensions.y, 1);

	Vec3 kernel_noise[16];
	{
		std::uniform_real_distribution<float> random_floats(0.0f, 1.0f);
		std::default_random_engine generator;
		for(u32 i = 0; i < 64; i++) {
			Vec4 sample = Vec4(
				random_floats(generator) * 2.0f - 1.0f,
				random_floats(generator) * 2.0f - 1.0f,
				random_floats(generator),
				0.0f
			);

			sample = Vec4(Vec3::normalize(sample.xyz), 0.0f);
			sample *= random_floats(generator);
			f32 scale = (f32)i / 64.0f;
			scale = Math::lerp(0.1f, 1.0f, scale * scale);
			sample *= scale;
			ssao_pass_constant_data.kernel[i] = sample;
		}

		for(u32 i = 0; i < 16; i++) {
			kernel_noise[i] = Vec3(
				random_floats(generator) * 2.0f - 1.0f,
				random_floats(generator) * 2.0f - 1.0f,
				0.0f
			);
		}
	}
	ssao_pass_constant_data.kernel_radius = 1.0f;
	ssao_pass_constant_data.kernel_bias = 0.0f;
	ssao_pass_constant_data.kernel_size = 32;
	ssao_noise_texture = render_context->createTexture2D(&kernel_noise, 4, 4, RenderContext::Format::Vec3);
	blur_constants_data.blur_amount = 4;


	createRenderTextures(render_context);

	{
		u32 terrain_patch_size = 32;
		StaticMeshVertex *vertices = (StaticMeshVertex *)platform->alloc(sizeof(StaticMeshVertex) * terrain_patch_size * terrain_patch_size);
		u32 vertex_index = 0;
		for(u32 y = 0; y < terrain_patch_size; y++) {
			for(u32 x = 0; x < terrain_patch_size; x++) {
				StaticMeshVertex vertex = {};
				vertex.position = Vec3(x, 0.0f, y);
				vertex.normal = Vec3::up;
				vertex.color = Vec3(0.2f, 1.0f, 0.2f);
				vertices[vertex_index++] = vertex;
			}
		}

		u16 *indices = (u16 *)platform->alloc(sizeof(u16) * (terrain_patch_size-1) * (terrain_patch_size-1) * 3);

		for(u32 y = 0; y < terrain_patch_size; y++) {
			for(u32 x = 0; x < terrain_patch_size; x++) {
				
			}
		}
	}

	
	btDefaultCollisionConfiguration *collision_config = new btDefaultCollisionConfiguration();
	btCollisionDispatcher *dispatcher = new btCollisionDispatcher(collision_config);
	btBroadphaseInterface *overlapping_pair_cache = new btDbvtBroadphase();
	overlapping_pair_cache->getOverlappingPairCache()->setInternalGhostPairCallback(new btGhostPairCallback());
	btSequentialImpulseConstraintSolver *solver = new btSequentialImpulseConstraintSolver();
	collision_world = new btCollisionWorld(dispatcher, overlapping_pair_cache, collision_config);
	physics_drawer = new PhysicsDebugDrawer();
	collision_world->setDebugDrawer(physics_drawer);


	btStaticPlaneShape *plane = new btStaticPlaneShape(btVector3(0, 1, 0), 0.0f);
	btCollisionObject *plane_obj = new btCollisionObject();
	plane_obj->setCollisionShape(plane);
	collision_world->addCollisionObject(plane_obj);
	
	btCapsuleShapeZ *player_collider = new btCapsuleShapeZ(1.0f, 0.5f);

	btPairCachingGhostObject *ghost_object = new btPairCachingGhostObject();
	ghost_object->setWorldTransform(btTransform(btQuaternion(0, 0, 0, 1), btVector3(0, 10, 0)));
	ghost_object->setCollisionFlags(btCollisionObject::CF_CHARACTER_OBJECT);
	ghost_object->setCollisionShape(player_collider);
	
	char_controller = new btKinematicCharacterController(ghost_object, player_collider, 0.5f, btVector3(0, 1, 0));
	char_controller->setGravity(btVector3(0, 0, 0));
	
	collision_world->addCollisionObject(ghost_object, btBroadphaseProxy::CharacterFilter, btBroadphaseProxy::StaticFilter|btBroadphaseProxy::DefaultFilter);

	btCollisionObject *obs_obj = new btCollisionObject();
	btBoxShape *obs_shape = new btBoxShape(btVector3(6.0f, 3.0f, 6.0f));
	obs_obj->setCollisionShape(obs_shape);
	obs_obj->setWorldTransform(btTransform(btQuaternion(0, 0, 0, 1), btVector3(0, 0, 0)));
	collision_world->addCollisionObject(obs_obj, btBroadphaseProxy::StaticFilter);

	// for(int i = 0; i < assets->static_meshes.count; i++) {
	// 	StaticMeshData *mesh_data = &assets->static_meshes.meshes[i];
	// 	btTriangleMesh *mesh_shape = new btTriangleMesh();
	// 	btIndexedMesh indexed_mesh;
	// 	indexed_mesh.m_indexType = PHY_ScalarType::PHY_SHORT;
	// 	indexed_mesh.m_numTriangles = mesh_data->index_count;
	// 	indexed_mesh.m_numVertices = mesh_data->vertex_count;
	// 	indexed_mesh.m_triangleIndexStride = sizeof(u16);
	// 	indexed_mesh.m_vertexStride = sizeof(StaticMeshVertex);
	// 	indexed_mesh.m_vertexType = PHY_ScalarType::PHY_FLOAT;
	// 	indexed_mesh.m_triangleIndexBase = (u8 *)mesh_data->indices;
	// 	indexed_mesh.m_vertexBase = (u8 *)mesh_data->vertices;
	// 	mesh_shape->addIndexedMesh(indexed_mesh, PHY_ScalarType::PHY_SHORT);
		
	// 	btBvhTriangleMeshShape *shape = new btBvhTriangleMeshShape(mesh_shape, false);
	// 	mesh_data->physics_obj = (void *)shape;
	// }

	// StaticMeshReference *reference = new StaticMeshReference();
	// reference->index = 29;
	// reference->transform = Mat4::translate(Vec3(-10.0f, 0.0f, 0.0f));
	// static_mesh_references = reference;
	// static_mesh_reference_count = 1;

	// genStaticMeshColliders(assets);
	generateMap();

	char_sequence.clips.add(&assets->walking_anim);
	char_sequence.clips.add(&assets->turning_anim);
}

void GameState::genStaticMeshColliders(Assets *assets) {
	int i = 0;
	// for(u32 i = 0; i < static_mesh_reference_count; i++) {
		StaticMeshReference &reference = static_mesh_references[i];
		StaticMeshData *mesh_data = &assets->static_meshes.meshes[reference.index];
		btBvhTriangleMeshShape *physics_shape = (btBvhTriangleMeshShape *)mesh_data->physics_obj;
		btCollisionObject *obj = new btCollisionObject();
		obj->setCollisionShape(physics_shape);
		Vec3 p = Mat4::extractTranslation(reference.transform);
		obj->setWorldTransform(btTransform(btQuaternion(0, 0, 0, 1), btVector3(p.x, p.y, p.z)));
		collision_world->addCollisionObject(obj);
	// }
}

void GameState::createRenderTextures(RenderContext *render_context) {
	hdr_color_render_texture = render_context->createRenderTexture(camera.window_dimensions.x, camera.window_dimensions.y, RenderContext::Format::Vec4);
	blur_render_texture = render_context->createRenderTexture(camera.window_dimensions.x, camera.window_dimensions.y, RenderContext::Format::u8_unorm);
	ao_render_texture = render_context->createRenderTexture(camera.window_dimensions.x, camera.window_dimensions.y, RenderContext::Format::u8_unorm);
	final_composite_texture = render_context->createRenderTexture(camera.window_dimensions.x, camera.window_dimensions.y, RenderContext::Format::u32_unorm);
}

void GameState::onResize(InputManager *input, f32 delta, Platform *platform, PlatformWindow *window, Assets *assets, RenderContext *render_context) {
	camera.calculateWindowBounds(platform, window);
	render_context->destroyRenderTexture(&blur_render_texture);
	render_context->destroyRenderTexture(&hdr_color_render_texture);
	render_context->destroyRenderTexture(&ao_render_texture);
	render_context->destroyRenderTexture(&final_composite_texture);
	createRenderTextures(render_context);
	pn_prepass.resize(render_context, camera.window_dimensions.x, camera.window_dimensions.y);
}

global_variable f32 g_anim_timer = 0.0f;

void GameState::update(InputManager *input, f32 delta, Platform *platform, PlatformWindow *window, Assets *assets, RenderContext *render_context) {
	timer += delta;
	camera.beginFrame(platform, window);
	// Vec2i lock_mouse_pos = Vec2i(camera.window_dimensions.x / 2, camera.window_dimensions.y / 2);
	// Vec2i mouse_delta = input->getMousePosition() - lock_mouse_pos;
	npc_position.z -= delta * 3.0f;
	camera.updatecameraLook(platform, input, delta);
	f32 move_speed = 1.0f;
	f32 max_jump_height = 5.0f;
	f32 time_to_jump_apex = 0.3f;
	f32 gravity = -(2 * max_jump_height) / Math::pow(time_to_jump_apex, 2);
	f32 jump_velocity = Math::abs(gravity) * time_to_jump_apex;

	velocity = Vec3();

	if(input->isKeyDown(Key::LShift)) move_speed *= 2.0f;
	if(input->isKeyDown(Key::W)) velocity += camera.forward(true);
	if(input->isKeyDown(Key::S)) velocity -= camera.forward(true);
	if(input->isKeyDown(Key::D)) velocity += camera.right();
	if(input->isKeyDown(Key::A)) velocity -= camera.right();

	if(Vec3::lengthSquared(velocity) > 0) velocity = Vec3::normalize(velocity) * move_speed;

	if(input->isKeyDown(Key::Space) && char_controller->canJump()) char_controller->jump(btVector3(0, jump_velocity, 0)); 


	velocity.x *= 0.5f;
	velocity.z *= 0.5f;

	char_controller->setWalkDirection(btVector3(velocity.x, velocity.y, velocity.z));
	char_controller->setGravity(btVector3(0, gravity, 0));
	char_controller->updateAction(collision_world, delta);
	btVector3 linear_velocity = char_controller->getLinearVelocity();
	// velocity = Vec3(linear_velocity.x(), linear_velocity.y(), linear_velocity.z());
	//  - camera.forward() * 5.0f
	camera.position = b2V(char_controller->getGhostObject()->getWorldTransform().getOrigin()) + Vec3(0.0f, 3.0f, 0);
	// input->setMousePosition(platform, lock_mouse_pos.x, lock_mouse_pos.y);
	g_anim_timer += delta;
	char_sequence.progress(delta);
	g_anim_timer = Math::wrap(g_anim_timer, 0.0f, assets->walking_anim.duration_seconds);
	assets->skel_dude.skeleton.applyAnimationClip(&assets->walking_anim, g_anim_timer);
}

void GameState::renderGeometry(PlatformWindow *window, RenderContext *render_context, InputManager *input, Assets *assets, Platform *platform,  f32 delta, bool can_override_shader) {
	renderMesh(render_context, &test_cube_mesh, Vec3(0.0f, -1.0f, 0.0f), Vec3(40.0f, 1.0f, 40.0f), Vec4(0.0f, 1.0f, 0.0f, 1.0f), 1.0f, 0.0f);
	renderMesh(render_context, &test_cube_mesh, Vec3(0.0f, 0.0f, 0.0f), Vec3(6.0f, 3.0f, 6.0f), Vec4(1.0f, 0.0f, 0.0f, 1.0f), 1.0f, 0.0f);
	int sphere_count = 10;
	for(int y = 0; y < sphere_count; y++) {
		float metallic = y * (1.0f / (f32)sphere_count);
		for(int x = 0; x < sphere_count; x++) {
			float roughness = x * (1.0f / (f32)sphere_count);
			// renderMesh(render_context, &test_sphere_mesh, Vec3(x * 2.0f, y * 2.0f, 0.0f), Vec3(1.0f), Vec4(1.0f), roughness, metallic);
		}
	}
	

	Vec3 model_position = Vec3(0.0f, 4.0f, 0.0f);
	for(u32 i = 0; i < assets->chr_old.voxel_count; i++) {
		Voxel &voxel = assets->chr_old.voxels[i];
		Vec4 color = Vec4();
		u32 palette_color = assets->chr_old.palette[voxel.color_index];
		f32 d = 1.0f / 255.0f;
		for(int b = 0; b < 4; b++) {
			color.xyzw[b] = (f32)GetByte(b, palette_color) * d;
		}
		// renderMesh(render_context, &test_cube_mesh, model_position + Vec3(voxel.x, voxel.z, voxel.y), Vec3(0.5f), color, 1.0f, 0.0f);
	}

	// for(u32 z = 0; z < MAP_SIZE; z++) {
	// 	for(u32 x = 0; x < MAP_SIZE; x++) {
	// 		u8 y = g_map[z][x];
	// 		Vec3 position = Vec3(x, y, z);
	// 		renderMesh(render_context, &test_cube_mesh, position, Vec3(0.5f), g_color_lookups[y], 1.0f, 0.0f);
	// 	}
	// }

	// renderModel(render_context, &assets->model, Vec3(-5.0f, 0.0f, 0.0f), Vec3(0.01f), Vec4(1.0f), 1.0f, 0.0f);
	// renderModel(render_context, &assets->tree2, Vec3(-10.0f, 0.0f, 0.0f), Vec3(0.1f), Vec4(1.0f), 1.0f, 0.0f);
	// renderModel(render_context, &assets->trees, Vec3(-10.0f, 0.0f, 0.0f), Vec3(0.1f), Vec4(1.0f), 1.0f, 0.0f);
	// for(int i = 0; i < 4; i++)
	// 	for(int j = 0; j < 4; j++)
	// 		renderModel(render_context, &assets->well, Vec3(-10.0f + (j * 2), -0.9f, i * 2), Vec3(1.0f), Vec4(1.0f), 1.0f, 0.0f);

	// renderMesh(render_context, &test_cube_mesh, Vec3(0.0f, -1.0f, 0.0f), Vec3(30.0f, 1.0f, 30.0f), Vec4(1.0f), 1.0f, 0.0f);

	if(can_override_shader)
		render_context->bindShader(&assets->shaders.statics);
	// renderStaticMesh(render_context, &assets->test_static, draw_position, Vec3(1.0f), Vec4(1.0f), 1.0f, 0.0f);

	for(u32 i = 0; i < static_mesh_reference_count; i++) {
		StaticMeshReference &reference = static_mesh_references[i];
		StaticMeshData *data = &assets->static_meshes.meshes[reference.index];
		// renderStaticMesh(render_context, data, reference.transform,  Vec4(1.0f), 1.0f, 0.0f);
	}
	if(can_override_shader)
		render_context->bindShader(&assets->shaders.skinned);
	renderSkinnedMesh(render_context, &assets->skel_dude, npc_position, Vec3(0.025f), Vec4(1.0f), 1.0f, 0.0f);
}

void GameState::render(PlatformWindow *window, RenderContext *render_context, InputManager *input, Assets *assets, Platform *platform,  f32 delta) {
	// for(u32 i = 0; i < assets->skel_dude.skeleton.num_joints; i++) {
	// 	const Joint &joint = assets->skel_dude.skeleton.joints[i];
	// 	Vec3 position = Mat4::extractTranslation(joint.world_mat);
	// 	if(joint.parent_index > -1) {
	// 		const Joint &parent_joint = assets->skel_dude.skeleton.joints[joint.parent_index];
	// 		Vec3 parent_position = Mat4::extractTranslation(parent_joint.world_mat) *  Vec3(0.075f) + Vec3(10.0f, 0.0f, 0.0f);
	// 		DebugRenderQueue::addLine(parent_position, position *  Vec3(0.075f) + Vec3(10.0f, 0.0f, 0.0f), Vec4(0.0f, 1.0f, 0.0f, 1.0f), 0.1f);
	// 	}
	// }
	if(input->isKeyDownOnce(Key::F5)) generateMap();
	pn_prepass.bindForWriting(platform, render_context);
	pn_prepass.clear(render_context, Vec4(0.0f, 0.0f, 0.0f, 0.0f));

	render_context->bindShaderConstant(&per_scene_constants, 0, 0);
	const float blend_factor[4] = { 0.f, 0.f, 0.f, 0.f };
	// render_context->bindBlendState(&alpha_blend, blend_factor, 0xffffffff);
	render_context->bindRasterState(&raster_state);
	render_context->bindSampler(&texture_sampler, 0);
	render_context->bindSampler(&wrap_texture_sampler, 1);
	

	PerSceneConstants scene_constants = {};
	scene_constants.projection = camera.getProjection();
	scene_constants.view = camera.getView();
	for(int i = 0; i < ArrayCount(lights); i++) {
		Light &light = lights[i];
		scene_constants.lights[i] = light.calcShaderLight();
	}
	scene_constants.camera_position = Vec4(camera.position, 1.0f);
	scene_constants.screen_size = Vec4(camera.window_dimensions.x, camera.window_dimensions.y, 0.0f, 0.0f);
	render_context->updateShaderConstant(&per_scene_constants, &scene_constants);
	
	render_context->bindDepthStencilState(&depth_state);
	render_context->bindShader(&assets->shaders.geometry);

	// position normal pre-pass
	// render_context->bindShader(&assets->shaders.geometry_pn);
	// renderGeometry(window, render_context, input, assets, platform, delta, false);

	
	// render_context->bindDepthStencilState(&no_depth_state);
	// render_context->bindRasterState(&no_depth_raster_state);
	// pn_prepass.bindForReading(render_context);
	
	// {
	// 	RenderTexture *rts[] = {&ao_render_texture};
	// 	render_context->bindRenderTextures(platform, rts, 1, 0);
	// 	render_context->clearRenderTexture(&ao_render_texture, Vec4(1.0f).xyzw);
		
	// 	render_context->bindTexture2D(&pn_prepass.normal.texture, 0);
	// 	render_context->bindTexture2D(&pn_prepass.position.texture, 1);
	// 	render_context->bindTexture2D(&ssao_noise_texture, 2);
	// 	render_context->bindShader(&assets->shaders.ao_pass);
	// 	ssao_pass_constant_data.sceen_size = Vec2(camera.window_dimensions.x, camera.window_dimensions.y);
	// 	ssao_pass_constant_data.projection = camera.getProjection();
	// 	render_context->updateShaderConstant(&ao_pass_constants, &ssao_pass_constant_data);
	// 	render_context->bindShaderConstant(&ao_pass_constants, 0, 0);
	// 	renderFullScreenQuad(render_context);

		
	// 	render_context->bindShader(&assets->shaders.blur);
	// 	render_context->updateShaderConstant(&blur_constants, &blur_constants_data);

	// 	rts[0] = &blur_render_texture;
	// 	render_context->bindRenderTextures(platform, rts, 1, 0);
	// 	render_context->bindTexture2D(&ao_render_texture.texture, 0);
	// 	render_context->bindShaderConstant(&blur_constants, 0, 0);
	// 	renderFullScreenQuad(render_context);
	// }

	render_context->bindShaderConstant(&per_scene_constants, 0, 0);
	RenderTexture *hdr_rts[] = {&hdr_color_render_texture};
	render_context->bindRenderTextures(platform, hdr_rts, 1, 0, true);
	render_context->clearRenderTexture(&hdr_color_render_texture, Vec4(0.0f, 0.0f, 0.0f, 1.0f).xyzw);
	render_context->bindTexture2D(&assets->test, 0);
	render_context->bindTexture2D(&assets->irr_map, 1);
	render_context->bindTexture2D(&assets->env_map, 2);
	render_context->bindTexture2D(&blur_render_texture.texture, 3);
	render_context->bindShader(&assets->shaders.geometry);
	renderGeometry(window, render_context, input, assets, platform, delta, true);

	scene_constants.projection = camera.getViewProjection(false);
	render_context->updateShaderConstant(&per_scene_constants, &scene_constants);
	render_context->bindShader(&assets->shaders.skybox);
	render_context->bindTexture2D(&assets->env_map, 0);

	{
		Vec3 vertices[] = {
		/* 0 */	Vec3(-1.0f, 1.0f, -1.0f), // Top Front left
		/* 1 */	Vec3(1.0f, 1.0f, -1.0f), // Top Front right
		/* 2 */	Vec3(-1.0f, -1.0f, -1.0f), // bottom Front left
		/* 3 */	Vec3(1.0f, -1.0f, -1.0f), // bottom Front right

		/* 4 */	Vec3(-1.0f, 1.0f, 1.0f), // Top back left
		/* 5 */	Vec3(1.0f, 1.0f, 1.0f), // Top back right
		/* 6 */	Vec3(-1.0f, -1.0f, 1.0f), // bottom back left
		/* 7*/	Vec3(1.0f, -1.0f, 1.0f), // bottom back right
		};

		u16 indices[] = {
			0,  2, 1,
			2,  3, 1,
			1,  3, 5,
			3,  7, 5,
			5,  7, 4,
			7,  6, 4,
			4,  6, 0,
			6,  2, 0,
			4,  0, 5,
			0,  1, 5,
			2,  6, 3,
			6,  7, 3,
		};

		u32 vertex_count = ArrayCount(vertices);
		u32 index_count = ArrayCount(indices);
		VertexBuffer vb = render_context->createVertexBuffer(vertices, sizeof(Vec3), vertex_count);
		VertexBuffer ib = render_context->createVertexBuffer(indices, sizeof(u16), index_count, RenderContext::BufferType::Index);
		render_context->bindShaderLayout(&skybox_layout);
		render_context->bindVertexBuffer(&vb, 0);
		render_context->bindIndexBuffer(&ib, RenderContext::Format::u16);
		render_context->sendDrawIndexed(RenderContext::Topology::TriangleList, index_count, 0, 0);
		render_context->destroyVertexBuffer(&vb);
		render_context->destroyVertexBuffer(&ib);
	}
	

	render_context->bindDefaultTextures(true);
	render_context->bindDepthStencilState(&no_depth_state);
	render_context->bindTexture2D(&hdr_color_render_texture.texture, 0);

	// Final blit
	render_context->bindShader(&assets->shaders.hdr_blit);

	{
		hdr_blit_constants_data.exposure = 1.0f;
		render_context->updateShaderConstant(&hdr_blit_constants, &hdr_blit_constants_data);
		render_context->bindShaderConstant(&hdr_blit_constants, -1, 0);
		renderFullScreenQuad(render_context);;
	}

	render_context->bindDepthStencilState(&depth_state);
	collision_world->debugDrawWorld();
}

void GameState::renderFullScreenQuad(RenderContext *render_context) {
	TexturedVertex vertices[] = {
		{ Vec3(-1.0f, 1.0f, 0.0f), Vec2(0.0f, 0.0f) },
		{ Vec3(1.0f, 1.0f, 0.0f), Vec2(1.0f, 0.0f) },
		{ Vec3(-1.0f, -1.0f, 0.0f), Vec2(0.0f, 1.0f) },
		{ Vec3(1.0f, -1.0f, 0.0f), Vec2(1.0f, 1.0f) },
	};

	u32 vertex_count = ArrayCount(vertices);
	
	
	VertexBuffer vb = render_context->createVertexBuffer(vertices, sizeof(TexturedVertex), vertex_count);
	
	
	render_context->bindShaderLayout(&textired_geom_layout);
	render_context->bindVertexBuffer(&vb, 0);
	render_context->sendDraw(RenderContext::Topology::TriangleStrip, vertex_count);
	render_context->destroyVertexBuffer(&vb);
}

void GameState::renderModel(RenderContext *render_context, ModelWrapper *model, Vec3 position, Vec3 scale, Vec4 color, float roughness, float metallic) {
	for(u32 i = 0; i < model->mesh_count; i++) {
		renderMesh(render_context, &model->meshes[i], position, scale, color * model->meshes[i].color, roughness, metallic);
	}
}

void GameState::renderMesh(RenderContext *render_context, MeshWrapper *mesh, Vec3 position, Vec3 scale, Vec4 color, float roughness, float metallic) {
	render_context->bindShaderLayout(&vertex_layout);
	render_context->bindVertexBuffer(&mesh->vertex_buffer, 0);
	render_context->bindIndexBuffer(&mesh->index_buffer, RenderContext::Format::u16);
	PerObjectConstants per_object = {};
	per_object.model = (Mat4::translate(position) * Mat4::scale(scale)) * mesh->local_matrix;
	per_object.color = color;
	per_object.roughness = roughness;
	per_object.metallic = metallic;
	
	render_context->updateShaderConstant(&per_object_constants, &per_object);
	render_context->bindShaderConstant(&per_object_constants, 1, 1);
	render_context->sendDrawIndexed(mesh->topology_format, mesh->index_count, 0, 0);
}

void GameState::renderSkinnedMesh(RenderContext *render_context, SkeletalMesh *mesh, Vec3 position, Vec3 scale, Vec4 color, float roughness, float metallic) {
	render_context->bindShaderLayout(&skin_vertex_layout);
	render_context->bindVertexBuffer(&mesh->vertex_buffer, 0);
	render_context->bindIndexBuffer(&mesh->index_buffer, RenderContext::Format::u16);
	SkinnedMeshConstants *per_object = new SkinnedMeshConstants();
	per_object->model = (Mat4::translate(position) * Mat4::scale(scale));
	per_object->color = color;
	per_object->roughness = roughness;
	per_object->metallic = metallic;
	
	for(u32 joint_index = 2; joint_index < mesh->skeleton.num_joints; joint_index++) {
		Joint &joint = mesh->skeleton.joints[joint_index];
		Mat4 bind_pose = joint.world_mat *  joint.inv_bind_mat;
		per_object->skining_matrices[joint_index] = bind_pose;
	}
	
	render_context->updateShaderConstant(&skinned_mesh_constants, per_object);
	render_context->bindShaderConstant(&skinned_mesh_constants, 1, 1);
	render_context->sendDrawIndexed(RenderContext::Topology::TriangleList, mesh->index_count, 0, 0);

	delete per_object;
}

void GameState::renderStaticMesh(RenderContext *render_context, StaticMeshData *mesh, Mat4 transform, Vec4 color, float roughness, float metallic) {
	render_context->bindShaderLayout(&static_vertex_layout);
	render_context->bindVertexBuffer(&mesh->vertex_buffer, 0);
	render_context->bindIndexBuffer(&mesh->index_buffer, RenderContext::Format::u16);
	PerObjectConstants per_object = {};
	per_object.model = transform;
	per_object.color = color;
	per_object.roughness = roughness;
	per_object.metallic = metallic;
	
	render_context->updateShaderConstant(&per_object_constants, &per_object);
	render_context->bindShaderConstant(&per_object_constants, 1, 1);
	render_context->sendDrawIndexed(RenderContext::Topology::TriangleList, mesh->index_count, 0, 0);
}

#ifdef PP_EDITOR
#define EDITOR_ONLY(stuff) stuff
#include <game/editor.h>
#else
#define EDITOR_ONLY(stuff)
#endif // PP_EDITOR

GAME_INIT(gameInit) {
	g_frame_stack = (StackAllocator *)mem_store->frame_memory.memory;
	u64 new_frame_size = mem_store->frame_memory.size - sizeof(StackAllocator);
	g_frame_stack->initialize((void *)((u8 *)mem_store->frame_memory.memory + sizeof(StackAllocator)), new_frame_size);
	
	u64 memory_size = sizeof(GameState);
	EDITOR_ONLY(memory_size += sizeof(EditorState);)
	
	Assert(memory_size <= mem_store->game_memory.size);	
	GameState *game = (GameState *)mem_store->game_memory.memory;
	
	game->init(platform, window, render_context, assets, audio);
	
	EDITOR_ONLY(
		EditorState *editor = (EditorState *)((u8 *)mem_store->game_memory.memory + sizeof(GameState));
		editor->init(render_context, assets);
	)
}

GAME_UPDATE(gameUpdate) {
	GameState *game = (GameState *)mem_store->game_memory.memory;
	g_frame_stack = (StackAllocator *)mem_store->frame_memory.memory;
	g_frame_stack->clear();
	
	EDITOR_ONLY(
	EditorState *editor = (EditorState *)((u8 *)mem_store->game_memory.memory + sizeof(GameState));
	if(input->isKeyDownOnce(Key::F4)) {
		editor->enabled = !editor->enabled;
	}
	
	if(editor->enabled) {
		if(!editor->was_enabled) editor->onShow(game, platform);
		editor->update(platform, input, delta, window, assets, render_context);
	} else {
		if(editor->was_enabled) editor->onHide(platform);
		) 
	
		game->update(input, delta, platform, window, assets, render_context);
		EDITOR_ONLY(
	}
	
	editor->was_enabled = editor->enabled;
	)
}

GAME_RESIZE(gameResize) {
	GameState *game = (GameState *)mem_store->game_memory.memory;
	g_frame_stack = (StackAllocator *)mem_store->frame_memory.memory;
	game->onResize(input, delta, platform, window, assets, render_context);
}

GAME_RENDER(gameRender) {
	GameState *game = (GameState *)mem_store->game_memory.memory;
	game->render(window, render_context, input, assets, platform, delta);
	
	EDITOR_ONLY(
		EditorState *editor = (EditorState *)((u8 *)mem_store->game_memory.memory + sizeof(GameState));
		if(editor->enabled) editor->render(platform, window, render_context, input, assets, delta);
	)

	DebugRenderQueue::flushRender(render_context, &assets->shaders.editor, game->camera.getViewProjection());
	
	game->camera.endFrame();
}