#version 450

layout (location = 0) in vec3 inPos;

layout (binding = 0) uniform UBO 
{
	mat4 depthMP;
	mat4 depthMV;
	mat4 depthMLV;
	float padding[16];
} ubo;

out gl_PerVertex 
{
    vec4 gl_Position;   
};

 
void main()
{
	gl_Position =  ubo.depthMP * ubo.depthMV * vec4(inPos, 1.0);
}