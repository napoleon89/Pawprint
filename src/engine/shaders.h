#ifndef SHADERS_H
#define SHADERS_H

#include <engine/std.h>
#include <engine/collections.h>
#include <string>

struct Platform;
struct RenderContext;

struct ShaderDB {
	typedef u64 ShaderPassKey;

	DynamicArray<u32> shader_keys;
	DynamicArray<const char *> shader_filenames;

	void loadFromMap(u8 *map_data, u64 map_size);
	void loadShader(Platform *platform, RenderContext *render_context, const char *name);
	s32 findShaderIndex(u32 shader_key);
};

#endif // SHADERS_H