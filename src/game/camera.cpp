#include <game/camera.h>
#include <engine/input.h>
#include <core/platform.h>

Vec3 Camera::forward(bool remove_y) {
	f32 y = 0.0f;
	if(!remove_y) y = Math::sin(Math::toRadians(pitch));
	return Vec3::normalize(Vec3(Math::cos(Math::toRadians(yaw)) * Math::cos(Math::toRadians(pitch)), y, Math::sin(Math::toRadians(yaw)) * Math::cos(Math::toRadians(pitch))));
}	

Vec3 Camera::right(bool remove_y) {
	return Vec3::normalize(Vec3::cross(Vec3(0.0f, 1.0f, 0.0f), forward(remove_y)));	
}

Vec3 Camera::up() {
	return Vec3::normalize(Vec3::cross(forward(), right()));
}

Mat4 Camera::getProjection() {
	return Mat4::perspective(90.0f, (f32)window_dimensions.x / (f32)window_dimensions.y, 0.01f, 1000.0f);
}

Mat4 Camera::getView(bool include_translation) {
	if(include_translation)
		return Mat4::lookAt(-position, forward(), Vec3(0.0f, 1.0f, 0.0f));
	else
		return Mat4::lookAt(Vec3(), forward(), Vec3(0.0f, 1.0f, 0.0f));

	// return Mat4::translate(-position) * Mat4::rotateX(Math::toRadians(pitch)) * Mat4::rotateY(Math::toRadians(yaw));
	// return Mat4();
}

Mat4 Camera::getViewProjection(bool include_translation) {
	return getProjection() * getView(include_translation);
}

void Camera::updatecameraLook(Platform *platform, InputManager *input, f32 delta) {
	
	if(input->isMouseButtonDown(MouseButton::Right)) {
		platform->setCursorVisible(false);
		if(input->isMouseButtonDownOnce(MouseButton::Right)) {
			lock_mouse_pos = input->getMousePosition();
		}
		
		Vec2 mouse_delta = v2iToV2(input->getMousePosition() - lock_mouse_pos);
		f32 rotation_speed = 10.0f;
		yaw -= mouse_delta.x * rotation_speed * delta;
		pitch -= mouse_delta.y * rotation_speed * delta;

		input->setMousePosition(platform, lock_mouse_pos.x, lock_mouse_pos.y);
	} else {
		platform->setCursorVisible(true);
	}
	
	
	yaw = Math::wrap(yaw, 0.0f, 360.0f);
	pitch = Math::clamp(pitch, -89.0f, 89.0f);
}

void Camera::beginFrame(Platform *platform, PlatformWindow *window) {
	calculateWindowBounds(platform, window);
}

void Camera::calculateWindowBounds(Platform *platform, PlatformWindow *window) {
	u32 window_width, window_height;
	platform->getWindowSize(window, window_width, window_height);
	window_dimensions = Vec2i(window_width, window_height);
}

void Camera::endFrame() {
	
}

Vec3 Camera::screenNormToRay(Vec2 screen_pos) {
	Vec4 ray_clip = Vec4(screen_pos.x, screen_pos.y, 1.0f, 1.0f);
	Mat4 projection = getProjection();
	Mat4 my_inv_p = Mat4::invPerspective(projection);
	Vec4 ray_eye = Mat4::transform(my_inv_p, ray_clip);
	
	Vec3 ray_wor = Mat4::transform(Mat4::transpose(getView()), Vec4(ray_eye.x, ray_eye.y, 1.0f, 0.0f)).xyz;
	ray_wor = Vec3::normalize(ray_wor);
	return ray_wor;
}

Vec3 Camera::screenToRay(Vec2i screen_pos) {
	f32 screen_x = ((f32)screen_pos.x / (f32)window_dimensions.x) * 2.0f - 1.0f;
	f32 screen_y = -(((f32)screen_pos.y / (f32)window_dimensions.y) * 2.0f - 1.0f);
	return screenNormToRay(Vec2(screen_x, screen_y));
}