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
	mat4 lightSpace;
	vec4 light_pos;
	vec3 camera_pos;
} ubo;

/*
layout (binding = 1) uniform OUBO 
{
	mat4 model;
} oubo;*/

layout (location = 0) out vec2 outUV;
layout (location = 1) out vec4 outShadowCoord;
layout (location = 2) out vec3 outLightPos;
layout (location = 3) out vec3 outPosition;
layout (location = 4) out vec3 outCamPosition;
layout (location = 5) out mat3 TBN;

out gl_PerVertex 
{
    vec4 gl_Position;   
};

void main() 
{
	const mat4 biasMat = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 );
	
	outUV = inUV;
	outPosition = inPos;
	outLightPos = ubo.light_pos.xyz;
	outCamPosition = ubo.camera_pos;
	
	vec3 T = normalize(vec3( vec4(inTangent,   0.0)));
    vec3 B = normalize(vec3( vec4(inBitangent, 0.0)));
    vec3 N = normalize(vec3( vec4(inNormal,    0.0)));
    TBN = mat3(T, B, N);
	
	outShadowCoord = (  ubo.lightSpace ) * vec4(inPos, 1.0);	
   
	gl_Position = ubo.projection * ubo.view * vec4(inPos.xyz, 1.0);
	//gl_Position = ubo.lightSpace * vec4(inPos.xyz, 1.0);
}
