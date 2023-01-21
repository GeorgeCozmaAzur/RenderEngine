#version 450

#extension GL_GOOGLE_include_directive : enable
#include "../computeshader/common.glsl"

layout (binding = 1) uniform UBO 
{
	mat4 view_proj;
	vec4 bias_near_far_pow;
} ubo;

layout (binding = 2) uniform sampler2D samplerColor;
layout (binding = 3) uniform sampler2D sampleNormal;
layout (binding = 4) uniform sampler3D s_VoxelGrid;
layout (binding = 5) uniform sampler2D sampleShadow;
layout (binding = 6) uniform sampler2D sampleShadowColor;

layout (location = 0) in vec2 inUV;
layout (location = 1) in vec3 inLightPos;
layout (location = 2) in vec3 inPosition;
layout (location = 3) in vec3 inCamPosition;
layout (location = 4) in vec4 inShadowCoord;
layout (location = 5) in mat3 TBN;

layout (location = 0) out vec4 outFragColor;


vec3 add_inscattered_light(vec3 color, vec3 world_pos)
{
    vec3 uv = world_to_uv(world_pos, ubo.bias_near_far_pow.y, ubo.bias_near_far_pow.z, ubo.bias_near_far_pow.w, ubo.view_proj);

    vec4  scattered_light = texture(s_VoxelGrid, uv);
    float transmittance   = scattered_light.a;

    return color * transmittance * 1.0 + scattered_light.rgb;
}

vec3 textureProj(vec4 shadowCoord, vec2 off)
{
	vec3 shadow = vec3(1.0);
	if ( shadowCoord.z > -1.0 && shadowCoord.z < 1.0 ) 
	{
		float dist = texture( sampleShadow, shadowCoord.st + off ).r;
		vec3 shadowColor = texture( sampleShadowColor, shadowCoord.st + off ).rgb;
		
		if ( shadowCoord.w > 0.0 && dist < shadowCoord.z ) 
		{
			shadow = vec3(0.1);
		}
		
		shadow *= shadowColor;
	}
	return shadow;
}

void main() 
{
	vec3 normal = texture(sampleNormal, inUV).rgb;
	normal = normal * 2.0 - 1.0;   
	normal = normalize(TBN * normal); 

	float dist = length(inLightPos - inPosition);
	float att = 1.0 / (1.0 + 0.1*dist + 0.01*dist*dist);
	vec3 lightDir = normalize(inLightPos - inPosition);  
	float diff = max(dot(normal, lightDir), 0.0) * att * 10.0;
	
	vec3 viewDir = normalize(inCamPosition - inPosition);
	vec3 reflectDir = reflect(-lightDir, normal); 
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32); 
	
	vec3 shadow = textureProj(inShadowCoord / inShadowCoord.w, vec2(0.0));
	
	vec4 outColor = texture(samplerColor, inUV);
	outColor.rgb *= shadow;
	outColor.rgb = add_inscattered_light(outColor.rgb, inPosition);

	//outFragColor = (diff + spec) * texture(samplerColor, inUV) * shadow;//as it was
	outFragColor = outColor;
}