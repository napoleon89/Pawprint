#pragma conditional DEPTH_NORMALS

struct Light {
	float4 position;
	float4 color;
	float constant_term;
	float linear_term;
	float quadratic_term;
	float padding;
};

cbuffer PerScene : register(b0) {
	float4x4 projection;	
	float4x4 view;
	float4 camera_position;
	float4 screen_size;
	Light lights[4];
};


static const float2 invAtan = float2(0.1591f, 0.3183f);
float2 sampleSphericalMap(float3 v)
{
    float2 uv = float2(atan2(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5f;
    return uv;
}

static const float PI = 3.14159265359f;

float3 fresnelSchlick(float cos_theta, float3 f0, float roughness) {
	float d = 1.0f - roughness;
	return f0 + (max(float3(d, d, d), f0) - f0) * pow(1.0f - cos_theta, 5.0f);
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
Texture2D main_texture : register(t0);
Texture2D tex_irradiance_map : register(t1);
Texture2D tex_environment_map : register(t2);
Texture2D tex_ao_pass : register(t3);

struct SurfaceShaderOutput {
	float3 albedo;
	float roughness;
	float metallic;
};


cbuffer PerObject : register(b1) {
	float4x4 model;
	float4 color;
	float4 metallic_roughness;
	float4x4 skining_matrices[128];
};

struct VS_OUTPUT {
	float4 pos : SV_POSITION;
	float3 world_pos : POSITION0;
	float3 normal : NORMAL;
	float2 uv : TEXCOORD1;
	float4 ids : COLOR;
};

VS_OUTPUT  vs_main(float3 in_pos : POSITION, float3 in_normal : NORMAL, float2 in_uv : TEXCOORD, float4 in_weights : BLENDWEIGHT, int4 in_joint_ids : BLENDINDICES) {
	VS_OUTPUT output;
	
	float4 total_local_pos = float4(0.0f, 0.0f, 0.0f, 0.0f);

	for(int i = 0; i < 4; i++) {
		// if(in_joint_ids[i] > -1) {
			float4x4 joint_transform = skining_matrices[in_joint_ids[i]];
			float4 pose_pos = mul(float4(in_pos, 1.0f), joint_transform);
			total_local_pos += pose_pos * in_weights[i];
		// }
	}

	float4 world_pos = mul(float4(total_local_pos.xyz, 1.0f), model);
	float4 view_pos = mul(world_pos, view);
	float4 clip_pos = mul(view_pos, projection);
	output.pos = clip_pos;
	output.normal = normalize(mul(float4(in_normal.x, in_normal.y, in_normal.z, 0.0f), model).xyz);
	output.world_pos = world_pos.xyz;
	output.uv = in_uv;
	output.ids = float4(in_joint_ids.x, in_joint_ids.y, in_joint_ids.z, in_joint_ids.w);
	return output;
}

SurfaceShaderOutput surfaceFunc(VS_OUTPUT input) {
	SurfaceShaderOutput output;
	output.albedo = color.rgb;
	output.roughness = metallic_roughness.y * 0.9f + 0.1f;
	output.metallic = metallic_roughness.x;
	return output;
}



#ifdef DEPTH_NORMALS

struct PixelOut {
	float4 view_normal : SV_TARGET0;
	float4 view_position : SV_TARGET1;
};

PixelOut ps_main(VS_OUTPUT input) {

	PixelOut result;
	result.view_normal = float4(input.view_normal, 1.0f);
	result.view_position = float4(input.view_pos, 1.0f);
	return result;
}

#else 

float4 ps_main(VS_OUTPUT input) : SV_TARGET0 {
	float2 screen_uv = input.pos.xy / screen_size.xy;
	float3 normal = normalize(input.normal);
	
	float3 world_pos = input.world_pos;
	SurfaceShaderOutput surface_output = surfaceFunc(input);
	float3 albedo = surface_output.albedo;
	float roughness = surface_output.roughness;
	float metallic = surface_output.metallic;
	
	float3 view = normalize(camera_position.xyz - world_pos);
	// return float4(abs(normal), 1.0f);

	float3 reflection = reflect(-view, normal);

	float3 base_surface_reflection = float3(0.04f, 0.04f, 0.04f);
	// float3 base_surface_reflection = tex_environment_map.Sample(sampler0,)
	base_surface_reflection = lerp(base_surface_reflection, albedo, metallic);
	float3 irradiance = tex_irradiance_map.Sample(sampler0, sampleSphericalMap(normal)).rgb;
	// float3 irradiance = float3(0.03f, 0.03f, 0.03f);

	float3 light_radiance = float3(0.0f, 0.0f, 0.0f);
	for(int i = 0; i < 4; i++) {
		Light light = lights[i];
		float3 pixel_to_light = normalize(light.position.xyz - world_pos);
		float3 camera_to_light = normalize(view + pixel_to_light);

		float distance = length(light.position.xyz - world_pos);
		float attenuation = 1.0f / (light.constant_term + light.linear_term * distance + light.quadratic_term * (distance * distance));
		// float attenuation = 1.0f / (distance * distance);
		// float attenuation = clamp(1.0f - distance / distance / (radius * radius), 0.0f, 1.0f);
		// attenuation *= attenuation;
		// float attenuation = smoothstep(radius, 0.0f, distance);
		float3 radiance = light.color.rgb * attenuation;

		float3 fresnel = fresnelSchlick(max(dot(camera_to_light, view), 0.0f), base_surface_reflection, roughness);
		float ndf = distributionGGX(normal, camera_to_light, roughness);
		float g = geometrySmith(normal, view, pixel_to_light, roughness);
		float ndotl = max(dot(normal, pixel_to_light), 0.0f);
		float3 numerator = ndf * g * fresnel;
		float denominator = 4.0f * max(dot(normal, view), 0.0f) * ndotl;
		float3 specular = numerator / max(denominator, 0.001f);

		float3 ks = fresnel;
		float3 kd = float3(1.0f, 1.0f, 1.0f) - ks;
		kd *= 1.0f - metallic;		

		
		light_radiance += (kd * albedo / PI + specular) * radiance * ndotl;
	}

	float ao = tex_ao_pass.Sample(sampler0, screen_uv).r;

	float3 ks = fresnelSchlick(max(dot(normal, view), 0.0), base_surface_reflection, roughness);
	float3 kd = float3(1.0f, 1.0f, 1.0f) - ks;
	kd *= 1.0f - metallic;
	float3 diffuse = irradiance * albedo;
	float3 ambient = (kd * diffuse);

	float3 final_color = (ambient + light_radiance) * ao;
	return float4(final_color, 1.0f);
}
#endif
