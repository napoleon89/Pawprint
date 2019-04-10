#ifndef EDITOR_H
#define EDITOR_H

#include <vector>
#include <engine/math.h>
#include <engine/static_mesh.h>
#include <imgui/ImGuizmo.h>
#include <game/camera.h>

struct InputManager;
struct RenderContext;
struct Assets;
struct GameState;

struct GameState;

struct EditorState {
	bool enabled;
	bool was_enabled;
	Camera start_camera;
	GameState *game;
	std::vector<StaticMeshReference> static_mesh_references;
	int selected_item;
	f32 timer;
	s32 selected_static;
	Vec3 hit_draw_position;
	bool paint_mode = false;
	ImGuizmo::OPERATION operation;

	
	void init(RenderContext *render_context, Assets *assets);
	void onShow(GameState *g, Platform *platform);
	void onHide(Platform *platform);
	void update(Platform *platform, InputManager *input, f32 delta, PlatformWindow *window, Assets *game_assets, RenderContext *render_context);
	void addMesh(s32 index, Vec3 position);
	void render(Platform *platform, PlatformWindow *window, RenderContext *render_context, InputManager *input, Assets *assets, f32 delta);
};

#endif // EDITOR_H