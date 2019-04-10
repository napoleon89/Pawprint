#include <main/reflection.h>
#include <engine/math.h>

using namespace Reflection;

void Reflection::Class::print() {
	printf("%s\n", name);
	for(u32 i = 0; i < field_count; i++) {
		const Field &field = fields[i];
		if(field.type->is_primitive) {
			printf("%s %s\n", field.type->name, field.name);
		} else {
			Class *class_type = (Class *)field.type;
			class_type->print();
		} 
	}
}

template<typename T>
const Type *getType();

#define GetType(type, fmt) template<> const Type *getType<type>() { static Type result = {#type, fmt, sizeof(type), true}; return &result; }
GetType(s8, "%d") 
GetType(s16, "%d") 
GetType(s32, "%d") 
GetType(s64, "%d") 
GetType(u8, "%d") 
GetType(u16, "%d") 
GetType(u32, "%d") 
GetType(u64, "%d") 
GetType(f32, "%.3f") 
GetType(f64, "%.3f")

template<>
Class *Reflection::getStruct<Vec2>() {
	static Class result;
	result.name = "Vec2";
	result.fields = new Field[2];
	result.field_count = 2;
	result.size = sizeof(Vec2);

	result.fields[0].type = getType<f32>();
	result.fields[0].name = "x";
	result.fields[0].offset = offsetof(Vec2, x);

	result.fields[1].type = getType<f32>();
	result.fields[1].name = "y";
	result.fields[1].offset = offsetof(Vec2, y);

	return &result;
}

template<>
const Type *getType<Vec2>() { 
	return (const Type *)getStruct<Vec2>();
}

template<>
Class *Reflection::getStruct<Vec2i>() {
	static Class result;
	result.name = "Vec2i";
	result.fields = new Field[2];
	result.field_count = 2;
	result.size = sizeof(Vec2i);

	result.fields[0].type = getType<s32>();
	result.fields[0].name = "x";
	result.fields[0].offset = offsetof(Vec2i, x);

	result.fields[1].type = getType<s32>();
	result.fields[1].name = "y";
	result.fields[1].offset = offsetof(Vec2i, y);

	return &result;
}

template<>
const Type *getType<Vec2i>() { 
	return (const Type *)getStruct<Vec2i>();
}

// == REFLECT_DATA_BEGIN
#include <engine\math.h>
#include <game\camera.h>

template<>
Class *Reflection::getStruct<Vec3>() {
	static Class result;
	result.name = "Vec3";
	result.fields = new Field[3];
	result.field_count = 3;
	result.size = sizeof(Vec3);

	result.fields[0].type = getType<f32>();
	result.fields[0].name = "x";
	result.fields[0].offset = offsetof(Vec3, x);
	result.fields[1].type = getType<f32>();
	result.fields[1].name = "y";
	result.fields[1].offset = offsetof(Vec3, y);
	result.fields[2].type = getType<f32>();
	result.fields[2].name = "z";
	result.fields[2].offset = offsetof(Vec3, z);


	return &result;
}

template<>
const Type *getType<Vec3>() { 
	return (const Type *)getStruct<Vec3>();
}
template<>
Class *Reflection::getStruct<Vec4>() {
	static Class result;
	result.name = "Vec4";
	result.fields = new Field[4];
	result.field_count = 4;
	result.size = sizeof(Vec4);

	result.fields[0].type = getType<f32>();
	result.fields[0].name = "x";
	result.fields[0].offset = offsetof(Vec4, x);
	result.fields[1].type = getType<f32>();
	result.fields[1].name = "y";
	result.fields[1].offset = offsetof(Vec4, y);
	result.fields[2].type = getType<f32>();
	result.fields[2].name = "z";
	result.fields[2].offset = offsetof(Vec4, z);
	result.fields[3].type = getType<f32>();
	result.fields[3].name = "w";
	result.fields[3].offset = offsetof(Vec4, w);


	return &result;
}

template<>
const Type *getType<Vec4>() { 
	return (const Type *)getStruct<Vec4>();
}
template<>
Class *Reflection::getStruct<Camera>() {
	static Class result;
	result.name = "Camera";
	result.fields = new Field[7];
	result.field_count = 7;
	result.size = sizeof(Camera);

	result.fields[0].type = getType<Vec3>();
	result.fields[0].name = "position";
	result.fields[0].offset = offsetof(Camera, position);
	result.fields[1].type = getType<Vec2i>();
	result.fields[1].name = "window_dimensions";
	result.fields[1].offset = offsetof(Camera, window_dimensions);
	result.fields[2].type = getType<f32>();
	result.fields[2].name = "yaw";
	result.fields[2].offset = offsetof(Camera, yaw);
	result.fields[3].type = getType<f32>();
	result.fields[3].name = "pitch";
	result.fields[3].offset = offsetof(Camera, pitch);
	result.fields[4].type = getType<f32>();
	result.fields[4].name = "roll";
	result.fields[4].offset = offsetof(Camera, roll);
	result.fields[5].type = getType<Vec2i>();
	result.fields[5].name = "lock_mouse_pos";
	result.fields[5].offset = offsetof(Camera, lock_mouse_pos);
	result.fields[6].type = getType<Vec3>();
	result.fields[6].name = "target_pos";
	result.fields[6].offset = offsetof(Camera, target_pos);


	return &result;
}

template<>
const Type *getType<Camera>() { 
	return (const Type *)getStruct<Camera>();
}
