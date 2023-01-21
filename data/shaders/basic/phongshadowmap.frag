#version 450

layout(set = 0, binding = 1) uniform FragUniformBufferObject {
	vec3 diffuse;
	float specular_power;
	float transparency;
} frag_ubo;

layout (binding = 2) uniform sampler2D sampleShadow;

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inPosition;
layout (location = 3) in vec3 inLightPos;
layout (location = 4) in vec3 inCamPosition;
layout (location = 5) in vec4 inShadowCoord;

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
	vec3 lightDir = normalize(inLightPos - inPosition);  
	float diff = max(dot(inNormal, lightDir), 0.0);
	
	vec3 viewDir = normalize(inCamPosition - inPosition);
	vec3 reflectDir = reflect(-lightDir, inNormal); 
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), frag_ubo.specular_power);
	//vec3 specular = specularStrength * spec * lightColor;  
	
	float shadow = textureProj(inShadowCoord / inShadowCoord.w, vec2(0.0));

	//outFragColor = vec4(inCamPosition, 1.0);
	vec4 own_color = vec4(frag_ubo.diffuse, frag_ubo.transparency);
	outFragColor = (spec + diff) * own_color * shadow;
}