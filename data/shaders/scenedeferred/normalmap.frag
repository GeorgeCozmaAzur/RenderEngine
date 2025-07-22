#version 450

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
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inPosition;
layout (location = 3) in vec4 inTangent;

layout (location = 0) out vec4 outFragColor;
layout (location = 1) out vec4 outFragPositions;
layout (location = 2) out vec4 outFragNormals;
layout (location = 3) out vec4 outFragRoughMetallic;

vec3 calculateNormal()
{
	vec3 tangentNormal = texture(normalsSampler, inUV).xyz * 2.0 - 1.0;

	vec3 N = normalize(inNormal);
	vec3 T = normalize(inTangent.xyz);
	vec3 B = normalize(cross(N, T));//cross(N, T) * inTangent.w;//normalize(cross(N, T));
	mat3 TBN = mat3(T, B, N);
	return normalize(TBN * tangentNormal);
}

void main() 
{
	vec3 albedo = pow(texture(albedoSampler, inUV).rgb, vec3(2.2)) * frag_ubo.baseColorFactor;
	vec4 rm = texture(roughnessMetalicSampler, inUV);
	float roughness = rm.g * frag_ubo.roughnessFactor;
	float metallic = rm.b * frag_ubo.metallicFactor;
	
	vec3 N = calculateNormal();
	
	outFragColor = vec4(albedo, 1.0);
	outFragPositions = vec4(inPosition, 1.0);
	outFragNormals = vec4(N, 1.0);
	outFragRoughMetallic = vec4(1.0,roughness,metallic, 1.0);

}