#pragma pack_matrix(row_major)

#include "../pbr/common.hlsl"   // contains BRDF() and helper functions

struct VSInput
{
    float3 position                : POSITION;
    float4 instancePositionRadius  : INSTANCE_POSITION;
    float4 instanceColor           : INSTANCE_COLOR;
};

struct PSInput
{
    float4 position                : SV_Position;
    float4 positionwvp                : Position;
    float4 centerPosition   : TEXCOORD0;
    float4 color            : TEXCOORD1;
    float3 camera           : TEXCOORD2;
};

cbuffer UboView : register(b0)
{
    float4x4 projection;
    float4x4 view;
	float3   camera;
    float    _pad0; // padding to 16 bytes
};

Texture2D samplerAlbedo  : register(t0);
Texture2D samplerPosition: register(t1);
Texture2D samplerNormal  : register(t2);
Texture2D samplerRM      : register(t3);
SamplerState linearSampler : register(s0);

PSInput VSMain(VSInput input)
{
    PSInput output;

    output.centerPosition = input.instancePositionRadius;
    output.color = input.instanceColor;

    float3 worldPos = input.position * input.instancePositionRadius.w + input.instancePositionRadius.xyz;
    float4 objectPosition = float4(worldPos, 1.0);

    output.position = mul(objectPosition, view);
    output.position = mul(output.position, projection);
	
	output.positionwvp = output.position;
	output.camera = camera;

    return output;
}

float2 GetScreenUV(float4 pos_clip)
{
    float2 uv = pos_clip.xy / pos_clip.w * 0.5 + 0.5;
    uv.y = 1.0 - uv.y; // DirectX top-left origin
    return uv;
}

float4 PSMain(PSInput input) : SV_Target
{
    // Reconstruct UV from pixel position
    float2 uv = GetScreenUV(input.positionwvp);

   float3 albedo = samplerAlbedo.Sample(linearSampler, uv).rgb;
    float3 position = samplerPosition.Sample(linearSampler, uv).rgb;
    float4 normalSample = samplerNormal.Sample(linearSampler, uv);
    float4 rmSample = samplerRM.Sample(linearSampler, uv);

    float3 normal = normalize(normalSample.rgb);
    float weHaveNormal = normalSample.a;
    float metallic = rmSample.b;
    float roughness = rmSample.g;

    float3 L = input.centerPosition.xyz - position;
    float dist = length(L);
    float atten = input.centerPosition.w / (dist * dist + 1.0);
    L = normalize(L);
    float3 N = normalize(normal);
    float NdotL = weHaveNormal * max(0.1, dot(N, L));
    float intensity = NdotL * atten;

    float3 viewDir = normalize(input.camera - position);
    float attenuation = 1.0 / (dist * dist);
    float3 radiance = input.color.rgb * attenuation;
    float3 light = BRDF(L, viewDir, N, albedo, metallic, roughness) * radiance;

    // HDR tonemap
    light = light / (light + 1.0);
    // gamma correction
    light = pow(light, 1.0 / 2.2);

    float alpha = saturate(1.0 - dist / input.centerPosition.w);
    //return float4(albedo, 1.0);//float4(light, alpha);
    return float4(light, alpha);
}