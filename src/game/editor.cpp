#include <game/editor.h>
#include <engine/input.h>
#include <core/render_context.h>
#include <game/assets.h>
#include <game/game_state.h>
#include <imgui/imgui.h>
#include <engine/debug_renderer.h>

void EditorState::init(RenderContext *render_context, Assets *assets) {
	enabled = false;
	was_enabled = false;
	operation = ImGuizmo::OPERATION::TRANSLATE;
	selected_static = -1;
}

void EditorState::onShow(GameState *g, Platform *platform) {
	game = g;
	start_camera = game->camera;
	static_mesh_references.clear();
	if(game->static_mesh_reference_count > 0) {
		static_mesh_references.reserve(game->static_mesh_reference_count);
		for(u32 i = 0; i < game->static_mesh_reference_count; i++) {
			static_mesh_references.push_back(game->static_mesh_references[i]);
		}
		platform->free(game->static_mesh_references);
	}
	
}

void EditorState::onHide(Platform *platform) {
	game->camera = start_camera;
	u32 count = (u32)static_mesh_references.size();
	if(count > 0) {
		game->static_mesh_references = (StaticMeshReference *)platform->alloc(sizeof(StaticMeshReference) * count);
		memcpy(game->static_mesh_references, &static_mesh_references[0], sizeof(StaticMeshReference) * count);
		game->static_mesh_reference_count = count;
	}
}

