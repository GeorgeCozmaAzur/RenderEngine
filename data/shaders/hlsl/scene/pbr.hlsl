
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
	float4 light_color;
};

cbuffer frag_ubo : register(b2)
{
	float baseColorFactor;
	float metallicFactor;
	float roughnessFactor;
	float aoFactor;
};

Texture2D g_texture_albedo : register(t0);
Texture2D g_texture_rm : register(t1);
SamplerState g_sampler : register(s0);


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
	float3 lightDir = normalize(input.lightPos - input.positionw);  
	float3 N = normalize(input.normal);
	float diff = max(dot(N, lightDir), 0.0);
	float3 viewDir = normalize(input.camPos - input.positionw);
	float3 reflectDir = reflect(-lightDir, N); 
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
	
    return ( diff + spec ) * g_texture_albedo.Sample(g_sampler, input.uv);
}
