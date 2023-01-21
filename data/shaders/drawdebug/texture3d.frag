#version 450

layout (binding = 1) uniform sampler3D samplerColor;

layout (location = 0) in vec3 inUV;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	vec4 color = texture(samplerColor, inUV);

	outFragColor = color;
}