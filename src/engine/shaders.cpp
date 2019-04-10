#include <engine/shaders.h>
#include <functional>
#include <algorithm>
#include <engine/json.h>

internal_func u32 computeShaderHash(const char *shader_name) {
	std::hash<std::string> hasher;
	u32 hashed = (u32)hasher(shader_name);
	return hashed;
}

inline ShaderDB::ShaderPassKey computeShaderPassKey(ShaderDB::ShaderPassKey start_key, ShaderDB::ShaderPassKey end_key) {
	ShaderDB::ShaderPassKey result = 0;
	result |= start_key;
	result = result << 32;
	result |= end_key;
	return result;
}

s32 ShaderDB::findShaderIndex(u32 shader_key) {
	for(u32 i = 0; i < shader_keys.count; i++) {
		const ShaderPassKey &key = shader_keys.getRefConst(i);
		if((shader_key & (key >> 32)) == key) {
			return i;
		}
	}
	return -1;
}

void ShaderDB::loadShader(Platform *platform, RenderContext *render_context, const char *name) {
	s32 index = findShaderIndex(computeShaderHash(name));
	Assert(index != -1);

}

void ShaderDB::loadFromMap(u8 *map_data, u64 map_size) {
	json_value_s *json = json_parse(map_data, (size_t)map_size);
	json_object_s *root_obj = (json_object_s *)json->payload;
	json_object_element_s *obj = root_obj->start;

	do {
		ShaderDB::ShaderPassKey start_key = computeShaderHash(obj->name->string);
		shader_keys.add(computeShaderPassKey(start_key, 0));
		shader_filenames.add(obj->name->string);

		json_object_s *conditionals = (json_object_s *)obj->value->payload;
		json_object_element_s *conditional = conditionals->start;
		if(conditional != 0) {
			do {
				if(conditional->value->type == json_type_string) {
					ShaderDB::ShaderPassKey end_key = computeShaderHash(conditional->name->string);
					json_string_s *pass_name = (json_string_s *)conditional->value->payload;
					shader_keys.add(computeShaderPassKey(start_key, end_key));
					shader_filenames.add(pass_name->string);
				}
				
			} while((conditional = conditional->next) != 0);
		}
	} while((obj = obj->next) != 0);
	std::sort(shader_keys.data, shader_keys.data + shader_keys.count);
}

