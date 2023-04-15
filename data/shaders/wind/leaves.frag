#version 450

layout (binding = 2) uniform sampler2D samplerColor;
//layout (binding = 3) uniform sampler2D perlin;

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inPosition;
layout (location = 3) in vec3 inLightPos;
layout (location = 4) in vec3 inCamPosition;

layout (location = 0) out vec4 outFragColor;

layout (binding = 3) uniform UboModel 
{
	float advance;
} uboModel;

void main() 
{
	float scale = 0.01;
	/*vec4 p = texture(perlin, inUV * uboModel.advance);
	float xCoord = inUV.x + p.x * scale;
	float yCoord = inUV.y + p.x * scale;*/

	vec4 tex_color = texture(samplerColor, inUV);
	if (tex_color.a < 0.5) {
		discard;
	}
	
	vec3 lightDir = normalize(inLightPos - inPosition);  
	float diff = max(dot(inNormal, lightDir), 0.0);
	
	vec3 viewDir = normalize(inCamPosition - inPosition);
	vec3 reflectDir = reflect(-lightDir, inNormal); 
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
	//vec3 specular = specularStrength * spec * lightColor;  

	//outFragColor = vec4(inCamPosition, 1.0);
	//vec4 own_color = vec4(frag_ubo.diffuse, frag_ubo.transparency);
	outFragColor = (spec + diff) * tex_color;
}