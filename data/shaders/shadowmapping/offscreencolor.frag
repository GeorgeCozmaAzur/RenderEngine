#version 450

layout(set = 0, binding = 1) uniform FragUniformBufferObject {
	vec3 diffuse;
	float specular_power;
	float transparency;
} frag_ubo;

layout(location = 0) out vec4 color;
//layout(location = 0) out float fragmentdepth;

void main() 
{	
//	fragmentdepth = gl_FragCoord.z;
	vec4 own_color = vec4(frag_ubo.diffuse, frag_ubo.transparency);
	//if(frag_ubo.transparency < 1)
	//discard;
	color = own_color;
}