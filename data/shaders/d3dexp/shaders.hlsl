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

struct VSInput
{
    float3 position    : POSITION;
    float2 uv        : TEXCOORD0;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float4 position1 : POSITION;
    float4 positionlp : WPOSITION;
    float2 uv : TEXCOORD;
};

cbuffer cb0 : register(b0)
{
    float4x4 g_mProj;
    float4x4 g_mWorldView;
    float4x4 g_mlightView;
    //float4 scale;
   // float4 offset;
    float4 padding[4];
    //float4 offset;
    //float4 padding[15];
};

Texture2D g_texture : register(t0);
Texture2D g_texture1 : register(t1);
SamplerState g_sampler : register(s0);

float LinearizeDepth(float depth)
{
    float n = 0.1; // camera z near
    float f = 5.0; // camera z far
    float z = depth;
    return (2.0 * n) / (f + n - z * (f - n));
}

float TrueDepth(float depth)
{
    float n = 0.1; // camera z near
    float f = 5.0; // camera z far
    float z = depth;
    return depth * (f - n) + n;
}

PSInput VSMain(VSInput input)
//PSInput VSMain(float4 position : POSITION, float4 uv : TEXCOORD)
{
    const float4x4 biasMat = float4x4(
        0.5, 0.0, 0.0, 0.5,
        0.0, 0.5, 0.0, 0.5,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0);

    PSInput result;

    //result.position = position;
    float4x4 g_mWorldViewProj =  mul(g_mWorldView, g_mProj);
    float4x4 g_mWorldViewProjlp =  mul(g_mlightView, g_mProj);
    //g_mWorldViewProjlp =  mul(biasMat, g_mWorldViewProjlp);
    result.position = mul(float4(input.position, 1.0f), g_mWorldViewProj);//position + offset;
    result.position1 = mul(float4(input.position, 1.0f), g_mWorldViewProj);//position + offset;
    result.positionlp = mul(float4(input.position, 1.0f), g_mWorldViewProjlp);//position + offset;
    //result.position = position + offset;
    result.uv = input.uv;

    return result;
}

float4 PSMainOffscreen(PSInput input) : SV_TARGET
{
    float depth = input.position.z;// / input.position1.w;
    return float4(depth + depth * 0.005, 0.0, 0.0, 1.0);
    //return float4(depth, 0.0, 0.0, 1.0);
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return g_texture.Sample(g_sampler, input.uv);
}



float4 PSMainMT(PSInput input) : SV_TARGET
{
    //float4 tmp = float4(1.0 / input.position.w);
    float4 projCoord = input.positionlp / input.positionlp.w;// *tmp;

    projCoord.y *= -1.0;

    // Scale and bias
    projCoord.xy += 1.0;
    projCoord.xy *= 0.5;

    float shadow = 1.0;
    float dist = g_texture1.Sample(g_sampler, projCoord.xy).x;
    if (projCoord.w > 0.0 && dist < projCoord.z)
    {
        shadow = 0.1;
    }
    //shadow = LinearizeDepth(dist);

    //return float4(shadow, shadow, shadow, 1.0);//(g_texture.Sample(g_sampler, input.uv) * 0.1 + g_texture1.Sample(g_sampler, input.uv)) * 0.5;
    return (float4(shadow, shadow, shadow, 1.0) + g_texture.Sample(g_sampler, input.uv)) * 0.5;
}