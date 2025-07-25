#version 450

#extension GL_GOOGLE_include_directive : enable
#include "../computeshader/common.glsl"

layout (binding = 1) uniform UBO 
{
	mat4 view_proj;
	vec4 bias_near_far_pow;
} ubo;

layout(binding = 2) uniform FragUniformBufferObject {
	vec3 diffuse;
	float specular_power;
	float transparency;
} frag_ubo;

layout (binding = 3) uniform sampler3D s_VoxelGrid;
layout (binding = 4) uniform sampler2D sampleShadow;
layout (binding = 4) uniform sampler2D sampleShadowColor;

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inPosition;
layout (location = 3) in vec3 inLightPos;
layout (location = 4) in vec3 inCamPosition;
layout (location = 5) in vec4 inShadowCoord;

layout (location = 0) out vec4 outFragColor;

vec3 worldToUV(vec3 p)
{
	vec3 minBound = vec3(-3.0, -3.0, -3.0); 
	vec3 maxBound = vec3(3.0, 3.0, 3.0); 
	return (p-minBound)/(maxBound-minBound);
}

float computeShadow(vec3 fragPos)
{
	float maxDist = 30.0;
	float minStep = 0.05;
	vec3 lightPos = vec3(4.0,-3.0,4.0);
	vec3 rayDir = normalize(lightPos - fragPos);
	float maxRayLength = length(lightPos - fragPos);
	float travelDist = 0.0;
	float res = 1.0;
	for(int i =0;i<128;i++)
	{
		vec3 curPos = fragPos + rayDir * travelDist;
		vec3 texCoord = worldToUV(curPos);
		float dist = texture(s_VoxelGrid, texCoord).r;
		//if(dist < minStep)
		//	return 0.0;
		//res = min(res, dist);//50 * dist/travelDist);
		res = min(res, 50 * (dist/travelDist));
		travelDist += dist;
		if(travelDist >= maxRayLength || travelDist >= maxDist)
			break;
	}
	return clamp(res, 0.0,1.0);
	//return 1.0;
}

void main() 
{
	vec4 vcolor = vec4(1.0,0.0,0.0,1.0);//texture(s_VoxelGrid, worldToUV(inPosition));//vec3(inUV, 0.5));
	float s= computeShadow(inPosition + inNormal * 0.02);
	outFragColor = vcolor * s;
}