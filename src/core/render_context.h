#ifndef RENDER_CONTEXT_H
#define RENDER_CONTEXT_H

#include <engine/std.h>
#include <string>
#include <core/platform.h>

struct PlatformPacket;
struct PlatformRenderState;

struct Shader {
	void *vertex_shader;
	void *pixel_shader;
	char *vertex_src;
	char *pixel_src;	
	u64 vertex_src_size;
	u64 pixel_src_size;
};

struct ShaderLayout {
	void *layout;	
};

struct ShaderConstant {
	void *buffer;	
};

struct VertexBuffer {
	void *buffer;
	u32 vertex_size;	
};

struct RasterState {
	void *state;
};

struct BlendState {
	void *state;	
};

struct DepthStencilState {
	void *state;	
};

struct Texture2D {
	void *texture;
	void *texture_handle;
	u32 width;
	u32 height;
};

typedef Texture2D TextureCube;

struct Sampler {
	void *sampler;	
};

struct RenderTexture {
	Texture2D texture;
	void *render_texture;
};

struct DepthStencilTexture {
	Texture2D texture;
	void *depth_stencil;	
};

struct RenderContext {
	PlatformPacket *pp;
	
	enum class Format {
		Vec2,
		Vec3,
		u32_unorm,
		u32,
		u16,
		Vec4,
		Depth24Stencil8,
		Vec4_16,
		u8_unorm,
		Int4,
		MAX
	};
	
	static u32 format_values[(u32)Format::MAX];
	static u32 format_sizes[(u32)Format::MAX];
	
	struct LayoutElement {
		char *name;
		Format format;
		u32 input_slot;
	};
	
	enum class Topology {
		TriangleList,
		TriangleStrip,
		MAX
	};
	
	static u32 topologies[(u32)Topology::MAX];
	
	enum class BufferType {
		Vertex,
		Index,
		MAX
	};
	
	static u32 buffer_types[(u32)BufferType::MAX];
	
	virtual void setClipRect(s32 x, s32 y, s32 w, s32 h);
	virtual void setViewport(s32 x, s32 y, s32 width, s32 height, f32 min_depth, f32 max_depth);
	virtual void  resizeBuffer(s32 width, s32 height);
	virtual void init(s32 width, s32 height, s32 refresh_rate, PlatformWindow *window);
	virtual void uninit();
	virtual void clear(float color[4]);
	virtual void present();
	virtual void bindDefaultTextures(bool depth = true, DepthStencilTexture *depth_texture = 0);
	
	
	virtual Texture2D createTexture2D(void *data, u32 width, u32 height, Format format, bool render_texture = false, bool depth = false, int sample_count = 1);
	virtual TextureCube createTextureCube(u8 **data, u32 width, u32 height, Format format);
	virtual void bindTexture2D(Texture2D *texture, u32 slot);
	virtual void destroyTexture2D(Texture2D *texture);
	
	virtual RenderTexture createRenderTexture(u32 width, u32 height, Format format, int sample_count = 1);
	virtual void destroyRenderTexture(RenderTexture *render_texture);
	virtual DepthStencilTexture createDepthStencilTexture(u32 width, u32 height, int sample_count = 1);
	virtual void destroyDepthStencilTexture(DepthStencilTexture *texture);
	virtual void bindRenderTextures(Platform *platform, RenderTexture **rts, u32 count, DepthStencilTexture *dst, bool use_default_depth = false);
	virtual void clearRenderTexture(RenderTexture *rt, float color[4]);
	virtual void clearDepthStencilTexture(DepthStencilTexture *dst, float value);
	
	virtual Sampler createSampler(bool repeat = false);
	virtual void bindSampler(Sampler *sampler, u32 slot);
	virtual void destroySampler(Sampler *sampler);
	
	virtual Shader createShader(char *vertex_ptr, u64 vertex_size, char *pixel_ptr, u64 pixel_size);
	virtual void destroyShader(Shader *shader);
	virtual void bindShader(Shader *shader);
	virtual void unbindShader();
	
	virtual ShaderLayout createShaderLayout(RenderContext::LayoutElement *elements, u32 count, Shader *shader, bool inc_input_slot = false, bool inc_byte_stride = true);
	virtual void bindShaderLayout(ShaderLayout *constant);
	
	virtual ShaderConstant createShaderConstant(u32 buffer_size);
	virtual void updateShaderConstant(ShaderConstant *constant, void *data);
	virtual void bindShaderConstant(ShaderConstant *constant, s32 vs_loc, s32 ps_loc);
	
	virtual VertexBuffer createVertexBuffer(void *vertices, u32 vertex_size, u32 num_vertices, BufferType type = BufferType::Vertex);
	virtual void destroyVertexBuffer(VertexBuffer *vb);
	virtual void bindVertexBuffer(VertexBuffer *vb, u32 slot);
	virtual void bindIndexBuffer(VertexBuffer *vb, Format format);
	
	virtual RasterState createRasterState(bool scissor_enabled, bool depth_enabled, bool cull_enabled = true, bool wireframe = false, bool msaa_enabled = false);
	virtual void bindRasterState(RasterState *state);
	virtual void destroyRasterState(RasterState *state);
	
	virtual BlendState createBlendState();
	virtual void bindBlendState(BlendState *state, const float factor[4], u32 mask);
	virtual void destroyBlendState(BlendState *state);
	
	virtual DepthStencilState createDepthStencilState(bool enabled = true);
	virtual void bindDepthStencilState(DepthStencilState *state);
	virtual void destroyDepthStencilState(DepthStencilState *state);

	virtual PlatformRenderState *saveRenderState();
	virtual void reloadRenderState(PlatformRenderState *state);
	virtual void destroyRenderState(PlatformRenderState *state);
	
	virtual void sendDraw(Topology topology, u32 num_vertices);
	virtual void sendDrawIndexed(Topology topology, u32 num_indices, int vertex_offset = 0, int index_offset = 0);
};

#endif // RENDER_CONTEXT_H