cbuffer PerScene : register(b0) {
	float4x4 view_projection;	
};

cbuffer PerObject : register(b1) {
	float4x4 model;
	float4 color;
};

struct VS_OUTPUT {
	float4 pos : SV_POSITION;
};

VS_OUTPUT  vs_main(float3 in_pos : POSITION) {
	VS_OUTPUT output;
	float4 world_space = mul(float4(in_pos, 1.0f), model);
	float4 pos = mul(world_space, view_projection);
	output.pos = pos;
	return output;
}

float4 ps_main(VS_OUTPUT input) : SV_TARGET  {
	return color;
}