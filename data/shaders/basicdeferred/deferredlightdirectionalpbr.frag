#version 450

#extension GL_GOOGLE_include_directive : enable
#include "../pbr/common.glsl"

layout (location = 0) in vec2 inUV;

layout(set = 0, binding = 0) uniform FragUniformBufferObject {
	mat4 depthMVP; 
	vec4 light_dir;
	vec3 camera_pos;
} frag_ubo;

layout (input_attachment_index = 0, binding = 1) uniform subpassInput samplerAlbedo;
layout (input_attachment_index = 1, binding = 2) uniform subpassInput samplerPosition;
layout (input_attachment_index = 2, binding = 3) uniform subpassInput samplerNormal;
layout (input_attachment_index = 3, binding = 4) uniform subpassInput samplerRM;
layout (binding = 5) uniform sampler2D sampleShadow;

layout (location = 0) out vec4 outFragColor;

float textureProj(vec4 shadowCoord, vec2 off)
{
	float shadow = 1.0;
	if ( shadowCoord.z > -1.0 && shadowCoord.z < 1.0 ) 
	{
		float dist = texture( sampleShadow, shadowCoord.st + off ).r;
		if ( shadowCoord.w > 0.0 && dist < shadowCoord.z ) 
		{
			shadow = 0.01;
		}
	}
	return shadow;
}

void main() 
{
	vec3 albedo = subpassLoad(samplerAlbedo).rgb;
	vec3 position = subpassLoad(samplerPosition).rgb;
	vec4 normalSample = subpassLoad(samplerNormal);
	vec4 rmSample = subpassLoad(samplerRM);
	vec3 normal = normalSample.rgb;
	float wehavenormal = normalSample.a;
	float metallic = rmSample.b;
	float roughness = rmSample.g;
	vec3 L = frag_ubo.light_dir.xyz;
	vec3 N = normalize(normal);
	vec3 viewDir = normalize(frag_ubo.camera_pos - position); 
	
	vec4 shadowCoord = (  frag_ubo.depthMVP ) * vec4(position, 1.0);	
	float shadow = textureProj(shadowCoord / shadowCoord.w, vec2(0.0));
	
	vec3 light = BRDF(L, viewDir, N, albedo, metallic, roughness) *10.0*shadow;
	light = light / (light + vec3(1.0));
    light = pow(light, vec3(1.0/2.2)); 
	
	outFragColor  = vec4(light, 1.0);	
}