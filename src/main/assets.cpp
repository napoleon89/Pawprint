#define STB_IMAGE_IMPLEMENTATION
#include <core/stb_image.h>
#include <game/assets.h>
#include <engine/vox.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>


Texture2D loadTexture(u8 *image_data, u64 image_size, RenderContext *render_context, RenderContext::Format format, int force_components) {
	int tex_w, tex_h, tex_comps;
	stbi_set_flip_vertically_on_load(true);
	u8 *tex_data = (u8 *)stbi_loadf_from_memory((stbi_uc *)image_data, image_size, &tex_w, &tex_h, &tex_comps, force_components);
	const char *fail_reason = stbi_failure_reason();
	Texture2D result  = render_context->createTexture2D(tex_data, (u32)tex_w,  (u32)tex_h, format);
	stbi_image_free(tex_data);
	return result;	
}

Texture2D loadTextureFromFile(Platform *platform, char *filename, RenderContext *render_context, char *extension, RenderContext::Format format, int force_components) {
	FileData texture_file = platform->readEntireFile(formatString("data/textures/%s.%s", filename, extension));
	
	Texture2D result = loadTexture((u8 *)texture_file.contents, texture_file.size, render_context, format, force_components);
	
	platform->free(texture_file.contents);
	printf("Loaded %s\n", filename);
	return result;
}

Shader loadShaderFromFile(Platform *platform, RenderContext *render_context, std::string shader_name) {
	std::string v_path = "data/shaders/" + shader_name + ".vo";
	std::string p_path = "data/shaders/" + shader_name + ".po";	

	FileData vertex_file = platform->readEntireFile(v_path.c_str());
	FileData pixel_file = platform->readEntireFile(p_path.c_str());

	Shader result = render_context->createShader(vertex_file.contents, vertex_file.size, pixel_file.contents, pixel_file.size);
	return result;
}

TextureCube loadTextureCubeFromFolder(Platform *platform, RenderContext *render_context, std::string folder_name) {
	u8 *images[6];
	char *names[] = {
		"right",
		"left",
		"top",
		"bottom", 
		"front",
		"back",
	};

	int tex_w, tex_h, tex_comps;
	for(int i = 0; i < 6; i++) {
		std::string path = "data/textures/skyboxes/" + folder_name + "/" + std::string(names[i]) + ".jpg";
		FileData file = platform->readEntireFile(path.c_str());
		u8 *texture_data = stbi_load_from_memory((stbi_uc *)file.contents, file.size, &tex_w, &tex_h, &tex_comps, 4);
		images[i] = texture_data;
	}

	TextureCube result = render_context->createTextureCube(images, tex_w, tex_h, RenderContext::Format::u32_unorm);

	for(int i = 0; i < 6; i++) {
		stbi_image_free(images[i]);
	}

	return result;
}

ModelWrapper loadModelFromMemory(Platform *platform, RenderContext *render_context, u8 *memory, u64 size) {
	// ofbx::IScene *scene = ofbx::load(memory, size);
	// int mesh_count = scene->getMeshCount();
	// MeshWrapper *meshes = (MeshWrapper *)platform->alloc(sizeof(MeshWrapper) * mesh_count);
	// for(int i = 0; i < mesh_count; i++) {

	// 	MeshWrapper mesh_wrapper = {};
	// 	const ofbx::Mesh *mesh = scene->getMesh(i);
	// 	const ofbx::Geometry *geometry = mesh->getGeometry();
	// 	u32 vertex_count = (u32)geometry->getVertexCount();
	// 	Vertex *vertices = (Vertex *)platform->alloc(sizeof(Vertex) * vertex_count);
	// 	u16 *indices = (u16 *)platform->alloc(sizeof(u16) * vertex_count);
	// 	const ofbx::Vec3 *fbx_vertices = geometry->getVertices();
	// 	const ofbx::Vec3 *fbx_normals = geometry->getNormals();
	// 	const ofbx::Vec2 *fbx_uvs = geometry->getUVs();
	// 	for(u32 v = 0; v < vertex_count; v++) {
	// 		Vertex vertex = {};
	// 		const ofbx::Vec3 fbx_position = fbx_vertices[v];
	// 		const ofbx::Vec3 fbx_normal = fbx_normals[v];
	// 		const ofbx::Vec2 fbx_uv = fbx_uvs[v];
	// 		vertex.position = Vec3(fbx_position.x, fbx_position.y, fbx_position.z);
		
	// 		vertex.normal = Vec3::normalize(Vec3(fbx_normal.x, fbx_normal.y, fbx_normal.z));
	// 		vertex.uv = Vec2(fbx_uv.x, fbx_uv.y);
	// 		indices[v] = v;
	// 		vertices[v] = vertex;
	// 	}
	// 	const ofbx::Matrix local_mat = mesh->getGlobalTransform();
	// 	for(int m = 0; m < 16; m++) {
	// 		// mesh_wrapper.local_matrix.data1d[m] = local_mat.m[m];
	// 	}
	// 	// mesh_wrapper.local_matrix = Mat4::transpose(mesh_wrapper.local_matrix);
	// 	mesh_wrapper.vertices = vertices;
	// 	mesh_wrapper.vertex_count = vertex_count;
	// 	mesh_wrapper.indices = indices;
	// 	mesh_wrapper.index_count = vertex_count;
	// 	mesh_wrapper.vertex_buffer = render_context->createVertexBuffer(vertices, sizeof(Vertex), mesh_wrapper.vertex_count);
	// 	mesh_wrapper.index_buffer = render_context->createVertexBuffer(indices, sizeof(u16), mesh_wrapper.index_count, RenderContext::BufferType::Index);
	// 	meshes[i] = mesh_wrapper;

		
	// }

	// const ofbx::Object *root = scene->getRoot();	
	// processFbxObject(root, 0);

	ModelWrapper model = {};
	// model.meshes = meshes;
	// model.mesh_count = mesh_count;
	return model;
}

