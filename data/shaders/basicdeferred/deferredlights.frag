#version 450

layout (location = 0) in vec4 inCenterPosition;
layout (location = 1) in vec4 inColor;

layout (input_attachment_index = 0, binding = 1) uniform subpassInput samplerPosition;
layout (input_attachment_index = 1, binding = 2) uniform subpassInput samplerNormal;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	vec3 position = subpassLoad(samplerPosition).rgb;
	vec4 normalSample = subpassLoad(samplerNormal);
	vec3 normal = normalSample.rgb;
	float wehavenormal = normalSample.a;

	vec3 L = inCenterPosition.xyz - position;
	// Distance from light to fragment position
	float dist = length(L);
	float atten = inCenterPosition.w / (pow(dist, 2.0) + 1.0);
	
	L = normalize(L);
	vec3 N = normalize(normal);
	float NdotL = wehavenormal * max(0.1, dot(N, L));
	float intensity = NdotL * atten;
	vec3 light = inColor.rgb * intensity;
	
	outFragColor  = vec4(light, intensity);	
}