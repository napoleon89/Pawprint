#include <stdio.h>
#include <string.h>
#include <engine/std.h>
#include <engine/timer.h>
#include <stdlib.h>
#include <engine/math.h>
#include <string>
#include <game/game.h>
#include <core/platform.h>
#include <imgui/imgui.h>
#include <imgui/ImGuizmo.h>
#include <engine/input.h>
#include <core/render_context.h>
#include <engine/audio.h>
#include <game/assets.h>
#include <core/imgui_impl.h>
#include <game/memory.h>
#include <engine/audio.h>
#include <main/reflection.h>
#include <engine/collections.h>
#include <game/world.h>

struct GameCode {
	void *game_code_dll;
	GameInitFunc *init;
	GameUpdateFunc *update;
	GameRenderFunc *render;
	GameResizeFunc *resize;
	FileTime last_write_time;
	bool is_valid;
};

internal_func bool loadGameCode(Platform *platform, GameCode *game_code, const char *source_dll_name, const char *temp_dll_name, const char *source_pdb_name, const char *temp_pdb_name) {
	bool copied = platform->copyFile((char *)source_dll_name, (char *)temp_dll_name);
	if(copied) copied = platform->copyFile((char *)source_pdb_name, (char *)temp_pdb_name);
	if(copied) {
		game_code->last_write_time = platform->getLastWriteTime((char *)source_dll_name);
		game_code->game_code_dll = platform->loadLibrary(temp_dll_name);
		
		if(game_code->game_code_dll) {
			game_code->init = (GameInitFunc *)platform->loadFunction(game_code->game_code_dll, "gameInit");
			game_code->update = (GameUpdateFunc *)platform->loadFunction(game_code->game_code_dll, "gameUpdate");
			game_code->render = (GameRenderFunc *)platform->loadFunction(game_code->game_code_dll, "gameRender");
			game_code->resize = (GameResizeFunc *)platform->loadFunction(game_code->game_code_dll, "gameResize");
			
			game_code->is_valid = (game_code->init != 0 && game_code->update != 0 && game_code->render != 0 && game_code->resize != 0);
		}
	}

	if(!game_code->is_valid) {
		game_code->init = gameInitStub;
		game_code->update = gameUpdateStub;
		game_code->render = gameRenderStub;	
		game_code->resize = gameResizeStub;
	}
	
	return copied && game_code->is_valid;
}

internal_func void unloadGameCode(Platform *platform, GameCode *game_code) {
	if(game_code->game_code_dll) {
		platform->unloadLibrary(game_code->game_code_dll);
		game_code->game_code_dll = 0;
	}
	
	game_code->is_valid = false;
	game_code->init = gameInitStub;
	game_code->update = gameUpdateStub;
	game_code->render = gameRenderStub;	
	game_code->resize = gameResizeStub;
}

static void ecsTest() {
	// EntityComponentDatabase database;
	// EntityHandle player = database.createEntity();
	// database.addOrUpdateTransformComponent(player, {Vec3(1.0f, 0.0f, 5.0f), Quat(), Vec3(1.0f)});
	// database.addOrUpdateShapeComponent(player, {Vec3(), 3.0f, Vec4(1.0f, 0.0f, 0.0f, 1.0f)});

	// database.addOrUpdateTransformComponent(database.createEntity(), {Vec3(10.0f, 2.0f, 50.0f), Quat(), Vec3(1.0f)});
	// database.addOrUpdateTransformComponent(database.createEntity(), {Vec3(0.0f, 1.0f, -40.0f), Quat(), Vec3(1.0f)});

	// EntityHandle npc = database.createEntity();
	// database.addOrUpdateTransformComponent(npc, {Vec3(10.0f, 100.0f, 5.0f), Quat(), Vec3(1.0f)});
	// database.addOrUpdateShapeComponent(npc, {Vec3(), 1.0f, Vec4(1.0f, 1.0f, 0.0f, 1.0f)});

	// printf("ecs stuff\n");
}