ModelWrapper loadModelFromFile(Platform *platform, RenderContext *render_context, const char *filepath) {
	FileData mesh_file = platform->readEntireFile(filepath);
	ModelWrapper result = loadModelFromMemory(platform, render_context, (u8 *)mesh_file.contents, mesh_file.size);
	platform->free(mesh_file.contents);
	return result;
}

#define ASSIMP_FLAGS aiProcess_MakeLeftHanded|aiProcess_LimitBoneWeights|aiProcess_JoinIdenticalVertices|aiProcess_Triangulate

StaticMeshData loadStaticMeshFromFile(Platform *platform, RenderContext *render_context, const char *filepath) {
	Assimp::Importer importer;
	// const aiScene *scene = importer.ReadFileFromMemory(file_data.contents, file_data.size, aiProcess_MakeLeftHanded);
	const aiScene *scene = importer.ReadFile(filepath, ASSIMP_FLAGS);
	if(scene == 0) {
		printf("Error: %s", importer.GetErrorString());
		printf("Couldn't load scene\n");
	}

	u32 mesh_count = scene->mNumMeshes;
	u32 vertex_count = 0;
	u32 index_count = 0;
	
	for(u32 mesh_index = 0; mesh_index < mesh_count; mesh_index++) {
		const aiMesh *mesh = scene->mMeshes[mesh_index];
		vertex_count += mesh->mNumVertices;
		index_count += mesh->mNumFaces * 3;
	}

	StaticMeshVertex *vertices = (StaticMeshVertex *)platform->alloc(sizeof(StaticMeshVertex) * vertex_count);
	u16 *indices = (u16 *)platform->alloc(sizeof(u16) * index_count);
	u32 index_offset = 0;
	u32 vertex_offset = 0;
	u32 index_vertex_count = 0;
	Vec3 min = Vec3(FLT_MAX);
	Vec3 max = Vec3(-FLT_MAX);
	for(u32 mesh_index = 0; mesh_index < mesh_count; mesh_index++) {
		const aiMesh *mesh = scene->mMeshes[mesh_index];
		const aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
		aiColor3D material_color;
		material->Get(AI_MATKEY_COLOR_DIFFUSE, material_color);
		Vec3 color = Vec3(material_color.r, material_color.g, material_color.b);
		for(u32 v = 0; v < mesh->mNumVertices; v++) {
			StaticMeshVertex vertex = {};
			const aiVector3D vertex_position = mesh->mVertices[v];
			const aiVector3D vertex_normal = mesh->mNormals[v];
				
			vertex.position = Vec3(vertex_position.x, vertex_position.y, vertex_position.z) * 5.0f;
			for(u32 j = 0; j < 3; j++) {
				if(vertex.position.xyz[j] < min.xyz[j]) {
					min.xyz[j] = vertex.position.xyz[j];
				}

				if(vertex.position.xyz[j] > max.xyz[j]) {
					max.xyz[j] = vertex.position.xyz[j];
				}
			}
			vertex.normal = Vec3(vertex_normal.x, vertex_normal.y, vertex_normal.z);
			vertex.color = color;
			vertices[vertex_offset++] = vertex;
		}

		for(u32 i = 0; i < mesh->mNumFaces; i++) {
			const aiFace *face = &mesh->mFaces[i];
			indices[index_offset++] = face->mIndices[0] + index_vertex_count;
			indices[index_offset++] = face->mIndices[2] + index_vertex_count;
			indices[index_offset++] = face->mIndices[1] + index_vertex_count;
		}
		index_vertex_count += mesh->mNumVertices;
	}

	StaticMeshData static_mesh = {};
	static_mesh.vertices = vertices;
	static_mesh.indices = indices;
	static_mesh.vertex_count = vertex_count;
	static_mesh.index_count = index_count;
	static_mesh.vertex_buffer = render_context->createVertexBuffer(vertices, sizeof(StaticMeshVertex), vertex_count);
	static_mesh.index_buffer = render_context->createVertexBuffer(indices, sizeof(u16), index_count, RenderContext::BufferType::Index);
	static_mesh.min = min;
	static_mesh.max = max;
	return static_mesh;
}

