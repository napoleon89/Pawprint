cbuffer PerScene {
	float4x4 projection;	
};

struct VS_OUTPUT {
	float4 pos : SV_POSITION;
	float3 uv : TEXCOORD0;
};

sampler sampler0;
Texture2D cubemap;

VS_OUTPUT  vs_main(float3 in_pos : POSITION) {
	VS_OUTPUT output;
	float4 pos = mul(float4(in_pos, 1.0f), projection);
	output.pos = pos.xyww;
	output.uv = in_pos;
	return output;
}

static const float2 invAtan = float2(0.1591f, 0.3183f);
float2 sampleSphericalMap(float3 v)
{
    float2 uv = float2(atan2(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5f;
    return uv;
}

float4 ps_main(VS_OUTPUT input) : SV_TARGET  {
	return cubemap.Sample(sampler0, sampleSphericalMap(normalize(input.uv)));
}