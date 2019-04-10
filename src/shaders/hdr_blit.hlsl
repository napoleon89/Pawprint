struct VS_OUTPUT {
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD0;
};

cbuffer PerScene : register(b0) {
	float4 exposure;
};

sampler sampler0 : register(s0);

Texture2D tex_albedo : register(t0);

VS_OUTPUT  vs_main(float3 in_pos : POSITION, float2 in_uv : TEXCOORD) {
	VS_OUTPUT output;
	output.pos = float4(in_pos.xy, 0.0f, 1.0f);
	output.uv = in_uv;
	return output;
}

float4 ps_main(VS_OUTPUT input) : SV_TARGET  {
	
	const float gamma = 2.2;
	float3 final_color = tex_albedo.Sample(sampler0, input.uv).rgb;
	// return float4(final_color, 1.0f);
	float3 mapped = float3(1.0f, 1.0f, 1.0f) - exp(-final_color * exposure.x);
	float d = 1.0f / gamma;
	mapped = pow(mapped, float3(d, d, d));
	return float4(mapped, 1.0f);
}