internal_func Mat4 aiMatToMat4(const aiMatrix4x4 &m) {
	Mat4 transform = Mat4();
	for(int i = 0; i < 16; i++) {
		transform.data1d[i] = ((f32 *)&m)[i];
	}
	return transform;
}

internal_func void processNode(Platform *platform, aiNode *node, u32 depth, s32 parent_index, Mat4 parent_world, std::vector<Joint> &joint_list) {
	depth++;
	Mat4 transform = aiMatToMat4(node->mTransformation);
	Mat4 world = parent_world * transform;

	Joint joint = {};
	joint.local_mat = transform;
	joint.world_mat = world;
	joint.parent_index = parent_index;
	joint.name = (char *)platform->alloc(sizeof(char) * node->mName.length);
	strcpy(joint.name, node->mName.C_Str());
	u32 pi = (u32)joint_list.size();
	joint_list.push_back(joint);
	parent_index++;
	

	for(u32 i = 0; i < node->mNumChildren; i++) {
		processNode(platform, node->mChildren[i], depth, pi, world, joint_list);
	}
}

SkeletalMesh loadSkeletalMeshFromFile(Platform *platform, RenderContext *render_context, const char *filepath) {
	Assimp::Importer importer;
	// const aiScene *scene = importer.ReadFileFromMemory(file_data.contents, file_data.size, aiProcess_MakeLeftHanded);
	const aiScene *scene = importer.ReadFile(filepath, ASSIMP_FLAGS);
	if(scene == 0) {
		printf("Error: %s", importer.GetErrorString());
		printf("Couldn't load scene\n");
	}

	std::vector<Joint> joints;
	// ::rotateX(Math::Pi32 * 0.5f)
	processNode(platform, scene->mRootNode, 0, -1, Mat4(), joints);
	u32 joint_count = (u32)joints.size();
	Skeleton skeleton = {};
	skeleton.joints = (Joint *)platform->alloc(sizeof(Joint) * joint_count);
	memcpy(skeleton.joints, &joints[0], sizeof(Joint) * joint_count);
	skeleton.num_joints = joint_count;
	joints.clear();

	for(u32 mesh_index = 0; mesh_index < scene->mNumMeshes; mesh_index++) {
		const aiMesh *mesh = scene->mMeshes[mesh_index];
		for(u32 bone_index = 0; bone_index < mesh->mNumBones; bone_index++) {
			const aiBone *bone = mesh->mBones[bone_index];
			skeleton.joints[skeleton.getJointIndexFromName(bone->mName.C_Str())].inv_bind_mat = aiMatToMat4(bone->mOffsetMatrix);
		}
	}

	// ------------------------------------------------------------------------------------------------------------------------------------

	u32 mesh_count = scene->mNumMeshes;
	u32 vertex_count = 0;
	u32 index_count = 0;
	
	for(u32 mesh_index = 0; mesh_index < mesh_count; mesh_index++) {
		const aiMesh *mesh = scene->mMeshes[mesh_index];
		vertex_count += mesh->mNumVertices;
		index_count += mesh->mNumFaces * 3;
	}

	SkinVertex *vertices = (SkinVertex *)platform->alloc(sizeof(SkinVertex) * vertex_count);
	u16 *indices = (u16 *)platform->alloc(sizeof(u16) * index_count);
	u32 index_offset = 0;
	u32 vertex_offset = 0;
	u32 index_vertex_count = 0;
	for(u32 mesh_index = 0; mesh_index < mesh_count; mesh_index++) {
		const u32 start_mesh_offset = vertex_offset;
		const aiMesh *mesh = scene->mMeshes[mesh_index];
		for(u32 v = 0; v < mesh->mNumVertices; v++) {
			SkinVertex vertex = {};
			const aiVector3D vertex_position = mesh->mVertices[v];
			const aiVector3D vertex_normal = mesh->mNormals[v];
				
			vertex.position = Vec3(vertex_position.x, vertex_position.y, vertex_position.z);
			vertex.normal = Vec3(vertex_normal.x, vertex_normal.y, vertex_normal.z);
			for(u32 i = 0; i < 4; i++) {
				vertex.joint_ids[i] = -1;
			}
			vertices[vertex_offset++] = vertex;
		}

		for(u32 i = 0; i < mesh->mNumFaces; i++) {
			const aiFace *face = &mesh->mFaces[i];
			Assert(face->mNumIndices == 3);
			indices[index_offset++] = face->mIndices[0] + index_vertex_count;
			indices[index_offset++] = face->mIndices[2] + index_vertex_count;
			indices[index_offset++] = face->mIndices[1] + index_vertex_count;
		}
		index_vertex_count += mesh->mNumVertices;

		for(u32 bone_index = 0; bone_index < mesh->mNumBones; bone_index++) {
			const aiBone *bone = mesh->mBones[bone_index];
			s32 joint_index = skeleton.getJointIndexFromName(bone->mName.C_Str());
			if(joint_index > -1) {
				for(u32 vertex_weight_index = 0; vertex_weight_index < bone->mNumWeights; vertex_weight_index++) {
					const aiVertexWeight &weight = bone->mWeights[vertex_weight_index];
					SkinVertex &vertex = vertices[start_mesh_offset + weight.mVertexId];
					for(u32 weight_index = 0; weight_index < 4; weight_index++) {
						if(vertex.joint_ids[weight_index] < 0) {
							vertex.weights.xyzw[weight_index] = weight.mWeight;
							vertex.joint_ids[weight_index] = (u8)joint_index;
							break;
						}
					}
				}
			}
		}

		for(u32 i = 0; i < vertex_count; i++) {
			const SkinVertex &vertex = vertices[i];
			Assert(vertex.joint_ids[0] != -1);
		}
	}

	
	SkeletalMesh result = {};
	result.skeleton = skeleton;
	result.vertices = vertices;
	result.indices = indices;
	result.vertex_count = vertex_count;
	result.index_count = index_count;
	result.vertex_buffer = render_context->createVertexBuffer(vertices, sizeof(SkinVertex), vertex_count);
	result.index_buffer = render_context->createVertexBuffer(indices, sizeof(u16), index_count, RenderContext::BufferType::Index);

	return result;
}

