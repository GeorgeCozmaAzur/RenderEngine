#version 450

layout (location = 0) in vec4 inPos;
layout (location = 2) in vec3 inColor;
layout (location = 3) in vec3 inNormal;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 model;
	vec4 lightPos;
	vec4 camPos;
} ubo;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outColor;
layout (location = 2) out vec3 outEyePos;
layout (location = 3) out vec3 outLightVec;

void main() 
{
	outNormal = inNormal;
	outColor = inColor;
	vec4 pos =  ubo.model * inPos;
	float d = length(pos.xyz);
	
	pos = normalize(pos);
	pos.z = 1-pos.z;
	pos.x = pos.x/pos.z;
	pos.y = pos.y/pos.z;
	pos.w = 1.0;
	pos.z = (d-0.1)/(16.0-0.1);
	gl_Position = pos;
	
	outEyePos = vec3(ubo.model * inPos);
	outLightVec = normalize(ubo.lightPos.xyz - outEyePos);

	// Clip against reflection plane
	vec4 clipPlane = vec4(0.0, 0.0, -1.0, 0.0);	
	gl_ClipDistance[0] = dot(ubo.model * inPos, clipPlane);	
}
