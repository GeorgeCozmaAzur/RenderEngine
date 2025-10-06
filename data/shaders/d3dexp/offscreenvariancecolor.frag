#version 450

layout(location = 0) out vec4 fragmentdepth;

void main() 
{	
	float depth = gl_FragCoord.z;
	
	float dx = dFdx(depth);
	float dy = dFdy(depth);
	float m2 = depth * depth;// * 0.25 * (dx*dx+dy*dy);
	fragmentdepth = vec4(depth, m2, 0.0,0.0);
}