#include <engine/shaders.h>
#pragma warning(disable:4706)

static const s32 g_multiply_de_bruijn_bit_position_2[32] = {
	0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8, 
	31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
};

inline s32 indexFromComponent(EntityArchetype component) {
	u32 v = (u32)component;
	return g_multiply_de_bruijn_bit_position_2[(u32)(v * 0x077CB531U) >> 27];
}

int main(int arg_count, char *args[]) {

	ShaderDB shader_db = {};

	// DynamicArray<Vec2> temp_positions;
	// for(int i = 0; i < 32; i++) {
	// 	temp_positions.add(Vec2(i, i * 2.0f));
	// 	const Vec2 &position = temp_positions.getRefConst(i);
	// 	printf("%.3f, %.3f\n", position.x, position.y);
	// }

	EntityArchetype entity_type = EntityArchetype::Transform | EntityArchetype::Temp | EntityArchetype::Temp2;
	// u32 entity_type = 0;
	int count = 0;
	// while(entity_type != EntityArchetype::None) {
	// 	count++;
		
	// 	u32 entity_walker = entity_type & (entity_type-1);
	// 	u32 comp_raw = entity_type - entity_walker;
	// 	EntityArchetype comp = (EntityArchetype)(comp_raw);
	// 	s32 index = indexFromComponent(comp);
	// 	entity_type = entity_walker;
	// }

	printf("Found %d components\n", count);

	// ecsTest();

	Platform platform = {};
	if(!platform.init()) {
		platform.error("Couldn't init platform");
	}
	
	std::string base_path = std::string(platform.getExePath());
	std::string game_dll_name = base_path + "game.dll";
	std::string temp_game_dll_name = base_path + "game_temp.dll";
	std::string game_pdb_name = base_path + "game.pdb";
	std::string temp_game_pdb_name = base_path + "game_temp.pdb";
	
	u32 window_width = 1600;
	u32 window_height = 900;
	
	PlatformWindow window = platform.createWindow("Pawprint Engine", window_width, window_height, true, true);

	if(window.handle == 0) {
		platform.error("Couldn't create window");
	}
	s32 refresh_rate = 60;
	RenderContext render_context;
	render_context.init(window_width, window_height, refresh_rate, &window);

	AudioEngine audio_engine = {};
	audio_engine.init();
	
	Timer frame_timer = Timer(&platform);
	bool running = true;
	
	MemoryStore mem_store = {};
	mem_store.game_memory = {0, Megabytes(8)};
	mem_store.asset_memory = {0, Megabytes(8)};
	mem_store.frame_memory = {0, Megabytes(2)};
	
	u64 total_memory_size = mem_store.calcTotalSize();
	mem_store.memory = (void *)platform.alloc(total_memory_size);
	memset(mem_store.memory, 0, total_memory_size); // NOTE(nathan): this might need removing for performance??
	u8 *byte_walker = (u8 *)mem_store.memory;
	for(int i = 0; i < MEMORY_STORE_COUNT; i++) {
		mem_store.blocks[i].memory = (void *)byte_walker;
		byte_walker += mem_store.blocks[i].size;
	}
	
	
	f32 target_seconds_per_frame = 1.0f / (f32)refresh_rate;
	f32 delta = target_seconds_per_frame;

	Assets *game_assets = (Assets *)mem_store.asset_memory.memory;

	{
		FileData file_data = platform.readEntireFile("data/shaders/shaders.map");
		shader_db.loadFromMap((u8 *)file_data.contents, file_data.size);
	}

	game_assets->shaders.geometry = loadShaderFromFile(&platform, &render_context, "geometry");
	game_assets->shaders.imgui = loadShaderFromFile(&platform, &render_context, "imgui");
	game_assets->shaders.skybox = loadShaderFromFile(&platform, &render_context, "skybox");
	game_assets->shaders.hdr_blit = loadShaderFromFile(&platform, &render_context, "hdr_blit");
	// game_assets->shaders.geometry_pn = loadShaderFromFile(&platform, &render_context, "geometry_pn");
	game_assets->shaders.ao_pass = loadShaderFromFile(&platform, &render_context, "ao_pass");
	game_assets->shaders.blur = loadShaderFromFile(&platform, &render_context, "blur");
	game_assets->shaders.statics = loadShaderFromFile(&platform, &render_context, "statics");
	game_assets->shaders.editor = loadShaderFromFile(&platform, &render_context, "editor");
	game_assets->shaders.skinned = loadShaderFromFile(&platform, &render_context, "skinned");
	

	for(u32 i = 0; i < shader_db.shader_keys.count; i++) {
		const ShaderDB::ShaderPassKey &key = shader_db.shader_keys.getRefConst(i);
		const std::string &filename = shader_db.shader_filenames.getRefConst(i);
		printf("%lld - %s\n", key, filename.c_str());
	}

	// game_assets->skybox = loadTextureCubeFromFolder(&platform, &render_context, "sky");
	// game_assets->test = loadTextureFromFile(&platform, "test", &render_context);
	game_assets->chr_old = loadVoxFromFile(&platform, "data/vox/grass.vox");

	u8 *test_image_data = (u8 *)platform.alloc(4);
	for(int i = 0; i < 4; i++) test_image_data[i] = 0xff;
	
	game_assets->test = loadTexture(test_image_data, 3, &render_context);
	game_assets->env_map = loadTextureFromFile(&platform, "skyboxes/apt", &render_context, "hdr", RenderContext::Format::Vec4, 4);
	game_assets->irr_map = loadTextureFromFile(&platform, "skyboxes/apt_ibl", &render_context, "hdr", RenderContext::Format::Vec4, 4);
	
	ImGui::CreateContext();
	ImGuiImpl::init(&platform, &window, &game_assets->shaders.imgui);
	
	ImGuiIO& io = ImGui::GetIO();
	ImGuiStyle &style = ImGui::GetStyle();
	style.WindowRounding = 0.0f;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	
	DirectoryContentsResult dir_contents = platform.getDirectoryContents("data/meshes/static/Modular Village/*.obj");
	game_assets->static_meshes.count = dir_contents.count;
	game_assets->static_meshes.meshes = (StaticMeshData *)platform.alloc(sizeof(StaticMeshData) * dir_contents.count);
	game_assets->static_meshes.names = dir_contents.contents;
	// for(u32 i = 0; i < dir_contents.count; i++) {
	// 	char *name = dir_contents.contents[i];
	// 	game_assets->static_meshes.meshes[i] = loadStaticMeshFromFile(&platform, &render_context, formatString("data/meshes/static/Modular Village/%s", name));
	// }

	{
		
		game_assets->test_static = loadStaticMeshFromFile(&platform, &render_context, "data/meshes/static/Modular Terrain/Hilly_Prop_Camp_Lean_To.obj");
	}
	// game_assets->skel_dude = loadSkeletonFromFile(&platform, "data/meshes/SK_Mannequin.fbx");
	game_assets->skel_dude = loadSkeletalMeshFromFile(&platform, &render_context, "data/meshes/dude.fbx");
	game_assets->walking_anim = loadClipFromFile(&platform, &game_assets->skel_dude.skeleton, "data/anims/walking.fbx");
	game_assets->turning_anim = loadClipFromFile(&platform, &game_assets->skel_dude.skeleton, "data/anims/right_turn_90.fbx");
	// loadSkeletonFromFile(&platform, "data/anims/ThirdPersonWalk.fbx");


	GameCode game_code = {};
	loadGameCode(&platform, &game_code, game_dll_name.c_str(), temp_game_dll_name.c_str(), game_pdb_name.c_str(), temp_game_pdb_name.c_str());
	game_code.init(&platform, &window,  &mem_store, &render_context, game_assets, &audio_engine);
	
	InputManager input(&window);
	while(running) {
		frame_timer.start(&platform);
		
		bool requested_to_quit = false;
		bool resized = false;
		platform.processEvents(&window, requested_to_quit, resized);
		running = !requested_to_quit;
		
		if(resized) {
			u32 width, height;
			platform.getWindowSize(&window, width, height);
			render_context.setViewport(0, 0, (s32)width, (s32)height, 0.0f, 1.0f);
			render_context.resizeBuffer((s32)width, (s32)height);
			game_code.resize(&platform, &mem_store, &input, delta, &window, game_assets, &render_context);
		}
		
		input.processKeys(&platform);
		ImGuiImpl::newFrame(&platform, &window, &render_context, delta);
		
		u32 width, height;
		platform.getWindowSize(&window, width, height);
		render_context.setViewport(0, 0, (s32)width, (s32)height, 0.0f, 1.0f);

		FileTime new_dll_write_time = platform.getLastWriteTime((char *)game_dll_name.c_str());
		if(platform.compareFileTime(&new_dll_write_time, &game_code.last_write_time) > 0) {
			// unloadGameCode(&platform, &game_code);
			// platform.sleepMS(1000);
			
			// if(loadGameCode(&platform, &game_code, game_dll_name.c_str(), temp_game_dll_name.c_str(), game_pdb_name.c_str(), temp_game_pdb_name.c_str())) {
			// 	printf("Reloading game code\n");
			// }
		}
		
		
		if(input.isKeyDownOnce(Key::Escape)) {
			running = false;
		}
		
		f32 debug_window_height = 0.0f;
		ImGui::SetNextWindowPos(ImVec2(20, 20));
		ImGui::Begin("Debug Stuff", 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
				ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize); {
			ImGui::Text("Frame time: %.3f", delta * 1000.0f);
			debug_window_height = ImGui::GetWindowHeight();
			ImGui::End();
		}
		
		if(input.isKeyDownOnce(Key::F11)) {
			platform.setWindowFullscreen(&window, platform.isWindowFullscreen(&window));
		}
		
		game_code.update(&platform, &mem_store, &input, delta, &window, game_assets, &render_context);
		f32 r = 51.0f / 255.0f;
		
		
		
		render_context.bindDefaultTextures(true);
		render_context.clear(Vec4(0.09f,0.04f,0.10f, 1.0f).xyzw);
		
		game_code.render(&platform, &mem_store, &window, &render_context, &input, game_assets, delta);
		
		ImGui::Render();
		ImGuiImpl::render(&render_context);
		
		render_context.present();
		
		
		
		u64 work_counter = frame_timer.getWallClock(&platform);
		f32 work_seconds_elapsed = frame_timer.getSecondsElapsed(&platform);
		
		f32 seconds_elapsed_for_frame = work_seconds_elapsed;
		if(seconds_elapsed_for_frame < target_seconds_per_frame)
		{
			u32 sleep_ms = (u32)((1000.0f * (target_seconds_per_frame - seconds_elapsed_for_frame)) - 1);
			if(sleep_ms > 0)
				platform.sleepMS(sleep_ms);
			while(seconds_elapsed_for_frame < target_seconds_per_frame)
			{
				seconds_elapsed_for_frame = frame_timer.getSecondsElapsed(&platform);
			}
		}
		else
		{
			// missed frame
			printf("Missed frame\n");
		}
		u64 end_counter = frame_timer.getWallClock(&platform);
		delta = frame_timer.getSecondsElapsed(&platform);
		input.endFrame();
	}
	
	audio_engine.uninit();
	unloadGameCode(&platform, &game_code);
	ImGuiImpl::shutdown();
	ImGui::DestroyContext();
	render_context.uninit();
	platform.destroyWindow(&window);
	
	
	platform.uninit();
	return 0;
}