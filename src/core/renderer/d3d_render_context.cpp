#include <d3d11.h>
#include <dxgi.h>
#include <engine/std.h>
#include <SDL2/SDL_syswm.h>
#include <core/render_context.h>

struct PlatformPacket {
	IDXGISwapChain *swap_chain;
	ID3D11Device *device;
	ID3D11DeviceContext *device_context;
	ID3D11RenderTargetView *render_target_view;
	ID3D11Debug *debug_interface;
	ID3D11DepthStencilView *depth_stencil_view;
	ID3D11Texture2D *depth_stencil_buffer;
};

struct PlatformRenderState {
	 UINT                        ScissorRectsCount, ViewportsCount;
    D3D11_RECT                  ScissorRects[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
    D3D11_VIEWPORT              Viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
    ID3D11RasterizerState*      RS;
    ID3D11BlendState*           BlendState;
    FLOAT                       BlendFactor[4];
    UINT                        SampleMask;
    UINT                        StencilRef;
    ID3D11DepthStencilState*    DepthStencilState;
    ID3D11ShaderResourceView*   PSShaderResource;
    ID3D11SamplerState*         PSSampler;
    ID3D11PixelShader*          PS;
    ID3D11VertexShader*         VS;
    UINT                        PSInstancesCount, VSInstancesCount;
    ID3D11ClassInstance*        PSInstances[256], *VSInstances[256];   // 256 is max according to PSSetShader documentation
    D3D11_PRIMITIVE_TOPOLOGY    PrimitiveTopology;
    ID3D11Buffer*               IndexBuffer, *VertexBuffer, *VSConstantBuffer;
    UINT                        IndexBufferOffset, VertexBufferStride, VertexBufferOffset;
    DXGI_FORMAT                 IndexBufferFormat;
    ID3D11InputLayout*          InputLayout;
};

void RenderContext::setClipRect(s32 x, s32 y, s32 w, s32 h) {
	D3D11_RECT r = {x, y, w, h};
	pp->device_context->RSSetScissorRects(1, &r);
}

void RenderContext::setViewport(s32 x, s32 y, s32 width, s32 height, f32 min_depth, f32 max_depth) {
	D3D11_VIEWPORT viewport = { (f32)x, (f32)y, (f32)width, (f32)height, min_depth, max_depth };
	pp->device_context->RSSetViewports(1, &viewport);
}

void RenderContext::resizeBuffer(s32 width, s32 height) {
	pp->device_context->OMSetRenderTargets(0, 0, 0);
	if(pp->render_target_view)
		pp->render_target_view->Release();
	if(pp->depth_stencil_view) 
		pp->depth_stencil_view->Release();
	HRESULT error = pp->swap_chain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
	
	ID3D11Texture2D *back_buffer = 0;
	error = pp->swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **)&back_buffer);
	
	error = pp->device->CreateRenderTargetView(back_buffer, 0, &pp->render_target_view);
	back_buffer->Release();
	
	
	D3D11_TEXTURE2D_DESC depth_stencil_desc;
	depth_stencil_desc.Width     = width;
	depth_stencil_desc.Height    = height;
	depth_stencil_desc.MipLevels = 1;
	depth_stencil_desc.ArraySize = 1;
	depth_stencil_desc.Format    = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depth_stencil_desc.SampleDesc.Count   = 1;
	depth_stencil_desc.SampleDesc.Quality = 0;
	depth_stencil_desc.Usage          = D3D11_USAGE_DEFAULT;
	depth_stencil_desc.BindFlags      = D3D11_BIND_DEPTH_STENCIL;
	depth_stencil_desc.CPUAccessFlags = 0; 
	depth_stencil_desc.MiscFlags      = 0;
	pp->device->CreateTexture2D(&depth_stencil_desc, 0, &pp->depth_stencil_buffer);
	pp->device->CreateDepthStencilView(pp->depth_stencil_buffer, 0, &pp->depth_stencil_view);
	pp->depth_stencil_buffer->Release();
	
	pp->device_context->OMSetRenderTargets(1, &pp->render_target_view, pp->depth_stencil_view);
}

