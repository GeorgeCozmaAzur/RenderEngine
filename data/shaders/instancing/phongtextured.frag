#version 450

layout (binding = 1) uniform sampler2D samplerColor;

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inPosition;
layout (location = 3) in vec3 inLightPos;
layout (location = 4) in vec3 inCamPosition;

layout (location = 0) out vec4 outFragColor;


void main() 
{
	vec4 tex_color = texture(samplerColor, inUV);
	if (tex_color.a < 0.5) {
		discard;
	}
	
	vec3 lightDir = normalize(inLightPos - inPosition);  
	vec3 N = normalize(inNormal);
	float diff = max(dot(N, lightDir), 0.0);
	vec3 viewDir = normalize(inCamPosition - inPosition);
	vec3 reflectDir = reflect(-lightDir, N); 
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);

	outFragColor = ( diff + spec ) * tex_color;
}