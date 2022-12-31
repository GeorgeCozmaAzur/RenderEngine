#version 450

layout (binding = 1) uniform sampler2D samplerColor;
layout (binding = 2) uniform sampler2D sampleNormal;

layout (location = 0) in vec2 inUV;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inLightPos;
layout (location = 3) in vec3 inPosition;
layout (location = 4) in vec3 inCamPosition;
layout (location = 5) in mat3 TBN;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	vec3 normal = texture(sampleNormal, inUV).rgb;
	normal = normal * 2.0 - 1.0;   
	normal = normalize(TBN * normal); 

	vec3 lightDir = normalize(inLightPos - inPosition);  
	float diff = max(dot(normal, lightDir), 0.0);
	
	vec3 viewDir = normalize(inCamPosition - inPosition);
	vec3 reflectDir = reflect(-lightDir, normal); 
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
	//vec3 specular = specularStrength * spec * lightColor;  

	//outFragColor = texture(sampleNormal, inUV);//vec4(inCamPosition, 1.0);
	outFragColor = (spec + diff) * texture(samplerColor, inUV);
}