void RenderContext::init(s32 width, s32 height, s32 refresh_rate, PlatformWindow *window) {
	pp = (PlatformPacket *)malloc(sizeof(PlatformPacket));
	memset(pp, 0, sizeof(PlatformPacket));
	DXGI_MODE_DESC buffer_desc = {0};
	buffer_desc.Width = width;
	buffer_desc.Height = height;
	buffer_desc.RefreshRate.Numerator = refresh_rate;
	buffer_desc.RefreshRate.Denominator = 1;
	buffer_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	buffer_desc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	buffer_desc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED  ;
	
	SDL_SysWMinfo win32_info = {0};
	SDL_GetWindowWMInfo((SDL_Window *)window->handle, &win32_info);
	
	DXGI_SWAP_CHAIN_DESC swap_chain_desc = {0};
	swap_chain_desc.BufferDesc = buffer_desc;
	swap_chain_desc.SampleDesc.Count =  1;
	swap_chain_desc.SampleDesc.Quality =  0;
	swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swap_chain_desc.BufferCount = 1; // NOTE(nathan): double buffering
	swap_chain_desc.OutputWindow = win32_info.info.win.window;
	swap_chain_desc.Windowed = true;
	swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	swap_chain_desc.Flags = D3D11_CREATE_DEVICE_DEBUG; // NOTE(nathan): possible DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH
	
	HRESULT error = D3D11CreateDeviceAndSwapChain(0, D3D_DRIVER_TYPE_HARDWARE, 0, 0, 0, 0, D3D11_SDK_VERSION, 
														  &swap_chain_desc, &pp->swap_chain, &pp->device, 0, &pp->device_context);
	
	
	// NOTE(nathan): disable alt-enter fullscreen
	IDXGIFactory1 *factory = 0;
	if(SUCCEEDED(pp->swap_chain->GetParent(__uuidof(IDXGIFactory1), (void **)&factory))) {
		factory->MakeWindowAssociation(win32_info.info.win.window, DXGI_MWA_NO_ALT_ENTER|DXGI_MWA_NO_PRINT_SCREEN );
		factory->Release();
	}
	
	resizeBuffer(width, height);
	
	error = pp->device->QueryInterface(IID_PPV_ARGS(&pp->debug_interface));
}

void RenderContext::uninit() {
	if(pp->swap_chain) pp->swap_chain->Release();
	if(pp->device) pp->device->Release();
	if(pp->device_context) pp->device_context->Release();
	if(pp->render_target_view) pp->render_target_view->Release();
	if(pp->debug_interface) pp->debug_interface->Release();
	if(pp->depth_stencil_view) pp->depth_stencil_view->Release();
}

void RenderContext::clear(float color[4]) {
	
	pp->device_context->ClearDepthStencilView(pp->depth_stencil_view, D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1.0f, 0);
	pp->device_context->ClearRenderTargetView(pp->render_target_view, color); 
}

void RenderContext::present() {
	pp->swap_chain->Present(0, 0);
}

void RenderContext::bindDefaultTextures(bool depth, DepthStencilTexture *depth_texture) {
	ID3D11DepthStencilView *depth_ptr = 0;
	if(depth) depth_ptr = pp->depth_stencil_view;
	if(depth_texture != 0) depth_ptr = (ID3D11DepthStencilView *)depth_texture->depth_stencil;
	pp->device_context->OMSetRenderTargets(1, &pp->render_target_view, depth_ptr);
}

Shader RenderContext::createShader(char *vertex_ptr, u64 vertex_size, char *pixel_ptr, u64 pixel_size) {
	Shader result = {};
	
	result.vertex_src = vertex_ptr;
	result.pixel_src = pixel_ptr;

	result.vertex_src_size = vertex_size;
	result.pixel_src_size = pixel_size;
	
	HRESULT error;
	error = pp->device->CreateVertexShader(vertex_ptr, vertex_size, 0, (ID3D11VertexShader **)(&result.vertex_shader));
	error = pp->device->CreatePixelShader(pixel_ptr, pixel_size, 0, (ID3D11PixelShader **)(&result.pixel_shader));
	return result;
}

void RenderContext::destroyShader(Shader *shader) {
	((ID3D11VertexShader *)shader->vertex_shader)->Release();
	((ID3D11PixelShader *)shader->pixel_shader)->Release();
	SDL_free(shader->vertex_src);
	SDL_free(shader->pixel_src);
}

