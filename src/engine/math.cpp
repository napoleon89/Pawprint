#include <engine/math.h>

Vec3 Vec3::up = Vec3(0.0f, 1.0f, 0.0f);
Vec3 Vec3::forward = Vec3(0.0f, 0.0f, 1.0f);
Vec3 Vec3::right = Vec3(1.0f, 0.0f, 0.0f);

Vec2 v2iToV2(Vec2i input) {
	return Vec2(input.x, input.y);
}
