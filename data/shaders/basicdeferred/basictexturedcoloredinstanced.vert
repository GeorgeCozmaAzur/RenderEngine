#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inColor;
layout (location = 3) in vec3 inNormal;

layout (location = 4) in vec3 instancePos;

layout (binding = 0) uniform UboView 
{
	mat4 projection;
	mat4 view; 
	mat4 model; 
} uboView;

layout (location = 0) out vec2 outUV;
layout (location = 1) out vec3 outColor;
layout (location = 2) out vec3 outPosition;
layout (location = 3) out vec3 outNormal;


out gl_PerVertex {
	vec4 gl_Position;
};

void main() 
{
	outUV = inUV;
	outColor = inColor;
	outNormal = inNormal;
	outPosition = vec3(uboView.model * vec4(inPos.xyz, 1.0));
	
	gl_Position = uboView.projection * uboView.view * uboView.model * vec4(inPos.xyz + instancePos, 1.0);
}