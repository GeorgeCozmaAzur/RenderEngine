#version 450

layout (binding = 1) uniform sampler2D samplerColor;
layout (binding = 2) uniform sampler2D frontParaboloidMap;
layout (binding = 3) uniform sampler2D backParaboloidMap;
layout (binding = 4) uniform samplerCube envMap;

layout (location = 0) in vec2 inUV;
layout (location = 1) in vec4 inPos;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec4 inEyePos;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	vec3 normal = normalize(inNormal);
	vec3 viewDir = normalize(inPos.xyz - inEyePos.xyz);
	vec3 R = reflect(viewDir, normal);

	vec4 color = texture(samplerColor, inUV);
	float a=0.05;
	float b=0.95;

	vec2 puv;
	puv.x = R.x/(1.0-R.z);
	puv.y = R.y/(1.0-R.z);
	puv *= vec2(0.5);
	puv += vec2(0.5);
	puv.x = a + puv.x * (b-a);
	vec4 colorf = texture(frontParaboloidMap,puv);

	vec2 puvb;
	puvb.x = R.x/(1.0+R.z);
	puvb.y = R.y/(1.0+R.z);
	puvb.x = -puvb.x;
	puvb *= vec2(0.5);
	puvb += vec2(0.5);
	puvb.x = a + puvb.x * (b-a);
	vec4 colorb = texture(backParaboloidMap,puvb);
	
	float zBias = 0.0;
	float f = step(R.z+zBias, 0.0);
	
	vec4 color1 =mix(colorb, colorf, f);
	//vec4 testcolor1 =mix(vec4(1.0,0.0,0.0,1.0), vec4(0.0,1.0,0.0,1.0), f);

	//vec4 color1 = texture(envMap, R);
	outFragColor = color * 0.25 + color1 * 0.75;

	
}