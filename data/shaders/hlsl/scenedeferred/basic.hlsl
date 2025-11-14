
#pragma pack_matrix(row_major)

struct VSInput
{
    float3 position : POSITION; 
    float2 uv       : TEXCOORD0;
    float3 normal   : NORMAL;   
};

struct PSInput
{
    float4 position  : SV_Position;
    float2 uv        : TEXCOORD0;
    float3 worldPos  : TEXCOORD1;
    float3 normal    : TEXCOORD2;
};

struct PSOutput
{
    float4 outFragColor     		: SV_Target0;
    float4 outFragPositions 		: SV_Target1;
    float4 outFragNormals   		: SV_Target2;
    float4 outFragRoughMetallic   	: SV_Target3;
};

cbuffer UboView : register(b0)
{
    float4x4 projection;
    float4x4 view;
    float3 camerapos;
};

cbuffer frag_ubo : register(b1)
{
	float baseColorFactor;
	float metallicFactor;
	float roughnessFactor;
	float aoFactor;
};

Texture2D albedoSampler : 			register(t0);
Texture2D roughnessMetallicSampler : register(t1);
SamplerState samplerLinear : register(s0);

PSInput VSMain(VSInput input)
{
    PSInput output;

    output.uv = input.uv;
    output.normal = input.normal;

    float4 worldPosition = float4(input.position, 1.0);//mul(float4(input.position, 1.0), model);
    output.worldPos = worldPosition.xyz;

    // Transform to clip space
    float4 viewPosition = mul(worldPosition, view);
    output.position = mul(viewPosition, projection);

    return output;
}

PSOutput PSMain(PSInput input)
{
    PSOutput output;

    // Base color (gamma → linear)
	float3 albedo = pow(albedoSampler.Sample(samplerLinear, input.uv).rgb, 2.2f.xxx) * baseColorFactor;
	 // Roughness/metallic map
    float3 rm = roughnessMetallicSampler.Sample(samplerLinear, input.uv).rgb;
    float roughness = rm.g * roughnessFactor;
    float metallic  = rm.b * metallicFactor;
	
	float3 N = normalize(input.normal);
	
    output.outFragColor = float4(albedo, 1.0);
    output.outFragPositions = float4(input.worldPos, 1.0);
    output.outFragNormals = float4(N, 1.0);
	output.outFragRoughMetallic = float4(1.0,roughness,metallic, 1.0);

    return output;
}