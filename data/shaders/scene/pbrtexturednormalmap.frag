#version 450

#extension GL_GOOGLE_include_directive : enable
#include "../pbr/common.glsl"

layout(set = 0, binding = 1) uniform FragUniformBufferObject {
	float baseColorFactor;
	float metallicFactor;
	float roughnessFactor;
	float aoFactor;
} frag_ubo;

layout (binding = 2) uniform sampler2D albedoSampler;
layout (binding = 3) uniform sampler2D roughnessMetalicSampler;
layout (binding = 4) uniform sampler2D normalsSampler;

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec4 inTangent;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inPosition;
layout (location = 4) in vec3 inLightPos;
layout (location = 5) in vec3 inCamPosition;

layout (location = 0) out vec4 outFragColor;

vec3 calculateNormal()
{
	vec3 tangentNormal = texture(normalsSampler, inUV).xyz * 2.0 - 1.0;

	vec3 N = normalize(inNormal);
	vec3 T = normalize(inTangent.xyz);
	vec3 B = cross(N, T) * inTangent.w;//normalize(cross(N, T));//cross(N, T) * inTangent.w;//normalize(cross(N, T));
	mat3 TBN = mat3(T, B, N);
	return normalize(TBN * tangentNormal);
}

void main() 
{
	vec3 albedo = pow(texture(albedoSampler, inUV).rgb, vec3(2.2)) * frag_ubo.baseColorFactor;
	vec4 rm = texture(roughnessMetalicSampler, inUV);
	float roughness = rm.g * frag_ubo.roughnessFactor;
	float metallic = rm.b * frag_ubo.metallicFactor;
	
	vec3 viewDir = normalize(inCamPosition - inPosition);
	vec3 N = calculateNormal();//normalize(inNormal);
	
	vec3 Lo = vec3(0.0);
	
	//for each light
	vec3 lightDir = normalize(inLightPos - inPosition);  
	float distance = length(inLightPos - inPosition);
	float attenuation = 1.0 / (distance * distance);
    vec3 radiance = vec3(3.0,3.0,3.0) * attenuation;
	Lo += BRDF(lightDir, viewDir, N, albedo, metallic, roughness) * radiance;
	
	//ambient
	vec3 ambient = vec3(frag_ubo.aoFactor) * albedo;

    vec3 color = ambient + Lo;

    // HDR tonemapping
    color = color / (color + vec3(1.0));
    // gamma correct
    color = pow(color, vec3(1.0/2.2)); 
	
	outFragColor = vec4(color, 1.0);
	//outFragColor = vec4(inTangent.xyz, 1.0);
	//outFragColor = vec4(roughness);
}