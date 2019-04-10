#include <core/platform.h>
#include <engine/mesh_utils.h>

void circleTable(Platform *platform, f32 **sint, f32 **cost, const int n, const bool halfCircle) {
	int i;
	
	/* Table size, the sign of n flips the circle direction */
	const int size = Math::abs(n);

	/* Determine the angle between samples */
	const f32 angle = (halfCircle?1:2)*(f32)Math::Pi32/(f32)( ( n == 0 ) ? 1 : n );

	/* Allocate memory for n samples, plus duplicate of first entry at the end */
	*sint = (f32 *)platform->alloc(sizeof(f32) * (size+1));
	*cost = (f32 *)platform->alloc(sizeof(f32) * (size+1));

	/* Bail out if memory allocation fails, fgError never returns */
	if (!(*sint) || !(*cost))
	{
		platform->free(*sint);
		platform->free(*cost);
	}

	/* Compute cos and sin around the circle */
	(*sint)[0] = 0.0;
	(*cost)[0] = 1.0;

	for (i=1; i<size; i++)
	{
		(*sint)[i] = (f32)Math::sin(angle*i);
		(*cost)[i] = (f32)Math::cos(angle*i);
	}

	
	if (halfCircle)
	{
		(*sint)[size] =  0.0f;  /* sin PI */
		(*cost)[size] = -1.0f;  /* cos PI */
	}
	else
	{
		/* Last sample is duplicate of the first (sin or cos of 2 PI) */
		(*sint)[size] = (*sint)[0];
		(*cost)[size] = (*cost)[0];
	}
}

MeshWrapper MeshUtils::genCubeMesh(Platform *platform, RenderContext *render_context) {
	Vertex vertices[] = {
		// front
		{Vec3(-1.0f, 1.0f, -1.0f), -Vec3::forward}, 	// Top Front left 
		{Vec3(1.0f, 1.0f, -1.0f), -Vec3::forward}, 		// Top Front right
		{Vec3(-1.0f, -1.0f, -1.0f), -Vec3::forward}, 	// bottom Front left
		{Vec3(1.0f, -1.0f, -1.0f), -Vec3::forward}, 	// bottom Front right
		// right
		{Vec3(1.0f, 1.0f, -1.0f), Vec3::right},			// Top Front right 
		{Vec3(1.0f, 1.0f, 1.0f), Vec3::right}, 			// Top back right
		{Vec3(1.0f, -1.0f, -1.0f), Vec3::right}, 		// bottom Front right
		{Vec3(1.0f, -1.0f, 1.0f), Vec3::right}, 		// bottom back right~
		// back
		{Vec3(1.0f, 1.0f, 1.0f), Vec3::forward}, 		// Top back right
		{Vec3(-1.0f, 1.0f, 1.0f), Vec3::forward}, 		// Top back left 
		{Vec3(1.0f, -1.0f, 1.0f), Vec3::forward}, 		// bottom back right~
		{Vec3(-1.0f, -1.0f, 1.0f), Vec3::forward}, 		// bottom back left
		// left
		{Vec3(-1.0f, 1.0f, 1.0f), -Vec3::right}, 		// Top back left 
		{Vec3(-1.0f, 1.0f, -1.0f), -Vec3::right}, 		// Top Front left
		{Vec3(-1.0f, -1.0f, 1.0f), -Vec3::right}, 		// bottom back left
		{Vec3(-1.0f, -1.0f, -1.0f), -Vec3::right}, 		// bottom Front left
		// top
		{Vec3(-1.0f, 1.0f, 1.0f), Vec3::up}, 			// Top back left 
		{Vec3(1.0f, 1.0f, 1.0f), Vec3::up}, 			// Top back right
		{Vec3(-1.0f, 1.0f, -1.0f), Vec3::up}, 			// Top Front left
		{Vec3(1.0f, 1.0f, -1.0f), Vec3::up}, 			// Top Front right
		// bottom
		{Vec3(-1.0f, -1.0f, -1.0f), -Vec3::up}, 		// bottom Front left
		{Vec3(1.0f, -1.0f, -1.0f), -Vec3::up}, 			// bottom Front right
		{Vec3(-1.0f, -1.0f, 1.0f), -Vec3::up}, 			// bottom back left 
		{Vec3(1.0f, -1.0f, 1.0f), -Vec3::up}, 			// bottom back right~
	};

	u16 indices[36];

	for(u16 i = 0; i < 6; i++) {
		u16 index_start = i * 6;
		u16 vertex_start = i * 4;
		indices[index_start] = vertex_start;
		indices[index_start+1] = vertex_start + 1;
		indices[index_start+2] = vertex_start + 2;

		indices[index_start+3] = vertex_start + 2;
		indices[index_start+4] = vertex_start + 1;
		indices[index_start+5] = vertex_start + 3;

		vertices[vertex_start + 0].uv = Vec2(0.0f, 0.0f);
		vertices[vertex_start + 1].uv = Vec2(1.0f, 0.0f);
		vertices[vertex_start + 2].uv = Vec2(0.0f, 1.0f);
		vertices[vertex_start + 3].uv = Vec2(1.0f, 1.0f);
	}

	MeshWrapper result = {};
	result.vertex_count = ArrayCount(vertices);
	u32 vertex_memory_size = result.vertex_count * sizeof(Vertex);
	result.vertices = (Vertex *)platform->alloc(vertex_memory_size);
	memcpy(result.vertices, &vertices[0], vertex_memory_size);
	
	result.index_count = 36;
	u32 index_memory_size = result.index_count * sizeof(u16);
	result.indices = (u16 *)platform->alloc(index_memory_size);
	memcpy(result.indices, &indices[0], index_memory_size);;

	result.vertex_buffer = render_context->createVertexBuffer(vertices, sizeof(Vertex), result.vertex_count);
	result.index_buffer = render_context->createVertexBuffer(indices, sizeof(u16), result.index_count, RenderContext::BufferType::Index);

	return result;
}