AnimationClip loadClipFromFile(Platform *platform, Skeleton *skeleton, const char *filepath) {
	Assimp::Importer importer;
	// const aiScene *scene = importer.ReadFileFromMemory(file_data.contents, file_data.size, aiProcess_MakeLeftHanded);
	const aiScene *scene = importer.ReadFile(filepath, aiProcess_MakeLeftHanded|aiProcess_Triangulate);
	if(scene == 0) {
		printf("Error: %s", importer.GetErrorString());
		printf("Couldn't load scene\n");
	}

	Assert(scene->mNumAnimations > 0);

	
	const aiAnimation *anim = scene->mAnimations[0];
	float duration_seconds = anim->mDuration / anim->mTicksPerSecond;
	AnimationChannel *channels = (AnimationChannel *)platform->alloc(sizeof(AnimationChannel) * anim->mNumChannels);
	for(u32 channel_index = 0; channel_index < anim->mNumChannels; channel_index++) {
		const aiNodeAnim *channel = anim->mChannels[channel_index];
		Assert(channel->mNumPositionKeys == channel->mNumScalingKeys);
		Assert(channel->mNumRotationKeys == channel->mNumScalingKeys);
		u32 frame_count = channel->mNumPositionKeys;
		JointPose *poses = (JointPose *)platform->alloc(sizeof(JointPose) * frame_count);
		f32 *frame_times = (f32 *)platform->alloc(sizeof(f32) * frame_count);
		for(u32 key_index = 0; key_index < frame_count; key_index++) {
			aiVectorKey pos_key = channel->mPositionKeys[key_index];
			aiQuatKey rot_key = channel->mRotationKeys[key_index];
			aiVectorKey scale_key = channel->mScalingKeys[key_index];

			frame_times[key_index] = (pos_key.mTime / anim->mDuration) * duration_seconds;

			JointPose pose = {};
			pose.translation = Vec3(pos_key.mValue.x, pos_key.mValue.y, pos_key.mValue.z);
			pose.scale = Vec3(scale_key.mValue.x, scale_key.mValue.y, scale_key.mValue.z);
			pose.rotation = Quat(rot_key.mValue.x, rot_key.mValue.y, rot_key.mValue.z, rot_key.mValue.w);
			poses[key_index] = pose;
		}

		AnimationChannel result_channel = {};
		result_channel.frame_count = frame_count;
		result_channel.frame_times = frame_times;
		result_channel.joint_index = skeleton->getJointIndexFromName(channel->mNodeName.C_Str());
		result_channel.poses = poses;
		channels[channel_index] = result_channel;
	}

	AnimationClip result = {};
	result.channel_count = anim->mNumChannels;
	result.channels = channels;
	result.duration_seconds = duration_seconds;

	return result;
}