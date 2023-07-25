#version 450

#extension GL_GOOGLE_include_directive : enable
#include "common.glsl"

layout(set = 0, binding = 1) uniform FragUniformBufferObject {
	vec4 albedo;
	vec4 lightPosition;
	vec4 cameraPosition;
	float roughness;
	float metallic;
	float ao;
} frag_ubo;

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inPosition;
layout (location = 3) in vec3 inLightPos;
layout (location = 4) in vec3 inCamPosition;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	vec3 albedo = frag_ubo.albedo.xyz;
	float roughness = frag_ubo.roughness;
	float metallic= frag_ubo.metallic;
	float ao = frag_ubo.ao;
	
	vec3 viewDir = normalize(frag_ubo.cameraPosition.xyz - inPosition);
	vec3 N = normalize(inNormal);
	
	vec3 Lo = vec3(0.0);
	
	//for each light
	vec3 lightDir = normalize(frag_ubo.lightPosition.xyz - inPosition);  
	float distance = length(frag_ubo.lightPosition.xyz - inPosition);
	float attenuation = 1.0 / (distance * distance);
    vec3 radiance = vec3(300.0,300.0,300.0) * attenuation;
	Lo += BRDF(lightDir, viewDir, N, albedo, metallic, roughness) * radiance;
	
	//ambient
	vec3 ambient = vec3(0.03) * albedo * ao;

    vec3 color = ambient + Lo;
	
    // HDR tonemapping
    color = color / (color + vec3(1.0));
    // gamma correct
    color = pow(color, vec3(1.0/2.2)); 
	
	outFragColor = vec4(color, 1.0);
	//outFragColor = vec4(G);
}