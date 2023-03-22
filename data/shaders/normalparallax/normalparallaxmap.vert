#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inTangent;
layout (location = 4) in vec3 inBitangent;


layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 view;
	vec4 light_pos;
	vec3 camera_pos;
} ubo;

layout (binding = 1) uniform OUBO 
{
	mat4 model;
} oubo;

layout (location = 0) out vec2 outUV;
layout (location = 1) out vec3 outNormal;
layout (location = 2) out vec3 outLightPos;
layout (location = 3) out vec3 outPosition;
layout (location = 4) out vec3 outCamPosition;

void main() 
{
	outUV = inUV;
	outNormal = inNormal;
	
	vec3 T = normalize( mat3(oubo.model) * inTangent );
	vec3 B = normalize( mat3(oubo.model) * inBitangent );
	vec3 N = normalize( mat3(oubo.model) * inNormal );
	
	mat3 TBN = transpose(mat3(T, B, N));
	
	outPosition = TBN * vec3(oubo.model * vec4(inPos, 1.0));;
	outLightPos = TBN * ubo.light_pos.xyz;
	outCamPosition = TBN * ubo.camera_pos;
   
	gl_Position = ubo.projection * ubo.view * oubo.model * vec4(inPos.xyz, 1.0);
}