void RenderContext::bindShader(Shader *shader) {
	pp->device_context->VSSetShader((ID3D11VertexShader *)shader->vertex_shader, 0, 0);
	pp->device_context->PSSetShader((ID3D11PixelShader *)shader->pixel_shader, 0, 0);
}

void RenderContext::unbindShader() {
	pp->device_context->VSSetShader(0, 0, 0);
	pp->device_context->PSSetShader(0, 0, 0);
}

u32 RenderContext::format_values[(u32)RenderContext::Format::MAX] = {
	DXGI_FORMAT_R32G32_FLOAT,
	DXGI_FORMAT_R32G32B32_FLOAT,
	DXGI_FORMAT_R8G8B8A8_UNORM,
	DXGI_FORMAT_R32_UINT,
	DXGI_FORMAT_R16_UINT,
	DXGI_FORMAT_R32G32B32A32_FLOAT,
	DXGI_FORMAT_D24_UNORM_S8_UINT,
	DXGI_FORMAT_R16G16B16A16_FLOAT,
	DXGI_FORMAT_R8_UNORM,
	DXGI_FORMAT_R8G8B8A8_SINT,
};

u32 RenderContext::format_sizes[(u32)RenderContext::Format::MAX] = {
	8,
	12,
	4,
	4,
	2,
	16,
	4,
	8,
	1,
	16,
};

ShaderLayout RenderContext::createShaderLayout(RenderContext::LayoutElement *elements, u32 count, Shader *shader, bool inc_input_slot, bool inc_byte_stride) {
	// D3D11_INPUT_ELEMENT_DESC final_layout_desc[2] = {
	// 	"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0,
	// 	"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0
	// };
	ShaderLayout result = {};
	D3D11_INPUT_ELEMENT_DESC *final_layout_desc = (D3D11_INPUT_ELEMENT_DESC *)malloc(sizeof(D3D11_INPUT_ELEMENT_DESC) * count);
	u32 counter = 0;
	u32 input_slot = 0;
	for(u32 i = 0; i < count; i++) {
		RenderContext::LayoutElement element = elements[i];
		u32 byte_stride = 0;
		if(inc_byte_stride) byte_stride = counter;
		u32 is = 0;
		if(inc_input_slot) is = input_slot;
		D3D11_INPUT_ELEMENT_DESC desc = {element.name, 0, (DXGI_FORMAT)format_values[(u32)element.format], is, byte_stride, D3D11_INPUT_PER_VERTEX_DATA, 0};
		final_layout_desc[i] = desc;
		counter += format_sizes[(u32)element.format];
		input_slot++;
	}
	
	pp->device->CreateInputLayout(final_layout_desc, count, shader->vertex_src, shader->vertex_src_size, (ID3D11InputLayout  **)&result.layout);
	free(final_layout_desc);
	return result;
}

void RenderContext::bindShaderLayout(ShaderLayout *layout) {
	pp->device_context->IASetInputLayout((ID3D11InputLayout *)layout->layout);	
}

ShaderConstant RenderContext::createShaderConstant(u32 buffer_size) { 
	ShaderConstant result = {};
	D3D11_BUFFER_DESC desc = {0};
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.ByteWidth = buffer_size;
	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	
	HRESULT error = pp->device->CreateBuffer(&desc, 0, (ID3D11Buffer **)&result.buffer);
	
	return result;
}

void RenderContext::updateShaderConstant(ShaderConstant *constant, void *data) {
	pp->device_context->UpdateSubresource((ID3D11Buffer *)constant->buffer, 0, 0, data, 0, 0);
}

void RenderContext::bindShaderConstant(ShaderConstant *constant, s32 vs_loc, s32 ps_loc) {
	if(vs_loc != -1) pp->device_context->VSSetConstantBuffers(vs_loc, 1, (ID3D11Buffer **)&constant->buffer);	
	if(ps_loc != -1) pp->device_context->PSSetConstantBuffers(ps_loc, 1, (ID3D11Buffer **)&constant->buffer);		
}



u32 RenderContext::buffer_types[(u32)BufferType::MAX] = {
	D3D11_BIND_VERTEX_BUFFER,
	D3D11_BIND_INDEX_BUFFER
};

