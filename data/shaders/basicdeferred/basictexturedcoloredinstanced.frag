#version 450

layout (binding = 1) uniform sampler2D samplerColor;

layout (location = 0) in vec2 inUV;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec3 inPosition;
layout (location = 3) in vec3 inNormal;


layout (location = 0) out vec4 outFragColor;
layout (location = 1) out vec4 outFragPositions;
layout (location = 2) out vec4 outFragNormals;

void main() 
{
	vec4 tex_color = texture(samplerColor, inUV);
	outFragColor  = vec4(inColor, 1.0) * tex_color;	
	outFragPositions  = vec4(inPosition, 1.0);
	outFragNormals  = vec4(inNormal, 1.0);	
}