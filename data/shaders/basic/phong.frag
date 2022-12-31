#version 450

layout(set = 0, binding = 1) uniform FragUniformBufferObject {
	vec3 diffuse;
	float specular_power;
	float transparency;
} frag_ubo;

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inPosition;
layout (location = 3) in vec3 inLightPos;
layout (location = 4) in vec3 inCamPosition;

layout (location = 0) out vec4 outFragColor;


void main() 
{
	vec3 lightDir = normalize(inLightPos - inPosition);  
	float diff = max(dot(inNormal, lightDir), 0.0);
	
	vec3 viewDir = normalize(inCamPosition - inPosition);
	vec3 reflectDir = reflect(-lightDir, inNormal); 
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), frag_ubo.specular_power);
	//vec3 specular = specularStrength * spec * lightColor;  

	//outFragColor = vec4(inCamPosition, 1.0);
	vec4 own_color = vec4(frag_ubo.diffuse, frag_ubo.transparency);
	outFragColor = (spec + diff) * own_color;
}