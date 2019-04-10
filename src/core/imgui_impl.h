#ifndef IMGUI_IMPL_H
#define IMGUI_IMPL_H

struct RenderContext;
struct Platform;
struct PlatformWindow;
struct Shader;

struct ImGuiImpl {
	static void init(Platform *platform, PlatformWindow *window, Shader *shader);
	static void shutdown();
	static void render(RenderContext *ctx);
	static void newFrame(Platform *platform, PlatformWindow *window, RenderContext *ctx, float delta);
	
};

#endif // IMGUI_IMPL_H