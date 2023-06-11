#version 450

layout (binding = 2) uniform sampler2D samplerColor;

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inPosition;
layout (location = 3) in vec3 inLightPos;
layout (location = 4) in vec3 inCamPosition;

layout (location = 0) out vec4 outFragColor;


void main() 
{
	vec4 tex_color = texture(samplerColor, inUV);
	vec4 own_color = tex_color;
	outFragColor = own_color;
}