void EditorState::update(Platform *platform, InputManager *input, f32 delta, PlatformWindow *window, Assets *game_assets, RenderContext *render_context) {
	timer += delta;
	game->camera.beginFrame(platform, window);
	game->camera.updatecameraLook(platform, input, delta);
	ImGuizmo::BeginFrame();
	ImGuizmo::SetRect(0, 0, game->camera.window_dimensions.x, game->camera.window_dimensions.y);
	f32 move_speed = 4.0f;
	if(input->isKeyDown(Key::LShift)) move_speed *= 2.0f;
	if(input->isKeyDown(Key::W)) game->camera.position += game->camera.forward() * move_speed * delta;
	if(input->isKeyDown(Key::S)) game->camera.position -= game->camera.forward() * move_speed * delta;
	if(input->isKeyDown(Key::D)) game->camera.position += game->camera.right() * move_speed * delta;
	if(input->isKeyDown(Key::A)) game->camera.position -= game->camera.right() * move_speed * delta;

	// if(ImGui::BeginMainMenuBar()) {
	// 	ImGui::EndMainMenuBar();	
	// } 

	
	// ImGui::DockSpaceOverViewport();

	f32 e_width = 250.0f;
	f32 margin = 5.0f;
	// ImGui::SetNextWindowPos(ImVec2(game->camera.window_dimensions.x - (e_width + margin), 25.0f));
	ImGui::Begin("Properties", 0, ImVec2(e_width, game->camera.window_dimensions.y - margin * 2.0f)); {
		
		for(int i = 0; i < 4; i++) {
			Light &light = game->lights[i];
			Vec3 color = light.color.xyz / 300.0f;
			ImGui::ColorEdit3(formatString("Light %d", i), color.xyz);
			light.color.xyz = color * 300.0f;

		}

		ImGui::Checkbox("Wioreframe", &Globals::wireframe);
		
		ImGui::DragFloat("radius", &game->ssao_pass_constant_data.kernel_radius, 0.1f);
		ImGui::DragFloat("bias", &game->ssao_pass_constant_data.kernel_bias, 0.1f);
		ImGui::DragInt("size", &game->ssao_pass_constant_data.kernel_size);
		ImGui::DragInt("blur", &game->blur_constants_data.blur_amount);

		if(ImGui::Button("Generated colliders")) {
			game->genStaticMeshColliders(game_assets);
		}

		if(ImGui::Button("Add mesh")) {
			addMesh(selected_item, game->camera.position + game->camera.forward() * 5.0f);
		}
		ImGui::PushID("Static meshes");
		ImGui::ListBox("", &selected_item, game_assets->static_meshes.names, game_assets->static_meshes.count, game_assets->static_meshes.count);
		ImGui::PopID();
		
		ImGui::End();
	}
	
	// ImGui::SetNextWindowBgAlpha(0.0f);
	// ImGui::Begin("Game view"); {
	// 	// ImGui::Image((ImTextureID)&game->hdr_color_render_texture.texture, ImVec2(800, 450));
	// 	ImVec2 window_pos = ImGui::GetWindowPos();
	// 	ImVec2 window_size = ImGui::GetWindowSize();
	// 	// render_context->setViewport(window_pos.x, window_pos.y, window_size.x, window_size.y, 0.0f, 1.0f);
	// 	ImGui::End();
	// }


	Vec3 ray_dir = game->camera.screenToRay(input->getMousePosition());
	Mat4 camera_view = Mat4::transpose(game->camera.getView());
	Mat4 camera_proj = Mat4::transpose(game->camera.getProjection());
	Vec3 camera_forward = game->camera.forward();
	f32 distance = FLT_MAX;
	Vec3 position;
	bool hit = false;
	u32 hit_index = 0;

	for(u32 i = 0; i < (u32)static_mesh_references.size(); i++) {
		StaticMeshReference &reference = static_mesh_references[i];
		
		StaticMeshData &mesh = game_assets->static_meshes.meshes[reference.index];
		Vec3 size = (mesh.max - mesh.min) * Mat4::extractScale(reference.transform);
		

		// drawAABBHandle(center - size * 0.5f, center + size * 0.5f, Vec4(1.0f, 0.0f, 0.0f, 1.0f));
		
	}

	paint_mode = input->isKeyDown(Key::LCtrl);
	if(input->isMouseButtonDownOnce(MouseButton::Left) && !ImGuizmo::IsOver()) {
		if(paint_mode) {
			addMesh(selected_item, hit_draw_position);
		} else if(hit) {
			selected_static = hit_index;
		} else {
			selected_static = -1;
		}
	}

	if(hit) {
		// drawAABBHandle(position - Vec3(0.1f), position + Vec3(0.1f));
	}
	
	if(input->isKeyDownOnce(Key::F1)) operation = ImGuizmo::OPERATION::TRANSLATE;
	if(input->isKeyDownOnce(Key::F2)) operation = ImGuizmo::OPERATION::ROTATE;
	if(input->isKeyDownOnce(Key::F3)) operation = ImGuizmo::OPERATION::SCALE;

	if(selected_static != -1) {
		StaticMeshReference &reference = static_mesh_references[selected_static];
		StaticMeshData &mesh = game_assets->static_meshes.meshes[reference.index];
		Mat4 world = Mat4::transpose(reference.transform);
		ImGuizmo::Manipulate(camera_view.data1d, camera_proj.data1d, operation, ImGuizmo::MODE::WORLD, world.data1d);
		world = Mat4::transpose(world);
		reference.transform = world;
	}

	// for(u32 i = 0; i < game_assets->skel_dude.num_joints; i++) {
	// 	Joint &joint = game_assets->skel_dude.joints[i];
	// 	Mat4 world = Mat4::transpose(joint.local_mat);
	// 	ImGuizmo::Manipulate(camera_view.data1d, camera_proj.data1d, operation, ImGuizmo::MODE::WORLD, world.data1d);
	// 	world = Mat4::transpose(world);
	// 	joint.local_mat = world;
	// }

	// drawLineHandle(Vec3(0.0f, 0.0f, 0.0f), Vec3(0.0f, 1.0f, 5.0f), Vec4(0.0f, 1.0f, 0.0f, 1.0f));
	// drawLineHandle(Vec3(0.0f, 0.0f, -5.0f), Vec3(1.0f, 0.0f, -3.0f));
	{
		// Vec3 draw_position = camera.position + ray_dir
		
	}
	

	DebugRenderQueue::addLine(Vec3(0.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f));
	DebugRenderQueue::addLine(Vec3(0.0f, 0.0f, 1.0f), Vec3(1.0f, 0.0f, 1.0f));
	DebugRenderQueue::addLine(Vec3(1.0f, 0.0f, 1.0f), Vec3(1.0f, 0.0f, 0.0f));
	DebugRenderQueue::addLine(Vec3(1.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, 0.0f));

	DebugRenderQueue::addLine(Vec3(0.0f, 1.0f, 0.0f), Vec3(0.0f, 0.0f, 0.0f));
	DebugRenderQueue::addLine(Vec3(0.0f, 1.0f, 1.0f), Vec3(0.0f, 0.0f, 1.0f));
	DebugRenderQueue::addLine(Vec3(1.0f, 1.0f, 1.0f), Vec3(1.0f, 0.0f, 1.0f));
	DebugRenderQueue::addLine(Vec3(1.0f, 1.0f, 0.0f), Vec3(1.0f, 0.0f, 0.0f));

	if(static_mesh_references.size() > 0) {
		game->static_mesh_references = &static_mesh_references[0];
		game->static_mesh_reference_count = (u32)static_mesh_references.size();
	}
}

