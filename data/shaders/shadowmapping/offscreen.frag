#version 450

layout(location = 0) out vec4 color;
//layout(location = 0) out float fragmentdepth;

void main() 
{	
//	fragmentdepth = gl_FragCoord.z;
	color = vec4(0.1, 0.1, 0.1, 1.0);
}