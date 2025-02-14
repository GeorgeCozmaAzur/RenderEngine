#version 450

#extension GL_GOOGLE_include_directive : enable
#include "../pbr/common.glsl"

layout (location = 0) in vec4 inCenterPosition;
layout (location = 1) in vec4 inColor;
layout (location = 2) in vec3 inCamera;

layout (input_attachment_index = 0, binding = 1) uniform subpassInput samplerAlbedo;
layout (input_attachment_index = 1, binding = 2) uniform subpassInput samplerPosition;
layout (input_attachment_index = 2, binding = 3) uniform subpassInput samplerNormal;
layout (input_attachment_index = 3, binding = 4) uniform subpassInput samplerRM;

layout (location = 0) out vec4 outFragColor;

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

	vec3 L = inCenterPosition.xyz - position;
	// Distance from light to fragment position
	float dist = length(L);
	float atten = inCenterPosition.w / (pow(dist, 2.0) + 1.0);
	
	L = normalize(L);
	vec3 N = normalize(normal);
	float NdotL = wehavenormal * max(0.1, dot(N, L));
	float intensity = NdotL * atten;
	
	vec3 viewDir = normalize(inCamera - position); 
	
	//vec3 light = inColor.rgb * intensity;
	
	float attenuation = 1.0 / (dist * dist);
    vec3 radiance = vec3(30.0,30.0,30.0) * attenuation;
	vec3 light = BRDF(L, viewDir, N, albedo, metallic, roughness) * radiance;
	
	 // HDR tonemapping
    light = light / (light + vec3(1.0));
    // gamma correct
    light = pow(light, vec3(1.0/2.2)); 
	
	float alpha = clamp(1.0-dist/inCenterPosition.w, 0.0, 1.0);
	
	outFragColor  = vec4(light, alpha);	
}