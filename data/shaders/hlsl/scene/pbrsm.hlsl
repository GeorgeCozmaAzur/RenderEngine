
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
    float4 shadowCoord : SHADOW;
};

cbuffer cb0 : register(b0)
{
	float4x4 projection;
	float4x4 view; 
	float4x4 lightSpace; 
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
Texture2D sampleShadow				: register(t2);
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
	result.shadowCoord = mul(float4(input.position, 1.0f), lightSpace);

    return result;
}

float textureProj(float4 shadowCoord, float2 off)
{
    float shadow = 1.0f;

    // shadowCoord is expected to be pre-divided by w (i.e. already projected), same as in GLSL usage.
    // Keep the same bounds check used in the GLSL shader.
    if (shadowCoord.z > -1.0f && shadowCoord.z < 1.0f)
    {
        // sample shadow map at st + off (note: sample returns float4)
        float dist = sampleShadow.Sample(samLinear, shadowCoord.xy + off).r;

        // The GLSL code tested (shadowCoord.w > 0.0 && dist < shadowCoord.z)
        // In the original call they passed inShadowCoord / inShadowCoord.w so w==1.0 there.
        // We'll keep the same test in case shadowCoord.w carries sign info.
        if (shadowCoord.w > 0.0f && dist < shadowCoord.z)
        {
            shadow = aoFactor; // from FragUniformBufferObject
        }
    }
    return shadow;
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
	
	float4 proj = input.shadowCoord;
    if (proj.w != 0.0f)
    {
        proj = proj / proj.w; // same as GLSL: inShadowCoord / inShadowCoord.w
		proj.y *= -1.0;//because rt is upsidedown
    }
    float shadow = textureProj(proj, float2(0.0f, 0.0f));
	
	 // Ambient (AO)
    float3 ambient = albedo * shadow;

    float3 color = ambient + Lo;
	
	// HDR tonemapping
    color = color / (color + 1.0f.xxx);
    // Gamma correction
    color = pow(color, 1.0f / 2.2f);

    return float4(color, 1.0f);
}
