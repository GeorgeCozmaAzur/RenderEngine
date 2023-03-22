#version 450

layout (binding = 2) uniform sampler2D samplerColor;
layout (binding = 3) uniform sampler2D sampleNormal;
layout (binding = 4) uniform sampler2D sampleDepth;
layout (binding = 5) uniform sampler2D sampleGloss;

layout (location = 0) in vec2 inUV;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inLightPos;
layout (location = 3) in vec3 inPosition;
layout (location = 4) in vec3 inCamPosition;

layout (location = 0) out vec4 outFragColor;

float heightScale = 0.05;

vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir)
{ 
    float height = 1.0 - textureLod(sampleDepth, texCoords, 0.0).r;    
	vec2 p = viewDir.xy / viewDir.z * (height * 0.05 -0.02);
    return texCoords - p;    
}

vec2 steepParallaxMapping(vec2 uv, vec3 viewDir) 
{
	const float minLayers = 8;
    const float maxLayers = 32;
	 float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));  
	// float numLayers = 32;
	float layerDepth = 1.0 / numLayers;
	float currentLayerDepth = 0.0;
	
	vec2 deltaTexCoords = viewDir.xy * heightScale / (viewDir.z * numLayers);
	
	vec2 currentTexCoords = uv;
	float currentDepthMapValue = 1.0 - textureLod(sampleDepth, currentTexCoords, 0.0).r;
	
	while(currentLayerDepth < currentDepthMapValue)
    {
        currentTexCoords -= deltaTexCoords;
        currentDepthMapValue = 1.0 - textureLod(sampleDepth, currentTexCoords, 0.0).r;  
        currentLayerDepth += layerDepth;  
    }
	
	return currentTexCoords;
} 

vec2 parallaxOcclusionMapping(vec2 uv, vec3 viewDir) 
{
	const float minLayers = 8;
    const float maxLayers = 48;
	float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));  
	//float numLayers = 48;
	float layerDepth = 1.0 / numLayers;
	float currentLayerDepth = 0.0;
	
	vec2 deltaTexCoords = viewDir.xy * heightScale / (viewDir.z * numLayers);
	
	vec2 currentTexCoords = uv;
	float currentDepthMapValue = 1.0 - textureLod(sampleDepth, currentTexCoords, 0.0).r;
	
	while(currentLayerDepth < currentDepthMapValue)
    {
        currentTexCoords -= deltaTexCoords;
        currentDepthMapValue = 1.0 - textureLod(sampleDepth, currentTexCoords, 0.0).r;  
        currentLayerDepth += layerDepth;  
    }

    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

    float afterDepth  = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = (1.0-texture(sampleDepth, prevTexCoords).r) - currentLayerDepth + layerDepth;
	
    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);
	
	
	return finalTexCoords;
} 

void main() 
{
	vec3 viewDir = normalize(inCamPosition - inPosition);
	
	vec2 texCoords = parallaxOcclusionMapping(inUV,  viewDir);
	
	// Discard fragments at texture border
	if (texCoords.x < 0.0 || texCoords.x > 1.0 || texCoords.y < 0.0 || texCoords.y > 1.0) {
		discard;
	}
	
	vec3 normal = texture(sampleNormal, texCoords).rgb;
	normal = normal * 2.0 - 1.0;   

	vec3 lightDir = normalize(inLightPos - inPosition);  
	float diff = max(dot(normal, lightDir), 0.0);
	
	
	vec3 reflectDir = reflect(-lightDir, normal); 
	float gloss = texture(sampleDepth, texCoords).r;
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32) * gloss;
	
	outFragColor = (spec + diff) * texture(samplerColor, texCoords);
}