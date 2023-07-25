#version 450

#extension GL_GOOGLE_include_directive : enable
#include "common.glsl"

layout(set = 0, binding = 1) uniform FragUniformBufferObject {
	vec3 albedo;
	float roughness;
	float metallic;
	float ao;
} frag_ubo;

layout (binding = 2) uniform sampler2D brdfLUT;
layout (binding = 3) uniform samplerCube irradianceMap;
layout (binding = 4) uniform samplerCube prefilterMap;

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inPosition;
layout (location = 3) in vec3 inLightPos;
layout (location = 4) in vec3 inCamPosition;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	vec3 albedo = frag_ubo.albedo;
	float roughness = frag_ubo.roughness;
	float metallic= frag_ubo.metallic;
	float ao = frag_ubo.ao;
	
	vec3 viewDir = normalize(inCamPosition - inPosition);
	vec3 R = reflect(-viewDir, inNormal); 
	
	vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);
	
	vec3 Lo = vec3(0.0);
	
	//for each light
	vec3 lightDir = normalize(inLightPos - inPosition); 
	vec3 N = normalize(inNormal);
	float distance = length(inLightPos - inPosition);
	float attenuation = 1.0 / (distance * distance);
    vec3 radiance = vec3(300.0,300.0,300.0) * attenuation;
	
	//BRDF
	Lo += BRDF(lightDir, viewDir, N, albedo, metallic, roughness) * radiance;
	
	//ibl ambient
	vec3 F = fresnelSchlickRoughness(max(dot(N, viewDir), 0.0), F0, roughness);
	vec3 irradiance = texture(irradianceMap, N).rgb;
    vec3 diffuse      = irradiance * albedo;
	// sample both the pre-filter map and the BRDF lut and combine them together as per the Split-Sum approximation to get the IBL specular part.
    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilteredColor = textureLod(prefilterMap, R,  roughness * MAX_REFLECTION_LOD).rgb;    
    vec2 brdf  = texture(brdfLUT, vec2(max(dot(N, viewDir), 0.0), roughness)).rg;
    vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);
 
    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;
	
	vec3 ambient = (kD * diffuse + specular) * ao;

    vec3 color = ambient + Lo;

    // HDR tonemapping
    color = color / (color + vec3(1.0));
    // gamma correct
    color = pow(color, vec3(1.0/2.2)); 	 
	
	outFragColor = vec4(color, 1.0);
}