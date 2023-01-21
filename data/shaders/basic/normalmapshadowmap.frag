#version 450

layout (binding = 1) uniform sampler2D samplerColor;
layout (binding = 2) uniform sampler2D sampleNormal;
layout (binding = 3) uniform sampler2D sampleShadow;

layout (location = 0) in vec2 inUV;
layout (location = 1) in vec3 inLightPos;
layout (location = 2) in vec3 inPosition;
layout (location = 3) in vec3 inCamPosition;
layout (location = 4) in vec4 inShadowCoord;
layout (location = 5) in mat3 TBN;

layout (location = 0) out vec4 outFragColor;

float textureProj(vec4 shadowCoord, vec2 off)
{
	float shadow = 1.0;
	if ( shadowCoord.z > -1.0 && shadowCoord.z < 1.0 ) 
	{
		float dist = texture( sampleShadow, shadowCoord.st + off ).r;
		if ( shadowCoord.w > 0.0 && dist < shadowCoord.z ) 
		{
			shadow = 0.1;
		}
	}
	return shadow;
}

void main() 
{
	vec3 normal = texture(sampleNormal, inUV).rgb;
	normal = normal * 2.0 - 1.0;   
	normal = normalize(TBN * normal); 

	float dist = length(inLightPos - inPosition) * 0.05;
	float att = 1.0 / (1.0 + 0.1*dist + 0.01*dist*dist);
	vec3 lightDir = normalize(inLightPos - inPosition);  
	float diff = max(dot(normal, lightDir), 0.0) * att;
	
	vec3 viewDir = normalize(inCamPosition - inPosition);
	vec3 reflectDir = reflect(-lightDir, normal); 
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
	//vec3 specular = specularStrength * spec * lightColor;  
	
	float shadow = textureProj(inShadowCoord / inShadowCoord.w, vec2(0.0));

	//outFragColor = texture(sampleNormal, inUV);//vec4(inCamPosition, 1.0);
	outFragColor = (diff + spec) * texture(samplerColor, inUV) * shadow;//as it was
	//outFragColor = texture(samplerColor, inUV) * shadow;//as it was
}