MeshWrapper MeshUtils::genSphereMesh(Platform *platform, RenderContext *render_context, Vec3 center, f32 radius) {
	s32 slices = 32;
	s32 stacks = 32;
	MeshWrapper result = {};
	result.topology_format = RenderContext::Topology::TriangleStrip;
	int vertex_count = slices*(stacks-1)+2;
	Vertex *vertices = (Vertex *)platform->alloc(vertex_count * sizeof(Vertex));
	int idx = 0;    /* idx into vertex/normal buffer */

	/* Pre-computed circle */
	f32 *sint1,*cost1;
	f32 *sint2,*cost2;

	/* precompute values on unit circle */
	circleTable(platform, &sint1,&cost1,slices,false);
	circleTable(platform, &sint2,&cost2, stacks,true);

	/* top */
	vertices[0] = { 
		Vec3(0.0f, radius, 0.0f),
		Vec3(0.0f, 1.0f, 0.0f),
		Vec2(0.0f, 0.0f),
	};
	
	idx = 1;

	/* each stack */
	for(int i=1; i<stacks; i++ )
	{
		for(int j=0; j<slices; j++, idx++)
		{
			f32 x = cost1[j]*sint2[i];
			f32 y = cost2[i];
			f32 z = sint1[j]*sint2[i];
			f32 u = (f32)j / (f32)stacks;
			f32 v = (f32)i / (f32)stacks;
			
			// uvs[idx] = Vec2(u, v);
			vertices[idx] = {
				Vec3(x * radius, y * radius, z * radius),
				Vec3(x, y, z),
				Vec2(u, v)
			};
		}
	}

	vertices[idx] = {
		Vec3(0.0f, -radius, 0.0f),
		Vec3(0.0f, -1.0f, 0.0f),
		Vec2(1.0f, 1.0f),
	};
	
	// genereate indices ---------------------------------------------------------------------------
	
	int index_count = (slices + 1) * 2 * stacks;
	u16 *indices = (u16 *)platform->alloc(index_count * sizeof(u16));
	
	idx = 0;
	
	
	// NOTE(nathan: top stacks
	for(int j = 0; j < slices; j++, idx+=2) {
		indices[idx] = j+1;
		indices[idx+1] = 0;
		
		
	}
	
	indices[idx] = 1;
	indices[idx+1] = 0;
	idx+=2;
	
	int offset = 0;
	
	// NOTE(nathan): middle stacks
	for(int i = 0; i < stacks-2; i++, idx+=2) {
		offset = 1+i*slices;
		for(int j = 0; j < slices; j++, idx+=2) {
			indices[idx] = offset + j + slices;
			indices[idx+1] = offset + j;
		}
		indices[idx] = offset + slices;
		indices[idx+1] = offset;
	}
	
	// NOTE(nathan): middle stack
	offset = 1 + (stacks - 2) * slices;
	for(int j = 0; j < slices; j++, idx+=2) {
		indices[idx] = vertex_count-1;
		indices[idx+1] = offset+j;
	}
	
	indices[idx] = vertex_count-1;
	indices[idx+1] = offset;

	/* Done creating vertices, release sin and cos tables */
	platform->free(sint1);
	platform->free(cost1);
	platform->free(sint2);
	platform->free(cost2); 
	result.vertices = vertices;
	result.vertex_count = vertex_count;
	result.indices = indices;
	result.index_count = index_count;

	result.vertex_buffer = render_context->createVertexBuffer(vertices, sizeof(Vertex), vertex_count);
	result.index_buffer = render_context->createVertexBuffer(indices, sizeof(u16), index_count, RenderContext::BufferType::Index);

	return result;
}