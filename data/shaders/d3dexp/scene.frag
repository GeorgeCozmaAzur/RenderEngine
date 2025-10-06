#version 450

layout (binding = 1) uniform sampler2D colorMap;
layout (binding = 2) uniform sampler2D shadowMap;

//layout (location = 0) in vec3 inNormal;
//layout (location = 1) in vec3 inColor;
//layout (location = 2) in vec3 inViewVec;
//layout (location = 3) in vec3 inLightVec;
layout (location = 0) in vec4 inShadowCoord;
layout (location = 1) in vec2 inuv;
/*
layout (binding = 2) uniform UboView 
{
	int technique;
} ubo;*/

layout (location = 0) out vec4 outFragColor;

#define ambient 0.1

float textureProj(vec4 shadowCoord, vec2 off)
{
	float shadow = 1.0;
	//if ( shadowCoord.z > -1.0 && shadowCoord.z < 1.0 ) 
	//{
		float dist = texture( shadowMap, shadowCoord.st + off ).r;
		if ( shadowCoord.w > 0.0 && dist < shadowCoord.z ) 
		{
			shadow = ambient;
		}
	//}
	return shadow;
}

float linstep(float low, float high, float v)
{
	return clamp((v-low)/(high-low), 0.0, 1.0);
}

float varianceshadow(vec4 shadowCoord, vec2 off)
{
	float lightBleedReductionAmount = 0.2;
	float varianceMin = 0.0000002;
	vec2 moments = texture(shadowMap, shadowCoord.st + off).xy;
	
	float p = step(shadowCoord.z, moments.x);
	float variance = max(moments.y - moments.x * moments.x, varianceMin);
	
	float d = shadowCoord.z - moments.x;
	float pMax = linstep(lightBleedReductionAmount, 1.0, variance / (variance + d*d));
	
	return min(max(p, pMax), 1.0);
}

float filterPCF(vec4 sc)
{
	ivec2 texDim = textureSize(shadowMap, 0);
	float scale = 0.5;
	float dx = scale * 1.0 / float(texDim.x);
	float dy = scale * 1.0 / float(texDim.y);

	float shadowFactor = 0.0;
	int count = 0;
	int range = 1;
	
	for (int x = -range; x <= range; x++)
	{
		for (int y = -range; y <= range; y++)
		{
			shadowFactor += textureProj(sc, vec2(dx*x, dy*y));
			count++;
		}
	
	}
	return shadowFactor / count;
}

void main() 
{	
	float shadow = 0.0;
	/*if(ubo.technique == 0)
	{
		shadow = textureProj(inShadowCoord / inShadowCoord.w, vec2(0.0));
	}
	else
	if(ubo.technique == 1)
	{
		shadow = filterPCF(inShadowCoord / inShadowCoord.w);
	}
	else
	if(ubo.technique == 2)
	{
		shadow = varianceshadow(inShadowCoord / inShadowCoord.w, vec2(0.0));
	}*/
	
	shadow = textureProj(inShadowCoord / inShadowCoord.w, vec2(0.0));

	/*vec3 N = normalize(inNormal);
	vec3 L = normalize(inLightVec);
	vec3 V = normalize(inViewVec);
	vec3 R = normalize(-reflect(L, N));
	vec3 diffuse = max(dot(N, L), ambient) * inColor;*/
	vec3 diffuse = texture( colorMap, inuv ).rgb;

	outFragColor = vec4(diffuse * shadow, 1.0);
}
