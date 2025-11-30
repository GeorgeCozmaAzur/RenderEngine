
#pragma pack_matrix(row_major)

#include "../pbr/common.hlsl"   // contains BRDF() and helper functions

cbuffer FragUniformBufferObject : register(b0)
{
    float4x4 depthMVP;
    float4 light_dir;
    float4 camera_pos;
    //float  _pad0;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

// Input attachments 
Texture2D samplerAlbedo    : register(t0);
Texture2D samplerPosition  : register(t1);
Texture2D samplerNormal    : register(t2);
Texture2D samplerRM        : register(t3);

Texture2D sampleShadow     : register(t4);
SamplerState ShadowSampler : register(s0);

float textureProj(float4 shadowCoord, float2 off)
{
    float shadow = 1.0;

    if (shadowCoord.z > -1.0 && shadowCoord.z < 1.0)
    {
        float dist = sampleShadow.Sample(ShadowSampler, shadowCoord.xy + off).r;

        if (dist < shadowCoord.z)
        {
            shadow = 0.01;
        }
    }

    return shadow;
}

PSInput VSMain(uint vertexID : SV_VertexID)
{
    PSInput o;

    // 3 vertices that cover the whole screen
    float2 pos[3] = {
        float2(-1.0, -1.0),
        float2(-1.0,  3.0),
        float2( 3.0, -1.0)
    };

    o.position = float4(pos[vertexID], 0.0, 1.0);
    o.uv = 0.5f * (pos[vertexID] + 1.0f);
    o.uv.y = 1.0-o.uv.y;
    return o;
}

float4 PSMain(PSInput input) : SV_Target0
{
    // Input attachments are typically loaded by pixel coordinate:
    uint2 pix = uint2(input.position.xy);

    float3 albedo = samplerAlbedo.Load(int3(pix, 0)).rgb;
    float3 position = samplerPosition.Load(int3(pix, 0)).rgb;

    float4 normalSample = samplerNormal.Load(int3(pix, 0));
    float4 rmSample     = samplerRM.Load(int3(pix, 0));

    float3 normal       = normalSample.rgb;
    float  metallic     = rmSample.b;
    float  roughness    = rmSample.g;

    float3 L = light_dir.xyz;
    float3 N = normalize(normal);
    float3 viewDir = normalize(camera_pos - position);

    float4 shadowCoord = mul(float4(position, 1.0), depthMVP);
    float4 proj = shadowCoord / shadowCoord.w;
	proj.y *= -1.0;//because rt is upsidedown

    float shadow = textureProj(proj, float2(0.0, 0.0));

    float3 light = BRDF(L, viewDir, N, albedo, metallic, roughness) * 10.0 * shadow;

    light = light / (light + float3(1.0, 1.0, 1.0));
    light = pow(light, 1.0 / 2.2);

    return float4(light, 1.0);
}