void EditorState::addMesh(s32 index, Vec3 position) {
	StaticMeshReference reference = {};
	reference.index = index;
	reference.transform = Mat4::translate(position);
	static_mesh_references.push_back(reference);
}

void EditorState::render(Platform *platform, PlatformWindow *window, RenderContext *render_context, InputManager *input, Assets *assets, f32 delta) {

	if(selected_item != -1 && paint_mode) {
		StaticMeshData &mesh = assets->static_meshes.meshes[selected_item];
		game->renderStaticMesh(render_context, &mesh, Mat4::translate(hit_draw_position), Vec4(1.0f), 1.0f, 0.0f);
	}
	

	// {
	// 	Vec3 vertices[] = {
	// 	/* 0 */	Vec3(-0.5f, 0.5f, -0.5f), // Top Front left
	// 	/* 1 */	Vec3(0.5f, 0.5f, -0.5f), // Top Front right
	// 	/* 2 */	Vec3(-0.5f, -0.5f, -0.5f), // bottom Front left
	// 	/* 3 */	Vec3(0.5f, -0.5f, -0.5f), // bottom Front right

	// 	/* 4 */	Vec3(-0.5f, 0.5f, 0.5f), // Top back left
	// 	/* 5 */	Vec3(0.5f, 0.5f, 0.5f), // Top back right
	// 	/* 6 */	Vec3(-0.5f, -0.5f, 0.5f), // bottom back left
	// 	/* 7*/	Vec3(0.5f, -0.5f, 0.5f), // bottom back right
	// 	};

	// 	u16 indices[] = {
	// 		0,  2, 1,
	// 		2,  3, 1,
	// 		1,  3, 5,
	// 		3,  7, 5,
	// 		5,  7, 4,
	// 		7,  6, 4,
	// 		4,  6, 0,
	// 		6,  2, 0,
	// 		4,  0, 5,
	// 		0,  1, 5,
	// 		2,  6, 3,
	// 		6,  7, 3,
	// 	};

		
	// 	render_context->bindShaderConstant(&per_object_constant, 1, 1);

	// 	u32 vertex_count = ArrayCount(vertices);
	// 	u32 index_count = ArrayCount(indices);
	// 	VertexBuffer vb = render_context->createVertexBuffer(vertices, sizeof(Vec3), vertex_count);
	// 	VertexBuffer ib = render_context->createVertexBuffer(indices, sizeof(u16), index_count, RenderContext::BufferType::Index);
	// 	render_context->bindVertexBuffer(&vb, 0);
	// 	render_context->bindIndexBuffer(&ib, RenderContext::Format::u16);

	// 	for(size_t i = 0; i < handles_aabbs.size(); i++) {
	// 		EditorAABBHandle &handle = handles_aabbs[i];
	// 		EditorPerObject per_object = {};
	// 		per_object.color = handle.color;
	// 		Vec3 size = handle.max - handle.min;
	// 		Vec3 center = handle.min + (size * 0.5f);
	// 		per_object.model = Mat4::translate(center) * Mat4::scale(size);
	// 		render_context->updateShaderConstant(&per_object_constant, &per_object);
	// 		render_context->sendDrawIndexed(RenderContext::Topology::TriangleList, index_count, 0, 0);
	// 	}

		
	// 	render_context->destroyVertexBuffer(&vb);
	// 	render_context->destroyVertexBuffer(&ib);
	// }
}
