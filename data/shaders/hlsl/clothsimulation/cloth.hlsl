// ------------------------------------------------------------
// Struct definitions
// ------------------------------------------------------------
struct Particle
{
    float4 pos;
    float4 vel;
    float4 normal;
    float4 uv;
};

// Input particle buffer
StructuredBuffer<Particle> particleIn : register(t0);

// Output particle buffer
RWStructuredBuffer<Particle> particleOut : register(u0);

// Texture lookup (shadow map)
Texture2D sampleShadow : register(t1);
SamplerState sampleShadowSampler : register(s0);

// ------------------------------------------------------------
// Constant buffer
// ------------------------------------------------------------
cbuffer UBO : register(b0)
{
    float deltaT;
    float particleMass;
    float springStiffness;
    float damping;
    float restDistanceH;
    float restDistanceV;
    float restDistanceD;
    float shearing;            // (unused in original GLSL)
    float4 externalForce;
    float4 pinnedCorners;
    float2 projectionSizes;
    uint2  particleCount;
    float2 _padding_;
};

// ------------------------------------------------------------
// Helper functions
// ------------------------------------------------------------

float3 springForce(float3 p0, float3 p1, float restDist)
{
    float3 dist = p0 - p1;
    float len = length(dist);
    return normalize(dist) * springStiffness * (len - restDist);
}

float3 sampleBuffer(uint2 uv)
{
    uint index = uv.y * particleCount.x + uv.x;
    return particleIn[index].pos.xyz;
}

// ------------------------------------------------------------
// Compute Shader
// ------------------------------------------------------------
[numthreads(10, 10, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    uint index = id.y * particleCount.x + id.x;
    uint maxIndex = particleCount.x * particleCount.y;

    if (index >= maxIndex)
        return;

    // Handle pinned corners
    if ((id.y == 0 && id.x == 0 && pinnedCorners.x == 1.0f) ||
        (id.y == 0 && id.x == (particleCount.x - 1) && pinnedCorners.y == 1.0f) ||
        (id.y == (particleCount.y - 1) && id.x == 0 && pinnedCorners.z == 1.0f) ||
        (id.y == (particleCount.y - 1) && id.x == (particleCount.x - 1) && pinnedCorners.w == 1.0f))
    {
        particleOut[index].pos = particleOut[index].pos;
        particleOut[index].vel = float4(0,0,0,0);
        return;
    }

    float3 gravityForce = externalForce.xyz * particleMass;
    float3 force = float3(0,0,0);

    float3 pos = particleIn[index].pos.xyz;
    float3 vel = particleIn[index].vel.xyz;

    // --------------------------------------------------------
    // Spring forces (cloth neighbors)
    // --------------------------------------------------------
    for (int i = -1; i < 2; i++)
    {
        if ((i < 0 && id.x == 0) || (i > 0 && id.x == particleCount.x - 1))
            continue;

        for (int j = -1; j < 2; j++)
        {
            if ((j < 0 && id.y == 0) || (j > 0 && id.y == particleCount.y - 1))
                continue;

            if (i == 0 && j == 0)
                continue;

            float restDist =
                (j == 0) ? restDistanceH :
                (i == 0) ? restDistanceV :
                           restDistanceD;

            float3 neighbor = sampleBuffer(uint2(id.x + i, id.y + j));
            force += springForce(neighbor, pos, restDist);
        }
    }

    // Add damping
    force += (-damping * vel);

    float3 accel = (force + gravityForce) / particleMass;

    // Update integration
    float3 newPos = pos + vel * deltaT;
    float3 newVel = vel + accel * deltaT;

    particleOut[index].pos = float4(newPos, 1.0f);
    particleOut[index].vel = float4(newVel, 0.0f);

    // --------------------------------------------------------
    // Scene collision via shadow texture
    // --------------------------------------------------------
    float2 xz = newPos.xz * projectionSizes.x;
    xz = xz * 0.5f + 0.5f;

    float shadowDist = (1.0f - sampleShadow.SampleLevel(sampleShadowSampler, xz, 0).r)
                        * projectionSizes.y;

    if (-newPos.y < shadowDist + 0.08f)
    {
        particleOut[index].vel = float4(0,0,0,0);
    }

    // --------------------------------------------------------
    // Compute normals
    // --------------------------------------------------------
    float3 normal = float3(0,0,0);

    float3 a, b, c, d;

    if (id.x > 0)
        a = particleIn[index - 1].pos.xyz - pos;

    if (id.y > 0)
        b = particleIn[index - particleCount.x].pos.xyz - pos;

    if (id.x < particleCount.x - 1)
        c = particleIn[index + 1].pos.xyz - pos;

    if (id.y < particleCount.y - 1)
        d = particleIn[index + particleCount.x].pos.xyz - pos;

    if (id.x > 0 && id.y > 0)
        normal += cross(a, b);

    if (id.y > 0 && id.x < particleCount.x - 1)
        normal += cross(b, c);

    if (id.x < particleCount.x - 1 && id.y < particleCount.y - 1)
        normal += cross(c, d);

    if (id.y < particleCount.y - 1 && id.x > 0)
        normal += cross(d, a);

    particleOut[index].normal = float4(normalize(-normal), 0.0f);
}