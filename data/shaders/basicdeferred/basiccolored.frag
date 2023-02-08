#version 450

layout (location = 0) in vec2 inUV;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec3 inPosition;
layout (location = 3) in vec3 inNormal;


layout (location = 0) out vec4 outFragColor;
layout (location = 1) out vec4 outFragPositions;
layout (location = 2) out vec4 outFragNormals;

void main() 
{
	outFragColor = vec4(inColor, 1.0);	
	outFragPositions  = vec4(inPosition, 1.0);
	outFragNormals  = vec4(inNormal, 1.0);	
}