VertexBuffer RenderContext::createVertexBuffer(void *vertices, u32 vertex_size, u32 num_vertices, RenderContext::BufferType type) {
	VertexBuffer result = {};
	result.vertex_size = vertex_size;
	D3D11_BUFFER_DESC vertex_buffer_desc = {0};
	vertex_buffer_desc.ByteWidth = vertex_size * num_vertices;
	vertex_buffer_desc.Usage = D3D11_USAGE_DEFAULT;
	vertex_buffer_desc.BindFlags = buffer_types[(u32)type];
	
	D3D11_SUBRESOURCE_DATA vertex_buffer_data = {0};
	vertex_buffer_data.pSysMem = vertices;
	HRESULT error = pp->device->CreateBuffer(&vertex_buffer_desc, &vertex_buffer_data, (ID3D11Buffer **)&result.buffer);
	return result;
}

void RenderContext::destroyVertexBuffer(VertexBuffer *vb) {
	if(vb->buffer != 0) {
		((ID3D11Buffer *)vb->buffer)->Release();	
	}
	
}

void RenderContext::bindVertexBuffer(VertexBuffer *vb, u32 slot) {
	u32 stride = vb->vertex_size;
	u32 offset = 0;
	pp->device_context->IASetVertexBuffers(slot, 1, (ID3D11Buffer **)&vb->buffer, &stride, &offset);
}

void RenderContext::bindIndexBuffer(VertexBuffer *vb, Format format) {
	pp->device_context->IASetIndexBuffer((ID3D11Buffer *)vb->buffer, (DXGI_FORMAT)format_values[(u32)format], 0);	
}

u32 RenderContext::topologies[(u32)RenderContext::Topology::MAX] = {
	D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
	D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
};

void RenderContext::sendDraw(Topology topology, u32 num_vertices) {
	pp->device_context->IASetPrimitiveTopology((D3D11_PRIMITIVE_TOPOLOGY)topologies[(u32)topology]);
	pp->device_context->Draw(num_vertices, 0);
}

void RenderContext::sendDrawIndexed(Topology topology, u32 num_indices, int vertex_offset, int index_offset) {
	pp->device_context->IASetPrimitiveTopology((D3D11_PRIMITIVE_TOPOLOGY)topologies[(u32)topology]);
	pp->device_context->DrawIndexed(num_indices, index_offset, vertex_offset);
}
			
			
RasterState RenderContext::createRasterState(bool scissor_enabled, bool depth_enabled, bool cull_enabled, bool wireframe, bool msaa_enabled) {
	RasterState result = {};
	D3D11_RASTERIZER_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.FillMode = wireframe ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID;
	desc.CullMode = cull_enabled ? D3D11_CULL_BACK : D3D11_CULL_NONE;
	desc.ScissorEnable = scissor_enabled;
	desc.DepthClipEnable = depth_enabled;
	desc.MultisampleEnable = msaa_enabled;
	pp->device->CreateRasterizerState(&desc, (ID3D11RasterizerState **)&result.state);
	return result;
}

void RenderContext::bindRasterState(RasterState *state) {
	pp->device_context->RSSetState((ID3D11RasterizerState *)state->state);
}

void RenderContext::destroyRasterState(RasterState *state) {
	((ID3D11RasterizerState *)state->state)->Release();
}

BlendState RenderContext::createBlendState() {
	BlendState result = {};
	D3D11_BLEND_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.AlphaToCoverageEnable = false;
	desc.RenderTarget[0].BlendEnable = true;
	desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
	desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
	desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	pp->device->CreateBlendState(&desc, (ID3D11BlendState **)&result.state);
	
	return result;
}

void RenderContext::bindBlendState(BlendState *state, const float factor[4], u32 mask) {
	pp->device_context->OMSetBlendState((ID3D11BlendState *)state->state, factor, mask);
}

void RenderContext::destroyBlendState(BlendState *state) {
	((ID3D11BlendState *)state->state)->Release();
}


