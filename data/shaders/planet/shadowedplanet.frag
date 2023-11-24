#version 450

layout (binding = 2) uniform sampler2D samplerColor;
layout (binding = 3) uniform sampler2D shadowMap;

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inPosition;
layout (location = 3) in vec3 inPositionInverse;
layout (location = 4) in vec3 inLightPos;
layout (location = 5) in vec3 inCamPosition;
layout (location = 6) in vec4 inShadowCoord;

layout (location = 0) out vec4 outFragColor;
layout (location = 1) out vec4 outFragPositions;

#define ambient 0.25

float textureProj(vec4 shadowCoord, vec2 off)
{
	float shadow = 1.0;
	if ( shadowCoord.z > -1.0 && shadowCoord.z < 1.0 ) 
	{
		float dist = texture( shadowMap, shadowCoord.st + off ).r;
		if ( shadowCoord.w > 0.0 && dist < shadowCoord.z ) 
		{
			float pul = shadowCoord.z * 0.5 + 0.5;
			float distt = pul;
			float finalshadow = mix(shadow, ambient, distt);
			shadow = finalshadow;
		}
	}
	return shadow;
}

void main() 
{
	vec4 tex_color = texture(samplerColor, inUV);
	
	float shadow = textureProj(inShadowCoord / inShadowCoord.w, vec2(0.0));

	
	vec3 lightDir = normalize(inLightPos - inPosition);  
	float diff = max(dot(inNormal, lightDir), 0.0);
	
	//vec3 viewDir = normalize(inCamPosition - inPosition);
	//vec3 reflectDir = reflect(-lightDir, inNormal); 
	//float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
	//vec3 specular = specularStrength * spec * lightColor;  

	//outFragColor = vec4(inCamPosition, 1.0);
	vec4 own_color = (( diff * shadow) * tex_color);// + vec4(0.0,0.2,0.5,0.0);
	outFragColor = own_color;
	//outFragColor = vec4(inUV.x, own_colorinUV.y, 0.0,1.0);
	outFragPositions  = vec4(inPositionInverse, 1.0);
}