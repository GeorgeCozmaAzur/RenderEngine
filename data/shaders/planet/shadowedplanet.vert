#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;

layout (binding = 0) uniform UboView 
{
	mat4 projection;
	mat4 view; 
	mat4 lightSpace;
	vec4 light_pos;
	vec3 camera_pos;
} ubo;

layout (binding = 1) uniform OUBO 
{
	mat4 model;
} oubo;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec2 outUV;
layout (location = 2) out vec3 outPos;
layout (location = 3) out vec3 outPosInverse;
layout (location = 4) out vec3 outLightPos;
layout (location = 5) out vec3 outCamPos;
layout (location = 6) out vec4 outShadowCoord;

out gl_PerVertex {
	vec4 gl_Position;
};

const mat4 biasMat = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 );
	
void main() 
{
	outNormal = mat3(oubo.model) * inNormal;
	outUV = inUV;
	outPos = vec3(oubo.model * vec4(inPos.xyz, 1.0));
	outPosInverse = vec3(inverse(oubo.model) * vec4(inPos.xyz, 1.0));
	outLightPos = ubo.light_pos.xyz;
	outCamPos = ubo.camera_pos;
	
	outShadowCoord = ( biasMat * ubo.lightSpace * oubo.model ) * vec4(inPos, 1.0);	
	
	gl_Position = ubo.projection * ubo.view * oubo.model * vec4(inPos.xyz, 1.0);
}