DepthStencilState RenderContext::createDepthStencilState(bool enabled) {
	DepthStencilState result = {};
	D3D11_DEPTH_STENCIL_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.DepthEnable = enabled;
	desc.DepthWriteMask = enabled ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
	desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	desc.StencilEnable = false;
	desc.FrontFace.StencilFailOp = desc.FrontFace.StencilDepthFailOp = desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	desc.FrontFace.StencilFunc = D3D11_COMPARISON_NEVER;
	desc.BackFace = desc.FrontFace;
	pp->device->CreateDepthStencilState(&desc, (ID3D11DepthStencilState **)&result.state);
	return result;
}

void RenderContext::bindDepthStencilState(DepthStencilState *state) {
	ID3D11DepthStencilState *bind_state = 0;
	if(state != 0) bind_state = (ID3D11DepthStencilState *)state->state;
	pp->device_context->OMSetDepthStencilState(bind_state, 0);
}

void RenderContext::destroyDepthStencilState(DepthStencilState *state) {
	((ID3D11BlendState *)state->state)->Release();
}

Texture2D RenderContext::createTextureCube(u8 **data, u32 width, u32 height, Format format) {
	Texture2D result = {};
	result.width = width;
	result.height = height;
	DXGI_FORMAT f = (DXGI_FORMAT)format_values[(u32)format];
	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = 1;
	desc.ArraySize = 6;
	desc.Format = f;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
	
	ID3D11Texture2D *pTexture = NULL;
	D3D11_SUBRESOURCE_DATA subResource[6];
	
	for(u8 i = 0; i < 6; i++) {
		subResource[i].pSysMem = (void *)data[i];
		subResource[i].SysMemPitch = width * format_sizes[(u32)format];
		subResource[i].SysMemSlicePitch = 0;
	}

	HRESULT error = pp->device->CreateTexture2D(&desc, &subResource[0], &pTexture);
	
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(srvDesc));
	srvDesc.Format = f;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.Texture2D.MipLevels = desc.MipLevels;
	srvDesc.Texture2D.MostDetailedMip = 0;
	error = pp->device->CreateShaderResourceView(pTexture, &srvDesc, (ID3D11ShaderResourceView **)&result.texture);
	
	
	result.texture_handle = (void *)pTexture;
	
	return result;	
}

Texture2D RenderContext::createTexture2D(void *data, u32 width, u32 height, Format format, bool render_texture, bool depth, int sample_count) {
	Texture2D result = {};
	result.width = width;
	result.height = height;
	DXGI_FORMAT f = (DXGI_FORMAT)format_values[(u32)format];
	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = f;
	desc.SampleDesc.Count = sample_count;
	desc.Usage = D3D11_USAGE_DEFAULT;
	u32 flags = D3D11_BIND_SHADER_RESOURCE;
	if(render_texture) flags |= D3D11_BIND_RENDER_TARGET;
	if(depth) flags = D3D11_BIND_DEPTH_STENCIL;
	desc.BindFlags = flags;
	desc.CPUAccessFlags = 0;
	
	ID3D11Texture2D *pTexture = NULL;
	D3D11_SUBRESOURCE_DATA subResource;
	subResource.pSysMem = data;
	subResource.SysMemPitch = width * format_sizes[(u32)format];
	subResource.SysMemSlicePitch = 0;
	
	D3D11_SUBRESOURCE_DATA *sr = &subResource;
	if(render_texture || depth) sr = 0;
	
	HRESULT error = pp->device->CreateTexture2D(&desc, sr, &pTexture);
	
	if(!depth) {
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory(&srvDesc, sizeof(srvDesc));
		srvDesc.Format = f;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = desc.MipLevels;
		srvDesc.Texture2D.MostDetailedMip = 0;
		error = pp->device->CreateShaderResourceView(pTexture, &srvDesc, (ID3D11ShaderResourceView **)&result.texture);
		
	}
	
	result.texture_handle = (void *)pTexture;
	
	return result;	
}

void RenderContext::bindTexture2D(Texture2D *texture, u32 slot) {
	// ID3D11ShaderResourceView* ptr[1] = {nullptr};
	// if(texture != 0) 
		// ptr[0] = (ID3D11ShaderResourceView*)&texture->texture;
	pp->device_context->PSSetShaderResources(slot, 1, (ID3D11ShaderResourceView**)&texture->texture);
}

void RenderContext::destroyTexture2D(Texture2D *texture) {
	((ID3D11ShaderResourceView *)texture->texture)->Release();
}

