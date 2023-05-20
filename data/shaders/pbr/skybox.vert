#version 450

layout (location = 0) in vec3 inPos;

layout (binding = 0) uniform UboView 
{
	mat4 projection;
	mat4 view; 
	vec4 light_pos;
	vec3 camera_pos;
} ubo;

layout (binding = 1) uniform UBO 
{
	mat4 model;
} uboModel;

layout (location = 0) out vec3 outUVW;

out gl_PerVertex 
{
	vec4 gl_Position;
};

void main() 
{
	outUVW = inPos;
	gl_Position = ubo.projection * uboModel.model * vec4(inPos.xyz, 1.0);
}
