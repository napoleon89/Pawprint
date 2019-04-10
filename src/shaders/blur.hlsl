struct VS_OUTPUT {
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD0;
};

cbuffer PerScene : register(b0) {
	int blur_amount;
	float padding[3];
};

sampler sampler0 : register(s0);

Texture2D main_texture : register(t0);

VS_OUTPUT  vs_main(float3 in_pos : POSITION, float2 in_uv : TEXCOORD) {
	VS_OUTPUT output;
	output.pos = float4(in_pos.xy, 0.0f, 1.0f);
	output.uv = in_uv;
	return output;
}

float4 ps_main(VS_OUTPUT input) : SV_TARGET  {
	float2 texture_size;
	float level;
	main_texture.GetDimensions(0, texture_size.x, texture_size.y, level);
	float2 texel_size = 1.0f / texture_size;
	float hlim_x = float(-blur_amount) * 0.5f + 0.5f;
	float2 hlim = float2(hlim_x, hlim_x);
	float result = 0.0f;
	for(int i = 0; i < blur_amount; ++i) {
		for(int j = 0; j < blur_amount; ++j) {
			float2 offset = (hlim + float2(float(j), float(i))) * texel_size;
			result += main_texture.Sample(sampler0, input.uv + offset).r;
		}
	}

	return float4(result / float(blur_amount * blur_amount), 0.0f, 0.0f, 1.0f);
}