RenderTexture RenderContext::createRenderTexture(u32 width, u32 height, Format format, int sample_count) {
	RenderTexture result = {};
	result.texture = createTexture2D(0, width, height, format, true, false, sample_count);
	ID3D11RenderTargetView *rtv = 0;
	ID3D11Texture2D *texture = (ID3D11Texture2D *)result.texture.texture_handle;
	
	D3D11_RENDER_TARGET_VIEW_DESC rtv_desc = {};
	rtv_desc.Format = (DXGI_FORMAT)format_values[(u32)format];
	rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	rtv_desc.Texture2D.MipSlice = 0;
	
	HRESULT error = pp->device->CreateRenderTargetView(texture, &rtv_desc, &rtv);
	result.render_texture = (void *)rtv;
	
	return result;
}

void RenderContext::destroyRenderTexture(RenderTexture *render_texture) {
	if(render_texture->texture.texture != 0)
		destroyTexture2D(&render_texture->texture);
	if(render_texture->render_texture != 0)
		((ID3D11RenderTargetView *)render_texture->render_texture)->Release();
}

DepthStencilTexture RenderContext::createDepthStencilTexture(u32 width, u32 height, int sample_count) {
	DepthStencilTexture result = {};
	result.texture = createTexture2D(0, width, height, Format::Depth24Stencil8, false, true, sample_count);
	ID3D11Texture2D *texture = (ID3D11Texture2D *)result.texture.texture_handle;
	ID3D11DepthStencilView *dsv = 0;
	pp->device->CreateDepthStencilView(texture, 0, &dsv);
	result.depth_stencil = (void *)dsv;
	return result;
}

void RenderContext::destroyDepthStencilTexture(DepthStencilTexture *texture) {
	if(texture->texture.texture != 0)
		destroyTexture2D(&texture->texture);
	if(texture->depth_stencil != 0)
		((ID3D11DepthStencilView *)texture->depth_stencil)->Release();
}

void RenderContext::bindRenderTextures(Platform *platform, RenderTexture **rts, u32 count, DepthStencilTexture *dst, bool use_default_depth) {
	ID3D11RenderTargetView **rtvs = (ID3D11RenderTargetView **)platform->alloc(sizeof(ID3D11RenderTargetView *) * count);
	
	for(u32 i = 0; i < count; i++) {
		RenderTexture *rt = rts[i];
		rtvs[i] = (ID3D11RenderTargetView *)rt->render_texture;
	}
	ID3D11DepthStencilView *dsv = 0;
	if(dst != 0) 
		dsv = (ID3D11DepthStencilView *)dst->depth_stencil;

	if(use_default_depth) dsv = pp->depth_stencil_view;
	
	pp->device_context->OMSetRenderTargets(count, rtvs, dsv);
	platform->free(rtvs);
}

void RenderContext::clearRenderTexture(RenderTexture *rt, float color[4]) {
	ID3D11RenderTargetView *rtv = (ID3D11RenderTargetView *)rt->render_texture;
	pp->device_context->ClearRenderTargetView(rtv, color); 
}

void RenderContext::clearDepthStencilTexture(DepthStencilTexture *dst, float value){
	ID3D11DepthStencilView *dsv = (ID3D11DepthStencilView *)dst->depth_stencil;
	pp->device_context->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, value, 0);
}

Sampler RenderContext::createSampler(bool repeat) {
	Sampler result = {};
	D3D11_TEXTURE_ADDRESS_MODE address_mode = repeat ? D3D11_TEXTURE_ADDRESS_WRAP : D3D11_TEXTURE_ADDRESS_CLAMP;
	D3D11_SAMPLER_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	desc.AddressU = address_mode;
	desc.AddressV = address_mode;
	desc.AddressW = address_mode;
	desc.MipLODBias = 0.f;
	desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	desc.MinLOD = 0.f;
	desc.MaxLOD = 0.f;
	pp->device->CreateSamplerState(&desc, (ID3D11SamplerState **)&result.sampler);
	
	return result;	
}

void RenderContext::bindSampler(Sampler *sampler, u32 slot) {
	pp->device_context->PSSetSamplers(slot, 1, (ID3D11SamplerState **)&sampler->sampler);
}

