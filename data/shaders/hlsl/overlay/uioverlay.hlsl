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
    float2 position    : POSITION;
    float2 uv        : TEXCOORD;
    float4 color        : COLOR;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
	float4 color        : COLOR;
};

Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);

cbuffer cb0 : register(b0)
{
	float2 scale;
	float2 translate;
}

PSInput VSMain(VSInput input)
{
    PSInput result;

    float2 finalPos = input.position * scale + translate;
	result.position = float4(finalPos, 0.0, 1.0);
	result.position.y = -result.position.y;
    result.uv = input.uv;
    result.color = input.color;

    return result;
}



float4 PSMain(PSInput input) : SV_TARGET
{
    return g_texture.Sample(g_sampler, input.uv) * input.color;
}


