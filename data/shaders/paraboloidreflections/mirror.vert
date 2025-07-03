#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inColor;
layout (location = 3) in vec3 inNormal;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 model;
	vec4 lightPos;
	vec4 camPos;
} ubo;

layout (location = 0) out vec2 outUV;
layout (location = 1) out vec4 outPos;
layout (location = 2) out vec3 outNormal;
layout (location = 3) out vec4 outEyePos;

void main() 
{
	outUV = inUV;
	outPos = vec4(inPos.xyz, 1.0);
	outEyePos = ubo.camPos;
	outNormal = inNormal;
	gl_Position = ubo.projection *ubo.model* outPos;		
}
