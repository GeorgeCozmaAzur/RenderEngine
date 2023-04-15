#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;

layout (binding = 0) uniform UboView 
{
	mat4 projection;
	mat4 view; 
	vec4 light_pos;
	vec3 camera_pos;
} ubo;

layout (binding = 1) uniform UboModel 
{
	vec4 wind;
	float advance;
} uboModel;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec2 outUV;
layout (location = 2) out vec3 outPos;
layout (location = 3) out vec3 outLightPos;
layout (location = 4) out vec3 outCamPos;

out gl_PerVertex {
	vec4 gl_Position;
};

vec3 ToSphere(vec3 point)
{
	float r = sqrt(point.x * point.x + point.y * point.y + point.z * point.z);
	float theta = acos(point.z/r);
	float phi = atan(point.y, point.x);
	return vec3(r, theta, phi);
}

vec3 ToCartesian(vec3 spoint)
{
	return vec3(spoint.r * sin(spoint.g) * cos(spoint.b) ,
				spoint.r * sin(spoint.g) * sin(spoint.b) ,
				spoint.r * cos(spoint.g));
}

void main() 
{
	outNormal = inNormal;
	outUV = inUV;
	outPos = inPos;
	outLightPos = ubo.light_pos.xyz;
	outCamPos = ubo.camera_pos;
	
	vec3 finalPos = inPos;
	vec3 dirfromcenter = normalize(inPos);
	vec3 plusvec = normalize(uboModel.wind.xyz + dirfromcenter);
	
	vec3 splusvec = ToSphere(plusvec);
	
	vec3 xzpoint = vec3(inPos.x, inPos.y * 0.25, inPos.z);
	float distfromcenterfactor = length(xzpoint) * 0.5;
	float windpower = sin(uboModel.advance) * uboModel.wind.a;
	windpower = mix(0, windpower, distfromcenterfactor);
	vec3 s = ToSphere(finalPos);
	s.g = mix(s.g, splusvec.g, windpower);
	s.b = mix(s.b, splusvec.b, windpower);
	finalPos = ToCartesian(s);
	
	gl_Position = ubo.projection * ubo.view * vec4(finalPos.xyz, 1.0);
}