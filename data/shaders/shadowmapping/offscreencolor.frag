#version 450

layout(set = 0, binding = 1) uniform FragUniformBufferObject {
	vec3 diffuse;
	float specular_power;
	float transparency;
} frag_ubo;

layout(location = 0) out vec4 outputColor;

void main() 
{
	vec3 whitecolor = vec3(1.0);
	vec3 ownColor = mix(whitecolor, frag_ubo.diffuse, frag_ubo.transparency);
	outputColor = vec4(ownColor, frag_ubo.transparency);
}