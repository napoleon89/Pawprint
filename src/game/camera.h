#ifndef CAMERA_H
#define CAMERA_H

#include <engine/math.h>
#include <core/render_context.h>

struct Platform;
struct InputManager;
struct PlatformWindow;

REFLECT_STRUCT
struct Camera {
	REFLECT_FIELD Vec3 position;
	REFLECT_FIELD Vec2i window_dimensions;
	REFLECT_FIELD f32 yaw;
	REFLECT_FIELD f32 pitch;
	REFLECT_FIELD f32 roll;
	REFLECT_FIELD Vec2i lock_mouse_pos;
	REFLECT_FIELD Vec3 target_pos;
	
	Vec3 forward(bool remove_y = false);
	Vec3 right(bool remove_y = false);
	Vec3 up();
	Mat4 getProjection();
	Mat4 getView(bool include_translation = true);
	Mat4 getViewProjection(bool include_translation = true);
	void updatecameraLook(Platform *platform, InputManager *input, f32 delta);
	void beginFrame(Platform *platform, PlatformWindow *window);
	void calculateWindowBounds(Platform *platform, PlatformWindow *window);
	void endFrame();
	Vec3 screenNormToRay(Vec2 screen_pos);
	Vec3 screenToRay(Vec2i screen_pos);
};

#endif // CAMERA_H