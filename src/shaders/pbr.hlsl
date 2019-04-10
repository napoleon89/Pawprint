struct Light {
	float4 position;
	float4 color;
};

cbuffer PerScene {
	float4x4 projection;	
	float4 cam_pos;
	Light lights[4];
};

struct VS_OUTPUT {
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD;
};

VS_OUTPUT  vs_main(float3 in_pos : POSITION, float2 in_uv : TEXCOORD) {
	VS_OUTPUT output;
	output.pos = mul(float4(in_pos, 1.0f), projection);
	output.uv = in_uv;
	return output;
}

static const float PI = 3.14159265359f;

float3 fresnelSchlick(float cos_theta, float3 f0) {
	return f0 + (1.0f - f0) * pow(1.0f - cos_theta, 5.0f);
}

float distributionGGX(float3 N, float3 H, float roughness) {
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0f);
    float NdotH2 = NdotH*NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
    denom = PI * denom * denom;
	
    return num / denom;
}

float geometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0f);
    float k = (r*r) / 8.0f;

    float num   = NdotV;
    float denom = NdotV * (1.0f - k) + k;
	
    return num / denom;
}
float geometrySmith(float3 N, float3 V, float3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0f);
    float NdotL = max(dot(N, L), 0.0f);
    float ggx2  = geometrySchlickGGX(NdotV, roughness);
    float ggx1  = geometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}



sampler sampler0;
Texture2D tex_albedo_roughness;
Texture2D tex_normal;
Texture2D tex_position_roughness;

float3 getNormalFromMap(float3 world_pos, float2 uv, float3 normal) {
	float3 tangent_normal = normalize((tex_normal.Sample(sampler0, uv)).xyz * 2.0f - 1.0f);
	float3 q1 = ddx(world_pos);
	float3 q2 = ddy(world_pos);
	float2 st1 = ddx(uv);
	float2 st2 = ddy(uv);
	
	float3 n = normalize(normal);
	float3 t = normalize(q1 * st2.y - q2 * st1.y);
	float3 b = -normalize(cross(n, t));
	float3x3 tbn = float3x3(t, b, n);
	return normalize(mul(tangent_normal, tbn));
}

float4 ps_main(VS_OUTPUT input) : SV_TARGET  {
	float4 albedo_roughness = tex_albedo_roughness.Sample(sampler0, input.uv);
	float4 position_metallic = tex_position_roughness.Sample(sampler0, input.uv);
	float3 N = tex_normal.Sample(sampler0, input.uv).xyz;
	
	float3 albedo = albedo_roughness.xyz;
	float metallic = position_metallic.w;
	float3 world_pos = position_metallic.xyz;
	float roughness = albedo_roughness.w;
	// float3 N = input.normal.xyz;
	float3 V = normalize(cam_pos.xyz - world_pos);
	
	
	float3 f0 = float3(0.04f, 0.04f, 0.04f);
	f0 = lerp(f0, albedo, metallic);
	
	
	float3 lo = float3(0.0f, 0.0f, 0.0f);
	for(int i = 0; i < 4; i++) {
		Light light = lights[i];
		float3 l = normalize(light.position.xyz - world_pos);
		float3 h = normalize(V + l);
		
		float distance = length(light.position.xyz - world_pos);
		float attenuation = 1.0f / (distance * distance);
		float3 radiance = light.color.xyz * attenuation;
		
		float3 f = fresnelSchlick(max(dot(h, V), 0.0f), f0);	
		float ndf = distributionGGX(N, h, roughness);
		float g = geometrySmith(N, V, l, roughness);
		
		float n_dot_l = max(dot(N, l), 0.0f);
		
		float3 numerator = ndf * g * f;
		float denominator = 4.0f * max(dot(N, V), 0.0f) * n_dot_l;
		float3 specular = numerator / max(denominator, 0.001f);
		
		float3 ks = f;
		float3 kd = float3(1.0f, 1.0f, 1.0f) - ks;
		kd *= 1.0f - metallic;
		
		lo += (kd * albedo / PI + specular) * radiance * n_dot_l;
	}
	
	float3 ambient = float3(0.03f, 0.03f, 0.03f) * albedo;
	float3 color = ambient + lo;
	color = color / (color + float3(1.0f, 1.0f, 1.0f));
	float gamma = 1.0f / 2.2f;
	color = pow(color, float3(gamma, gamma, gamma));
	
	
	return float4(color, 1.0f);
}