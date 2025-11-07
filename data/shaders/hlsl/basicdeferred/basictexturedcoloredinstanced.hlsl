
#pragma pack_matrix(row_major)

struct VSInput
{
    float3 position : POSITION; 
    float2 uv       : TEXCOORD0;
    float3 color    : COLOR;   
    float3 normal   : NORMAL;   
	
	float3 instancePosition  : INSTANCE_POSITION;
};

struct PSInput
{
    float4 position  : SV_Position;
    float2 uv        : TEXCOORD0;
    float3 color     : COLOR0;
    float3 worldPos  : TEXCOORD1;
    float3 normal    : TEXCOORD2;
};

struct PSOutput
{
    float4 outFragColor     : SV_Target0;
    float4 outFragPositions : SV_Target1;
    float4 outFragNormals   : SV_Target2;
};

cbuffer UboView : register(b0)
{
    float4x4 projection;
    float4x4 view;
    float4x4 model;
};

Texture2D samplerColor : register(t0);
SamplerState samplerLinear : register(s0);

PSInput VSMain(VSInput input)
{
    PSInput output;

    output.uv = input.uv;
    output.color = input.color;
    output.normal = input.normal;

    float4 worldPosition = mul(float4(input.position + input.instancePosition, 1.0), model);
    output.worldPos = worldPosition.xyz;

    // Transform to clip space
    float4 viewPosition = mul(worldPosition, view);
    output.position = mul(viewPosition, projection);

    return output;
}

PSOutput PSMain(PSInput input)
{
    PSOutput output;

    float4 texColor = samplerColor.Sample(samplerLinear, input.uv);
    output.outFragColor = texColor;
    output.outFragPositions = float4(input.worldPos, 1.0);
    output.outFragNormals = float4(input.normal, 1.0);

    return output;
}