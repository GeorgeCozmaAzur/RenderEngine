#pragma pack_matrix(row_major)
// ------------------------------------------------------------
// Constant Buffer
// ------------------------------------------------------------
cbuffer UBO : register(b0)
{
    float4x4 projection;
    float4x4 model;
};

// ------------------------------------------------------------
// Vertex Shader
// ------------------------------------------------------------
struct VSInput
{
    float3 pos : POSITION;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float3 uvw      : POSITION;
};

PSInput VSMain(VSInput input)
{
    PSInput output;

    // Direction for cubemap lookup
    output.uvw = input.pos;

    // Convert GLSL: projection * model * vec4(pos,1)
    float4 worldPos = mul(float4(input.pos, 1.0), model);
    output.position = mul(worldPos, projection);

    return output;
}
// ============================================================
// Constant Buffer (UBOParams)
// ============================================================
cbuffer UBOParams : register(b1)
{
    float4 lights[4];
    float exposure;
    float gamma;
    float2 _padding;       // padding for 16-byte alignment
};

TextureCube samplerEnv : register(t0);
SamplerState envSampler : register(s0);

// ============================================================
// Uncharted 2 Tonemapping
// ============================================================
float3 Uncharted2Tonemap(float3 color)
{
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;

    return ((color * (A * color + C * B) + D * E) /
            (color * (A * color + B) + D * F)) - E / F;
}

// ============================================================
// Pixel Shader
// ============================================================
float4 PSMain(PSInput input) : SV_Target
{
    float3 sampleDir = input.uvw;
    sampleDir.y *= -1.0;          // flip like GLSL

    float3 color = samplerEnv.Sample(envSampler, sampleDir).rgb;

    // Tone mapping
    color = Uncharted2Tonemap(color * exposure);
    color = color * (1.0 / Uncharted2Tonemap(float3(11.2, 11.2, 11.2)));

    // Gamma correction
    color = pow(color, 1.0 / gamma);

    return float4(color, 1.0);
}