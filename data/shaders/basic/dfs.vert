#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;

layout (binding = 0) uniform UboView 
{
	mat4 projection;
	mat4 view;
	//mat4 lightSpace;
	vec4 light_pos;
	vec3 camera_pos;
} ubo;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec2 outUV;
layout (location = 2) out vec3 outPos;
layout (location = 3) out vec3 outLightPos;
layout (location = 4) out vec3 outCamPos;
//layout (location = 5) out vec4 outShadowCoord;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() 
{
	outNormal = inNormal;
	outUV = inUV;
	outPos = inPos;
	outLightPos = ubo.light_pos.xyz;
	outCamPos = ubo.camera_pos;
	//outShadowCoord = (  ubo.lightSpace ) * vec4(inPos, 1.0);	
	
	gl_Position = ubo.projection * ubo.view * vec4(inPos.xyz, 1.0);
}