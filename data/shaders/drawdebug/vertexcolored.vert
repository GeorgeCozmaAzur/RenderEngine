#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inColor;

layout (binding = 0) uniform UboView 
{
	mat4 projection;
	mat4 view;
	vec4 light_pos;
	vec3 camera_pos;
} uboView;
/*
layout (binding = 1) uniform UBO 
{
	mat4 model; 
} ubo;
*/
layout (location = 0) out vec3 outColor;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() 
{
	outColor = inColor;
	gl_Position = uboView.projection * uboView.view * vec4(inPos.xyz, 1.0);
}