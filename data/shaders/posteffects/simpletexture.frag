#version 450

layout (binding = 0) uniform sampler2D samplerColor;
//layout (binding = 1) uniform sampler2D samplerColor2;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	vec4 col = texture(samplerColor, inUV);			
	//vec4 col2 = texture(samplerColor2, inUV);			
	outFragColor = col;// * col2;
}