void RenderContext::destroySampler(Sampler *sampler) {
	((ID3D11SamplerState *)sampler->sampler)->Release();	
}

PlatformRenderState *RenderContext::saveRenderState() {
	PlatformRenderState *state = (PlatformRenderState *)malloc(sizeof(PlatformRenderState));
		
	state->ScissorRectsCount = state->ViewportsCount = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    pp->device_context->RSGetScissorRects(&state->ScissorRectsCount, state->ScissorRects);
    pp->device_context->RSGetViewports(&state->ViewportsCount, state->Viewports);
    pp->device_context->RSGetState(&state->RS);
    pp->device_context->OMGetBlendState(&state->BlendState, state->BlendFactor, &state->SampleMask);
    pp->device_context->OMGetDepthStencilState(&state->DepthStencilState, &state->StencilRef);
    pp->device_context->PSGetShaderResources(0, 1, &state->PSShaderResource);
    pp->device_context->PSGetSamplers(0, 1, &state->PSSampler);
    state->PSInstancesCount = state->VSInstancesCount = 256;
    pp->device_context->PSGetShader(&state->PS, state->PSInstances, &state->PSInstancesCount);
    pp->device_context->VSGetShader(&state->VS, state->VSInstances, &state->VSInstancesCount);
    pp->device_context->VSGetConstantBuffers(0, 1, &state->VSConstantBuffer);
    pp->device_context->IAGetPrimitiveTopology(&state->PrimitiveTopology);
    pp->device_context->IAGetIndexBuffer(&state->IndexBuffer, &state->IndexBufferFormat, &state->IndexBufferOffset);
    pp->device_context->IAGetVertexBuffers(0, 1, &state->VertexBuffer, &state->VertexBufferStride, &state->VertexBufferOffset);
    pp->device_context->IAGetInputLayout(&state->InputLayout);
	return state;	
}

void RenderContext::reloadRenderState(PlatformRenderState *state) {
	pp->device_context->RSSetScissorRects(state->ScissorRectsCount, state->ScissorRects);
    pp->device_context->RSSetViewports(state->ViewportsCount, state->Viewports);
    pp->device_context->RSSetState(state->RS); if (state->RS) state->RS->Release();
    pp->device_context->OMSetBlendState(state->BlendState, state->BlendFactor, state->SampleMask); if (state->BlendState) state->BlendState->Release();
    pp->device_context->OMSetDepthStencilState(state->DepthStencilState, state->StencilRef); if (state->DepthStencilState) state->DepthStencilState->Release();
    pp->device_context->PSSetShaderResources(0, 1, &state->PSShaderResource); if (state->PSShaderResource) state->PSShaderResource->Release();
    pp->device_context->PSSetSamplers(0, 1, &state->PSSampler); if (state->PSSampler) state->PSSampler->Release();
    pp->device_context->PSSetShader(state->PS, state->PSInstances, state->PSInstancesCount); if (state->PS) state->PS->Release();
    for (UINT i = 0; i < state->PSInstancesCount; i++) if (state->PSInstances[i]) state->PSInstances[i]->Release();
    pp->device_context->VSSetShader(state->VS, state->VSInstances, state->VSInstancesCount); if (state->VS) state->VS->Release();
    pp->device_context->VSSetConstantBuffers(0, 1, &state->VSConstantBuffer); if (state->VSConstantBuffer) state->VSConstantBuffer->Release();
    for (UINT i = 0; i < state->VSInstancesCount; i++) if (state->VSInstances[i]) state->VSInstances[i]->Release();
    pp->device_context->IASetPrimitiveTopology(state->PrimitiveTopology);
    pp->device_context->IASetIndexBuffer(state->IndexBuffer, state->IndexBufferFormat, state->IndexBufferOffset); if (state->IndexBuffer) state->IndexBuffer->Release();
    pp->device_context->IASetVertexBuffers(0, 1, &state->VertexBuffer, &state->VertexBufferStride, &state->VertexBufferOffset); if (state->VertexBuffer) state->VertexBuffer->Release();
    pp->device_context->IASetInputLayout(state->InputLayout); if (state->InputLayout) state->InputLayout->Release();
}

void RenderContext::destroyRenderState(PlatformRenderState *state) {
	free(state);
}
