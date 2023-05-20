#version 450

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

const float PI = 3.14159265359;
// ----------------------------------------------------------------------------
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}   
// ----------------------------------------------------------------------------

void main() 
{
	vec3 albedo = frag_ubo.albedo;
	//vec3 albedo = pow(texture(albedoColor, inUV).rgb, vec3(2.2));//frag_ubo.albedo
	float roughness = frag_ubo.roughness;
	//float roughness = texture(roughnessColor, inUV).r;//frag_ubo.roughness
	float metallic= frag_ubo.metallic;
	//float metallic = texture(metalicColor, inUV).r;//frag_ubo.metallic
	float ao = frag_ubo.ao;
	//float ao = texture(aoColor, inUV).r;//frag_ubo.ao
	
	vec3 viewDir = normalize(inCamPosition - inPosition);
	vec3 R = reflect(-viewDir, inNormal); 
	
	vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);
	
	vec3 Lo = vec3(0.0);
	
	//for each light
	vec3 lightDir = normalize(inLightPos - inPosition);  
	vec3 halfVec = normalize(viewDir + lightDir);
	float distance = length(inLightPos - inPosition);
	float attenuation = 1.0 / (distance * distance);
    vec3 radiance = vec3(300.0,300.0,300.0) * attenuation;
	
	//BRDF
	float NDF = DistributionGGX(inNormal, halfVec, roughness);   
    float G   = GeometrySmith(inNormal, viewDir, lightDir, roughness);      
    vec3 F    = fresnelSchlick(max(dot(halfVec, viewDir), 0.0), F0);
	
	vec3 numerator    = NDF * G * F; 
    float denominator = 4.0 * max(dot(inNormal, viewDir), 0.0) * max(dot(inNormal, lightDir), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
    vec3 specular = numerator / denominator;
	
	vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
	
	float NdotL = max(dot(inNormal, lightDir), 0.0);        

    Lo += (kD * albedo / PI + specular) * radiance * NdotL;	
	
	//ambient
	//vec3 ambient = vec3(0.03) * albedo * ao;
	//ibl ambient
	 F = fresnelSchlickRoughness(max(dot(inNormal, viewDir), 0.0), F0, roughness);
	vec3 irradiance = texture(irradianceMap, inNormal).rgb;
    vec3 diffuse      = irradiance * albedo;
	// sample both the pre-filter map and the BRDF lut and combine them together as per the Split-Sum approximation to get the IBL specular part.
    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilteredColor = textureLod(prefilterMap, R,  roughness * MAX_REFLECTION_LOD).rgb;    
    vec2 brdf  = texture(brdfLUT, vec2(max(dot(inNormal, viewDir), 0.0), roughness)).rg;
     specular = prefilteredColor * (F * brdf.x + brdf.y);

    vec3 ambient = (kD * diffuse + specular) * ao;
    
     kS = F;
     kD = 1.0 - kS;
    kD *= 1.0 - metallic;

    vec3 color = ambient + Lo;

    // HDR tonemapping
    color = color / (color + vec3(1.0));
    // gamma correct
    color = pow(color, vec3(1.0/2.2)); 
	
	//vec3 N=normalize(inNormal);
	//N.y=-N.y;
	//color = texture(prefilterMap, reflect(-viewDir, inNormal)).rgb;	 
	
	outFragColor = vec4(color, 1.0);
	//outFragColor = vec4(specular, 1.0);
	//outFragColor = vec4(G);
}