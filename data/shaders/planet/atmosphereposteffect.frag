#version 450

#define M_PI 3.1415926535897932384626433832795

layout (binding = 0) uniform UboMatrices
{
	mat4 cameraInvProjection;
	mat4 cameraInvView;
} uboMatrices;

layout (binding = 1) uniform sampler2D samplerColor;
layout (binding = 2) uniform sampler2D samplerPositions;
layout (binding = 3) uniform sampler2D samplerDepth;
layout (binding = 4) uniform sampler2D samplerNoise;
layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outFragColor;

layout (binding = 5) uniform UboView 
{
	
	vec4	sun;//poistion and intensity of the sun
	vec4	cameraPosition;
	vec4	viewDirection;
	vec4	dimensions;//radius of the planet, radius of the atmosphere, Rayleigh scale height, Mie scale height
	vec4	scatteringCoefficients;//Rayleigh and Mie scattering coefficiants
	float	distanceFactor;//a factor that will depend how distances affects the scattering
} ubo;

int numOpticalDepthPoints = 16;

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

float densityAtPoint(vec3 densitySamplePoint, float densityFalloff) {
	float planetRadius = ubo.dimensions.x;
	float atmosphereRadius = ubo.dimensions.y;
	//float rayleighHeight = ubo.dimensions.z;
	vec3 planetCentre = vec3(0.0,0.0,0.0);
	
	//float densityFalloff = 4.0 * rayleighHeight;
	
	float heightAboveSurface = length(densitySamplePoint - planetCentre) - planetRadius;
	float height01 = heightAboveSurface / (atmosphereRadius - planetRadius);
	float localDensity = exp(-height01 * densityFalloff) * (1 - height01);
	return localDensity;
}

float opticalDepth(vec3 rayOrigin, vec3 rayDir, float rayLength, float densityFalloff) {
	vec3 densitySamplePoint = rayOrigin;
	float stepSize = rayLength / (numOpticalDepthPoints - 1);
	float stepSizeOd = stepSize/(ubo.dimensions.y * ubo.distanceFactor);
	float opticalDepth = 0;

	for (int i = 0; i < numOpticalDepthPoints; i ++) {
		float localDensity = densityAtPoint(densitySamplePoint, densityFalloff);
		opticalDepth += localDensity * stepSizeOd;
		densitySamplePoint += rayDir * stepSize;
	}
	return opticalDepth;
}

vec3 calculateLight(vec3 rayOrigin, vec3 rayDir, float rayLength, vec3 originalCol) {
	
	float ditherStrength = 1.0;
	float ditherScale = 3.0;
	vec4 blueNoise = texture(samplerNoise, inUV * ditherScale);
	blueNoise = (blueNoise - 0.5) * ditherStrength;
	
	float planetRadius = ubo.dimensions.x;
	float atmosphereRadius = ubo.dimensions.y;
	vec3 planetCenter = vec3(0.0,0.0,0.0);
	vec3 dirToSun = normalize(ubo.sun.xyz - rayOrigin);
	
	float mu = dot(rayDir, dirToSun);
    float mu_2 = mu * mu;
    float rayleighPhase = 3.0 / (16.0 * M_PI) * (1.0 + mu_2);
	
	vec3 inScatterPoint = rayOrigin;
	float stepSize = rayLength / (numOpticalDepthPoints - 1.0);
	float stepSizeOd = stepSize/(ubo.dimensions.y * ubo.distanceFactor);
	vec3 inScatteredLight = vec3(0.0);
	float viewRayOpticalDepth = 0.0;
	float rayleighHeight = ubo.dimensions.z;

	for (int i = 0; i < numOpticalDepthPoints; i ++) {
		float sunRayLength = raySphereIntersection(planetCenter, inScatterPoint, dirToSun, atmosphereRadius).y;
		float sunRayOpticalDepth = opticalDepth(inScatterPoint + dirToSun, dirToSun, sunRayLength, rayleighHeight);
		float localDensity = densityAtPoint(inScatterPoint, rayleighHeight) * stepSizeOd;
		viewRayOpticalDepth += localDensity;
		vec3 transmittance = exp(-(viewRayOpticalDepth + sunRayOpticalDepth) * ubo.scatteringCoefficients.xyz);
		
		inScatteredLight += localDensity * transmittance * ubo.scatteringCoefficients.xyz * rayleighPhase * ubo.sun.w;
		inScatteredLight += blueNoise.xyz * 0.001;
		inScatterPoint += rayDir * stepSize;
	}
	
	float oct = exp(-viewRayOpticalDepth);
	
	vec3 finalCol = originalCol * oct + inScatteredLight;

	return finalCol;
}

void main() 
{
	vec4 scenecolor = texture(samplerColor, inUV);
	vec4 scenepositions = texture(samplerPositions, inUV);
	vec4 scenedepthndc = texture(samplerDepth, inUV);
	
	//View direction is a direction to every fragment. Then it will be transformed in world space. 
	vec3 ViewRayDirection = vec3(0.0,0.0,0.1);
	ViewRayDirection.x += (-1.0 + 2.0 * inUV.x);
	ViewRayDirection.y += (-1.0 + 2.0 * inUV.y);
	vec4 RealViewRayDirection = (uboMatrices.cameraInvProjection * vec4(ViewRayDirection, 1.0));
	ViewRayDirection.x = RealViewRayDirection.x/RealViewRayDirection.w;
	ViewRayDirection.y = RealViewRayDirection.y/RealViewRayDirection.w;
	ViewRayDirection.z = RealViewRayDirection.z/RealViewRayDirection.w;
	ViewRayDirection = (uboMatrices.cameraInvView * vec4(ViewRayDirection, 1.0)).xyz;
	ViewRayDirection = normalize(ViewRayDirection - ubo.cameraPosition.xyz);

	const float epsilon = 0.0001;
	float AtmosphereRadius = ubo.dimensions.y;

	float sceneDepth = length(scenepositions.xyz - ubo.cameraPosition.xyz);
	vec2 t = raySphereIntersection(vec3(0,0,0), ubo.cameraPosition.xyz, ViewRayDirection, AtmosphereRadius);
	
	float dToAtmosphere = t.x;
	//The distance through atmosphere can be cut short by the planet surface
	float dThroughAtmosphere = dToAtmosphere > 0 ? min(t.y, sceneDepth - t.x) : min(t.y, sceneDepth);
	vec3 rayOrigin = ubo.cameraPosition.xyz;
	vec3 light = scenecolor.xyz;
	if(dThroughAtmosphere>0)
	{
		//If we are outside the atmosphere, the starting point it will be the first point intersected. 
		//If we are inside the atmosphere, the the starting point will be the camera position
		vec3 pointInAtmosphere = dToAtmosphere > 0 ? (rayOrigin + ViewRayDirection * (dToAtmosphere)) : rayOrigin;  
		light = calculateLight(pointInAtmosphere, ViewRayDirection, dThroughAtmosphere - epsilon * 2, scenecolor.xyz);
	}

	light = mix(light, (1.0 - exp(-1.0 * light)), 1.0);
	outFragColor = vec4(light, 1.0);
}