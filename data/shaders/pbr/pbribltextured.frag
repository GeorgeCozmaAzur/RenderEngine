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

layout (binding = 5) uniform sampler2D albedoColor;
layout (binding = 6) uniform sampler2D normalMap;
layout (binding = 7) uniform sampler2D roughnessColor;
layout (binding = 8) uniform sampler2D metalicColor;
layout (binding = 9) uniform sampler2D aoColor;

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inTangent;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inPosition;
layout (location = 4) in vec3 inLightPos;
layout (location = 5) in vec3 inCamPosition;

layout (location = 0) out vec4 outFragColor;

vec3 calculateNormal()
{
	vec3 tangentNormal = texture(normalMap, inUV).xyz * 2.0 - 1.0;

	vec3 N = normalize(inNormal);
	vec3 T = normalize(inTangent.xyz);
	vec3 B = normalize(cross(N, T));
	mat3 TBN = mat3(T, B, N);
	return normalize(TBN * tangentNormal);
}

void main() 
{
	vec3 albedo = pow(texture(albedoColor, inUV).rgb, vec3(2.2));
	float roughness = texture(roughnessColor, inUV).r;
	float metallic = texture(metalicColor, inUV).r;
	float ao = texture(aoColor, inUV).r;
	
	vec3 N = calculateNormal();
	
	vec3 viewDir = normalize(inCamPosition - inPosition);
	vec3 R = reflect(-viewDir, N); 
	
	vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);
	
	vec3 Lo = vec3(0.0);
	
	//for each light
	vec3 lightDir = normalize(inLightPos - inPosition);  
	float distance = length(inLightPos - inPosition);
	float attenuation = 1.0 / (distance * distance);
    vec3 radiance = vec3(300.0,300.0,300.0) * attenuation;
	
	//BRDF
	Lo += BRDF(lightDir, viewDir, N, albedo, metallic, roughness) * radiance;
	
	//ambient
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