#ifndef VOX_H
#define VOX_H

#include <engine/std.h>
#include <engine/math.h>

#define EnumName ChunkType
#define EnumType s32
#define EnumValues \
	EnumValue(Main, 1313423693) \
	EnumValue(Size, 1163544915) \
	EnumValue(Xyzi, 1230657880) \
	EnumValue(Rgba, 1094862674)

#include <engine/enum.cpp>

struct Platform;

struct Voxel {
	u8 x, y, z, color_index;
};

struct VoxModel {
	Vec3 size;
	u32 *palette;
	Voxel *voxels;
	u32 voxel_count;
};

VoxModel loadVoxFileFromMemory(Platform *platform, u8 *data, u64 data_size);
VoxModel loadVoxFromFile(Platform *platform, char *file_path);

#endif // VOX_H