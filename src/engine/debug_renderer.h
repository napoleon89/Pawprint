#ifndef DEBUG_RENDERER_H
#define DEBUG_RENDERER_H

#include <engine/math.h>

struct RenderContext;
struct Shader;

struct DebugRenderQueue {
	static void init(RenderContext *render_context, Shader *debug_shader);
	static void addLine(const Vec3 &start, const Vec3 &end, const Vec4 &color = Vec4(1.0f), f32 thickness = 0.05f);
	static void addAABB(const Vec3 &min, const Vec3 &max, const Vec4 &color = Vec4(1.0f), f32 thickness = 0.05f);
	static void startframe();
	static void flushRender(RenderContext *render_context, Shader *debug_shader, const Mat4 &view_projection);
};

#endif // DEBUG_RENDERER_H