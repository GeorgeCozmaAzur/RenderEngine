
#pragma pack_matrix(row_major)

struct VSInput
{
    float3 position    : POSITION;
    float3 normal    : NORMAL;
    float2 uv        : TEXCOORD0;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);


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

float4 PSMainTextured(PSInput input) : SV_TARGET
{
    return g_texture.Sample(g_sampler, input.uv);
}
