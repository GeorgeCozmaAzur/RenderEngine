#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUV;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 model;
	float depth;
} ubo;

layout (location = 0) out vec3 outUV;

out gl_PerVertex 
{
	vec4 gl_Position;   
};

void main() 
{
	outUV = vec3(inUV, ubo.depth);

	gl_Position = ubo.projection * ubo.model * vec4(inPos.xyz, 1.0);
	
}
