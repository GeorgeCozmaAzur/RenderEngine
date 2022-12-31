#version 450

layout (binding = 0) uniform sampler2D samplerColor;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	vec4 col = texture(samplerColor, inUV);
	float luminosity = 0.299 * col.r +  0.587 * col.g + 0.114 * col.b;
    vec3 RGB = vec3(luminosity);
    
    //vec3 sRGB = sqrt(RGB);
			
	outFragColor = vec4(RGB, 1.0);
}