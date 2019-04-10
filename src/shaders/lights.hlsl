cbuffer PerScene : register(b0) {
	float4x4 projection;	
};

cbuffer PerObject : register(b1) {
	float4x4 model;
	float4 color;
	float constant_term;
	float linear_term;
	float quadratic_term;
	float padding;
};

struct VS_OUTPUT {
	float4 pos : SV_POSITION;
};

VS_OUTPUT  vs_main(float3 in_pos : POSITION, float3 in_normal : NORMAL, float2 in_uv : TEXCOORD) {
	VS_OUTPUT output;
	float4 world_pos = mul(float4(in_pos, 1.0f), model);
	output.pos = mul(world_pos, projection);
	return output;
}

sampler sampler0;
Texture2D tex_albedo_roughness : register(t0);
Texture2D tex_normal : register(t1);
Texture2D tex_position_metallic : register(t2);
Texture2D tex_irradiance_map : register(t3);
Texture2D tex_environment_map : register(t4);

float4 ps_main(VS_OUTPUT input) : SV_TARGET{
	float4 result = float4(1.0f, 0.0f, 0.0f, 1.0f);
	return result;
}