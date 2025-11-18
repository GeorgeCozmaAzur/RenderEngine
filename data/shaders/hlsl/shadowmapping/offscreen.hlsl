
#pragma pack_matrix(row_major)

struct VSInput
{
    float3 position    : POSITION;
};

struct PSInput
{
    float4 position : SV_POSITION;
};

cbuffer cb0 : register(b0)
{
    float4x4 depthMVP;
};

PSInput VSMain(VSInput input)
{
    PSInput result;
    result.position = mul(float4(input.position, 1.0f), depthMVP);

    return result;
}

