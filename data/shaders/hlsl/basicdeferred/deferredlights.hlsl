#pragma pack_matrix(row_major)

struct VSInput
{
    float3 position                : POSITION;
    float4 instancePositionRadius  : INSTANCE_POSITION;
    float4 instanceColor           : INSTANCE_COLOR;
};

struct PSInput
{
    float4 position                : SV_Position;
    float4 positionwvp             : Position;
    float4 centerPosition          : TEXCOORD0;
    float4 color                   : TEXCOORD1;
};

cbuffer UboView : register(b0)
{
    float4x4 projection;
    float4x4 view;
};

Texture2D samplerPosition : register(t0);
Texture2D samplerNormal   : register(t1);
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

    float3 position = samplerPosition.Sample(linearSampler, uv).rgb;
    float4 normalSample = samplerNormal.Sample(linearSampler, uv);

    float3 normal = normalSample.rgb;
    float weHaveNormal = normalSample.a;

    float3 L = input.centerPosition.xyz - position;
    float dist = length(L);
    float atten = input.centerPosition.w / (dist * dist + 1.0);

    L = normalize(L);
    float3 N = normalize(normal);
    float NdotL = weHaveNormal * max(0.1, dot(N, L));

    float intensity = NdotL * atten;
    float3 light = input.color.rgb * intensity;

    return float4(light, intensity);
}