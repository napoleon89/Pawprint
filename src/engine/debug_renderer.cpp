#include <engine/debug_renderer.h>
#include <core/render_context.h>

struct DebugRenderLine {
	Vec4 color;
	Vec3 start;
	Vec3 end;
	f32 thickness;
};

struct DebugRenderAABB {
	Vec3 min, max;
	Vec4 color;
	f32 thickness;
};

struct DebugPerScene {
	Mat4 view_projection;
};

struct DebugPerObject {
	Mat4 model;
	Vec4 color;
};

global_variable std::vector<DebugRenderLine> g_debug_lines;
global_variable std::vector<DebugRenderAABB> g_debug_aabbs;
global_variable ShaderConstant g_per_scene_constant;
global_variable ShaderConstant g_per_object_constant;
global_variable ShaderLayout g_debug_shader_layout;

void DebugRenderQueue::init(RenderContext *render_context, Shader *debug_shader) {
	RenderContext::LayoutElement layout[] = {
		{"POSITION", RenderContext::Format::Vec3}
	};
	g_debug_shader_layout = render_context->createShaderLayout(layout, ArrayCount(layout), debug_shader);
	g_per_scene_constant = render_context->createShaderConstant(sizeof(DebugPerScene));
	g_per_object_constant = render_context->createShaderConstant(sizeof(DebugPerObject));

}

void DebugRenderQueue::addLine(const Vec3 &start, const Vec3 &end, const Vec4 &color, f32 thickness) {
	DebugRenderLine line = {};
	line.start = start;
	line.end = end;
	line.color = color;
	line.thickness = thickness;
	g_debug_lines.push_back(line);
}

void DebugRenderQueue::addAABB(const Vec3 &min, const Vec3 &max, const Vec4 &color, f32 thickness) {
	DebugRenderAABB aabb = {};
	aabb.min = min;
	aabb.max = max;
	aabb.color = color;
	aabb.thickness = thickness;
	g_debug_aabbs.push_back(aabb);
}

void DebugRenderQueue::flushRender(RenderContext *render_context, Shader *debug_shader, const Mat4 &view_projection) {

	for(size_t i = 0; i < g_debug_aabbs.size(); i++) {
		DebugRenderAABB &handle = g_debug_aabbs[i];
		// drawLineHandle(handle.min, handle.max, handle.color);
		Vec3 bottom_front_left 	= Vec3(handle.min.x, handle.min.y, handle.min.z);
		Vec3 bottom_front_right = Vec3(handle.max.x, handle.min.y, handle.min.z);
		Vec3 bottom_back_left 	= Vec3(handle.min.x, handle.min.y, handle.max.z);
		Vec3 bottom_back_right 	= Vec3(handle.max.x, handle.min.y, handle.max.z);

		Vec3 top_front_left 	= Vec3(handle.min.x, handle.max.y, handle.min.z);
		Vec3 top_front_right 	= Vec3(handle.max.x, handle.max.y, handle.min.z);
		Vec3 top_back_left 		= Vec3(handle.min.x, handle.max.y, handle.max.z);
		Vec3 top_back_right 	= Vec3(handle.max.x, handle.max.y, handle.max.z);

		addLine(bottom_front_left, bottom_back_left, handle.color);
		addLine(bottom_back_left, bottom_back_right, handle.color);				
		addLine(bottom_back_right, bottom_front_right, handle.color);				
		addLine(bottom_front_right, bottom_front_left, handle.color);				

		addLine(top_front_left, top_back_left, handle.color);
		addLine(top_back_left, top_back_right, handle.color);
		addLine(top_back_right, top_front_right, handle.color);
		addLine(top_front_right, top_front_left, handle.color);

		addLine(bottom_front_left, top_front_left, handle.color);
		addLine(bottom_back_left, top_back_left, handle.color);				
		addLine(bottom_back_right, top_back_right, handle.color);				
		addLine(bottom_front_right, top_front_right, handle.color);				
	}

	render_context->bindShader(debug_shader);
	{
		Vec3 vertices[] = {
			Vec3(0.0f, 0.5f, 0.0f),
			Vec3(0.0f, 0.5f, 1.0f),
			Vec3(0.0f, -0.5f, 0.0f),
			Vec3(0.0f, -0.5f, 1.0f),

			Vec3(-0.5f, 0.0f, 0.0f),
			Vec3(-0.5f, 0.0f, 1.0f),
			Vec3(0.5f, 0.0f, 0.0f),
			Vec3(0.5f, 0.0f, 1.0f),
		};

		u16 indices[] = {
			0, 1, 2,
			2, 1, 3,
			4, 5, 6,
			6, 5, 7,

			1, 0, 2,
			1, 2, 3,
			5, 4, 6,
			5, 6, 7,
		};

		DebugPerScene per_scene_data = {};
		per_scene_data.view_projection = view_projection;
		render_context->updateShaderConstant(&g_per_scene_constant, &per_scene_data);
		render_context->bindShaderConstant(&g_per_scene_constant, 0, 0);

		render_context->bindShaderConstant(&g_per_object_constant, 1, 1);
		render_context->bindShaderLayout(&g_debug_shader_layout);
		u32 vertex_count = ArrayCount(vertices);
		u32 index_count = ArrayCount(indices);
		VertexBuffer vb = render_context->createVertexBuffer(vertices, sizeof(Vec3), vertex_count);
		VertexBuffer ib = render_context->createVertexBuffer(indices, sizeof(u16), index_count, RenderContext::BufferType::Index);
		render_context->bindVertexBuffer(&vb, 0);
		render_context->bindIndexBuffer(&ib, RenderContext::Format::u16);
		DebugPerObject per_object = {};
		for(size_t i = 0; i < g_debug_lines.size(); i++) {
			DebugRenderLine &handle = g_debug_lines[i];
			
			
			per_object.color = handle.color;
			Vec3 dir = Vec3::normalize(handle.end - handle.start);
			// Fix for co-planar vectors
			Vec3 world_right = Vec3();
			if(dir == Vec3::up) world_right = Vec3(1.0f, 0.0f, 0.0f);
			else if(dir == Vec3(0.0f, -1.0f, 0.0f)) world_right = Vec3(-1.0f, 0.0f, 0.0f);
			else world_right = Vec3::normalize(Vec3::cross(dir, Vec3(0.0f, 1.0f, 0.0f)));
			Vec3 local_up = Vec3::normalize(Vec3::cross(world_right, dir));
			
			per_object.model = Mat4::translate(handle.start) * Mat4::basisChange(world_right, local_up, dir) * Mat4::scale(Vec3(handle.thickness, handle.thickness, Vec3::length(handle.end - handle.start)));
			render_context->updateShaderConstant(&g_per_object_constant, &per_object);
			render_context->sendDrawIndexed(RenderContext::Topology::TriangleList, index_count, 0, 0);
		}

		
		render_context->destroyVertexBuffer(&vb);
		render_context->destroyVertexBuffer(&ib);
	}

	g_debug_lines.clear();
	g_debug_aabbs.clear();
}