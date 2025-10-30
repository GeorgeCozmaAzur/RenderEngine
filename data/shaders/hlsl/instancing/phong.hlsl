//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************
#pragma pack_matrix(row_major)

struct VSInput
{
    float3 position    : POSITION;
    float3 normal    : NORMAL;
    float2 uv        : TEXCOORD0;
	
	float4 instance_position: INSTANCE_POSITION;
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

Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);


PSInput VSMain(VSInput input)
{
    PSInput result;

    float4x4 g_mWorldViewProj =  mul(view, projection);
    result.position = mul(float4(input.position + input.instance_position, 1.0f), g_mWorldViewProj);//position + offset;
	result.positionw = input.position + input.instance_position;
    result.uv = input.uv;
	result.normal = input.normal;
	result.lightPos = light_pos;
	result.camPos = camera_pos;

    return result;
}

float4 PSMainTextured(PSInput input) : SV_TARGET
{
	float3 lightDir = normalize(input.lightPos - input.positionw);  
	float3 N = normalize(input.normal);
	float diff = max(dot(N, lightDir), 0.0);
	float3 viewDir = normalize(input.camPos - input.positionw);
	float3 reflectDir = reflect(-lightDir, N); 
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
	
    return ( diff + spec ) * g_texture.Sample(g_sampler, input.uv);
}
