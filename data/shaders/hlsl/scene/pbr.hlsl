
#pragma pack_matrix(row_major)

#include "../pbr/common.hlsl"   // contains BRDF() and helper functions

struct VSInput
{
    float3 position    : POSITION;
    float3 normal    : NORMAL;
    float2 uv        : TEXCOORD0;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float3 positionw : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
	float3 lightPos : LIGHT;
    float3 camPos : CAMERA;
};

cbuffer cb0 : register(b0)
{
	float4x4 projection;
	float4x4 view; 
	float4 light_pos;
	float3 camera_pos;
};

cbuffer global_frag_ubo : register(b1)
{
	float4 light0Color;
};

cbuffer frag_ubo : register(b2)
{
	float baseColorFactor;
	float metallicFactor;
	float roughnessFactor;
	float aoFactor;
};

Texture2D albedoSampler             : register(t0);
Texture2D roughnessMetallicSampler  : register(t1);
SamplerState samLinear              : register(s0);


PSInput VSMain(VSInput input)
{
    PSInput result;

    float4x4 g_mWorldViewProj =  mul(view, projection);
    result.position = mul(float4(input.position, 1.0f), g_mWorldViewProj);//position + offset;
	result.positionw = input.position;
    result.uv = input.uv;
	result.normal = input.normal;
	result.lightPos = light_pos.xyz;
	result.camPos = camera_pos;

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
	// Base color (gamma → linear)
    float3 albedo = pow(albedoSampler.Sample(samLinear, input.uv).rgb, 2.2f.xxx) * baseColorFactor;

 // Roughness/metallic map
    float3 rm = roughnessMetallicSampler.Sample(samLinear, input.uv).rgb;
    float roughness = rm.g * roughnessFactor;
    float metallic  = rm.b * metallicFactor;
	
	 // View direction & normal
    float3 viewDir = normalize(input.camPos - input.positionw);
    float3 N = normalize(input.normal);
	
	float3 Lo = 0.0f.xxx;
	
	// Single point light
    float3 lightDir = normalize(input.lightPos - input.positionw);
    float distance = length(input.lightPos - input.positionw);
    float attenuation = 1.0f / (distance * distance);
    float3 radiance = light0Color.rgb * attenuation;
	
	Lo += BRDF(lightDir, viewDir, N, albedo, metallic, roughness) * radiance;
	
	 // Ambient (AO)
    float3 ambient = aoFactor.xxx * albedo;

    float3 color = ambient + Lo;
	
	// HDR tonemapping
    color = color / (color + 1.0f.xxx);
    // Gamma correction
    color = pow(color, 1.0f / 2.2f);

    return float4(color, 1.0f);
}
