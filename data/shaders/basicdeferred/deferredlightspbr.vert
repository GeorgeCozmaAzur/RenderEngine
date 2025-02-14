#version 450

layout (location = 0) in vec3 inPos;

layout (location = 1) in vec4 instancePositionAndRadius;
layout (location = 2) in vec4 instanceColor;

layout (binding = 0) uniform UboView 
{
	mat4 projection;
	mat4 view; 
	vec3 camera;
} uboView;

layout (location = 0) out vec4 outCenterPosition;
layout (location = 1) out vec4 outColor;
layout (location = 2) out vec3 outCamera;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() 
{
	outCenterPosition = instancePositionAndRadius;
	outColor = instanceColor;
	outCamera = uboView.camera;
	vec4 objectPosition = vec4((inPos.xyz * instancePositionAndRadius.w + instancePositionAndRadius.xyz) , 1.0);
	
	gl_Position = uboView.projection * uboView.view * objectPosition;
}