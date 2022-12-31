#version 450

layout (location = 0) in vec2 inUV;

layout (binding = 0) uniform sampler2D samplerColor;
layout (binding = 1) uniform sampler2D samplePositions;
layout (binding = 2) uniform sampler2D sampleNormals;

struct Light {
	vec4 position;
	vec3 color;
	float radius;
};
#define lightCount 50
layout (binding = 3) uniform UBO 
{
	Light lights[lightCount];
	vec4 viewPos;
	int displayDebugTarget;
} ubo;

layout (location = 0) out vec4 outFragColor;


void main() 
{
	vec3 fragPos = texture(samplePositions, inUV).rgb;
	vec3 N = normalize(texture(sampleNormals, inUV).rgb);
	vec4 tex_color = texture(samplerColor, inUV) * 0.1;
	
	for(int i = 0; i < lightCount; ++i)
	{
		vec3 L = ubo.lights[i].position.xyz - fragPos;
		// Distance from light to fragment position
		float dist = length(L);
		float atten = ubo.lights[i].radius / (pow(dist, 2.0) + 1.0);
		float NdotL = max(0.0, dot(N, L));
		
		tex_color += NdotL * atten * vec4(ubo.lights[i].color, 1.0);
	}
	outFragColor  = tex_color;
	//outFragColor  = texture(samplePositions, inUV);	
}