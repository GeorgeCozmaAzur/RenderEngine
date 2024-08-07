#version 450

layout (location = 0) in vec3 inColor;

layout (location = 0) out vec4 outFragColor;


void main() 
{
	vec4 color = vec4(inColor, inColor.x);
	if(color.r <= 0.5)
	discard;
	outFragColor  = color;	
}