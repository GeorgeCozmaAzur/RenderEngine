#version 450

#extension GL_GOOGLE_include_directive : enable
#include "../computeshader/common.glsl"

layout (binding = 1) uniform UBO 
{
	mat4 view_proj;
	vec4 bias_near_far_pow;
} ubo;

layout(binding = 2) uniform FragUniformBufferObject {
	vec3 diffuse;
	float specular_power;
	float transparency;
} frag_ubo;

layout (binding = 3) uniform sampler3D s_VoxelGrid;
layout (binding = 4) uniform sampler2D sampleShadow;
layout (binding = 4) uniform sampler2D sampleShadowColor;

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inPosition;
layout (location = 3) in vec3 inLightPos;
layout (location = 4) in vec3 inCamPosition;
layout (location = 5) in vec4 inShadowCoord;

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
	vec3 lightDir = normalize(inLightPos - inPosition);  
	float diff = max(dot(inNormal, lightDir), 0.0);
	
	vec3 viewDir = normalize(inCamPosition - inPosition);
	vec3 reflectDir = reflect(-lightDir, inNormal); 
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), frag_ubo.specular_power); 
	
	vec3 shadow = textureProj(inShadowCoord / inShadowCoord.w, vec2(0.0));

	vec4 own_color = vec4(frag_ubo.diffuse, frag_ubo.transparency);
	own_color.rgb *= shadow;
	own_color.rgb = add_inscattered_light(own_color.rgb, inPosition);
	//own_color.b=1.0;
	
	outFragColor = own_color;
}