#version 450

#define M_PI 3.1415926535897932384626433832795

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inPosition;
layout (location = 3) in vec3 inLightPos;
layout (location = 4) in vec3 inCamPosition;

layout (location = 0) out vec4 outFragColor;
layout (location = 1) out vec4 outFragPositions;

layout (binding = 2) uniform UboView 
{
	
	vec4	sun;//poistion and intensity of the sun
	vec4	cameraPosition;
	vec4	viewDirection;
	vec4	dimensions;//radius of the planet, radius of the atmosphere, Rayleigh scale height, Mie scale height
	vec4	scatteringCoefficients;//Rayleigh and Mie scattering coefficiants
	float g;        // Mie scattering direction wip
	
} ubo;

int viewSamples = 16;
int lightSamples = 16;

vec2 raySphereIntersection(vec3 sphereCenter, vec3 o, vec3 d, float r)
{
	vec3 offset = o - sphereCenter;
    // Solving analytically as a quadratic function
    //  assumes that the sphere is centered at the origin
    // f(x) = a(x^2) + bx + c
    float a = dot(d, d);
    float b = 2.0 * dot(d, offset);
    float c = dot(offset, offset) - r * r;

    // Discriminant or delta
    float delta = b * b - 4.0 * a * c;

    // Roots not found
    if (delta < 0.0) {
      // TODO
      return vec2(1e5, -1e5);
    }

    float sqrtDelta = sqrt(delta);
    // TODO order??
    return vec2((-b - sqrtDelta) / (2.0 * a),
                (-b + sqrtDelta) / (2.0 * a));
}

vec3 computeSkyColor(vec3 ray, vec3 origin)
{
    vec3 sunDir = normalize(ubo.sun.xyz - inPosition);
	float I_sun = ubo.sun.w;

	float PlanetRadius = ubo.dimensions.x;
	float AtmosphereRadius = ubo.dimensions.y;
	float rayleighHeight = ubo.dimensions.z;
	float mieHeight = ubo.dimensions.w;
	
    vec2 t = raySphereIntersection(vec3(0,0,0), origin, ray, AtmosphereRadius);
    // Intersects behind
    if (t.x > t.y) {
        return vec3(1.0, 0.0, 0.0);
    }
	
    t.y = min(t.y, raySphereIntersection(vec3(0,0,0), origin, ray, PlanetRadius).x);
    float segmentLen = (t.y - t.x) / float(viewSamples);

    float tCurrent = 0.0f; 

    vec3 sum_R = vec3(0.0);
    vec3 sum_M = vec3(0.0);

    float optDepth_R = 0.0;
    float optDepth_M = 0.0;

    float mu = dot(ray, sunDir);
    float mu_2 = mu * mu;
    
    float phase_R = 3.0 / (16.0 * M_PI) * (1.0 + mu_2);
	
    float g_2 = ubo.g * ubo.g;
    float phase_M = 3.0 / (8.0 * M_PI) * 
                          ((1.0 - g_2) * (1.0 + mu_2)) / 
                          ((2.0 + g_2) * pow(1.0 + g_2 - 2.0 * ubo.g * mu, 1.5));

    for (int i = 0; i < viewSamples; ++i)
    {
        vec3 vSample = origin + ray * (tCurrent + segmentLen * 0.5);
        float height = (length(vSample) - PlanetRadius)/(AtmosphereRadius - PlanetRadius);

        float h_R = exp(-height / rayleighHeight) * segmentLen;
		float h_M = exp(-height / mieHeight) * segmentLen;
		optDepth_R += h_R;
		optDepth_M += h_M;

        float segmentLenLight = raySphereIntersection(vec3(0,0,0), vSample, sunDir, AtmosphereRadius).y / float(lightSamples);
        float tCurrentLight = 0.0;

		float optDepthLight_R = 0.0;
		float optDepthLight_M = 0.0;

        for (int j = 0; j < lightSamples; ++j)
        {
            vec3 lSample = vSample + sunDir * (tCurrentLight + segmentLenLight * 0.5);
            float heightLight = (length(lSample) - PlanetRadius)/(AtmosphereRadius - PlanetRadius);          
            optDepthLight_R += exp(-heightLight / rayleighHeight) * segmentLenLight;
			optDepthLight_M += exp(-heightLight / mieHeight) * segmentLenLight;
			tCurrentLight += segmentLenLight;
        }

        vec3 att = exp(-(ubo.scatteringCoefficients.xyz * (optDepth_R + optDepthLight_R) + 
						ubo.scatteringCoefficients.w * 1.1f * (optDepth_M + optDepthLight_M)));
		sum_R += h_R * att;
		sum_M += h_M * att;

        tCurrent += segmentLen;
    }
	
    return I_sun * (sum_R * ubo.scatteringCoefficients.xyz * phase_R + sum_M * ubo.scatteringCoefficients.w * phase_M);
}

void main() 
{
	vec3 rayOrigin = ubo.cameraPosition.xyz;
	vec3 acolor = computeSkyColor(normalize(inPosition - rayOrigin), rayOrigin);
	acolor = mix(acolor, (1.0 - exp(-1.0 * acolor)), 1.0);
	outFragColor = vec4(acolor, 0.8);
	outFragPositions  = vec4(inPosition, 1.0);
}