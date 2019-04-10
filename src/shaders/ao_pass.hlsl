struct VS_OUTPUT {
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD0;
};

cbuffer PerScene : register(b0) {
	float4x4 projection;
	float2 screen_size;
	float kernel_radius;
	float kernel_bias;
	float4 kernel[64];
	int kernel_size;
	float padding[3];
};

sampler sampler0 : register(s0);
sampler wrap_sampler : register(s1);

Texture2D tex_normal : register(t0);
Texture2D tex_position : register(t1);
Texture2D tex_noise : register(t2);

VS_OUTPUT  vs_main(float3 in_pos : POSITION, float2 in_uv : TEXCOORD) {
	VS_OUTPUT output;
	output.pos = float4(in_pos.xy, 0.0f, 1.0f);
	output.uv = in_uv;
	return output;
}

float4 ps_main(VS_OUTPUT input) : SV_TARGET  {
	float2 ao_uv = float2(input.uv.x, input.uv.y);
	float2 noise_scale = float2(screen_size.x / 4.0f, screen_size.y / 4.0f);
	float3 world_pos = tex_position.Sample(sampler0, ao_uv).rgb;
	float3 normal = normalize(tex_normal.Sample(sampler0, ao_uv).rgb);
	// return float4(abs(normal), 1.0f);
	// normal.y = -normal.y;
	// world_pos.y = -world_pos.y;

	float3 random_vector = tex_noise.Sample(wrap_sampler, ao_uv * noise_scale).rgb;
	float3 tangent = normalize(random_vector - normal * dot(random_vector, normal));
	float3 bitangent = cross(normal, tangent);
	float3x3 tbn = float3x3(tangent, bitangent, normal);
	// return float4(abs(random_vector), 1.0f);
	
	float occlusion = 0.0f;
	for(int i = 0; i < kernel_size; i++) {
		float3 sample = mul(kernel[i].xyz, tbn);
		sample = world_pos + sample * kernel_radius;

		float4 offset = float4(sample, 0.0f);
		offset = mul(offset, projection);
		offset.y = -offset.y; // for some reason ?????? needs investigation
		
		offset.xy /= offset.w;
		offset.xy = offset.xy * 0.5f + 0.5f;

		float sample_depth = tex_position.Sample(sampler0, offset.xy).z;
		
		float range_check = abs(world_pos.z - sample_depth) < kernel_radius ? 1.0f : 0.0f;
		occlusion += (sample_depth <= sample.z + kernel_bias ? 1.0f : 0.0f) * range_check;
	}
	occlusion =  1.0f - (occlusion / kernel_size);
	return float4(occlusion, 0.0f, 0.0f, 1.0f);
}