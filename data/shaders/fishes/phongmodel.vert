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

layout (binding = 1) uniform UboInstance 
{
	//mat4 model; 
	vec4 position;
	vec4 scale;
} uboInstance;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec2 outUV;
layout (location = 2) out vec3 outPos;
layout (location = 3) out vec3 outLightPos;
layout (location = 4) out vec3 outCamPos;

out gl_PerVertex {
	vec4 gl_Position;
};

mat4 translationMatrix(vec3 translation) {
    return mat4(
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        translation.x, translation.y, translation.z, 1.0
    );
}

mat4 rotateTowards(vec3 from, vec3 to) {
    vec3 v = cross(from, to);            // Cross product to find the rotation axis
    float c = dot(from, to);             // Dot product to find the cosine of the angle
    float k = 1.0 / (1.0 + c);           // A factor used to prevent division by zero

    mat3 rotation = mat3(
        vec3(v.x * v.x * k + c,       v.y * v.x * k + v.z,  v.z * v.x * k - v.y),
        vec3(v.x * v.y * k - v.z,     v.y * v.y * k + c,    v.z * v.y * k + v.x),
        vec3(v.x * v.z * k + v.y,     v.y * v.z * k - v.x,  v.z * v.z * k + c)
    );

    return mat4(rotation);               // Convert 3x3 to 4x4 rotation matrix
}
mat3 rotationMatrix(vec3 axis, float angle) {
    // Normalize the axis to ensure correct scaling
    axis = normalize(axis);

    // Calculate the cosine and sine of the angle
    float cosTheta = cos(angle);
    float sinTheta = sin(angle);
    float oneMinusCos = 1.0 - cosTheta;

    // Extract axis components
    float x = axis.x;
    float y = axis.y;
    float z = axis.z;

    // Construct the rotation matrix
    return mat3(
        cosTheta + x * x * oneMinusCos,         x * y * oneMinusCos - z * sinTheta,  x * z * oneMinusCos + y * sinTheta,
        y * x * oneMinusCos + z * sinTheta,     cosTheta + y * y * oneMinusCos,      y * z * oneMinusCos - x * sinTheta,
        z * x * oneMinusCos - y * sinTheta,     z * y * oneMinusCos + x * sinTheta,  cosTheta + z * z * oneMinusCos
    );
}

void main() 
{
	vec3 circlePosition;
	circlePosition.x = sin(uboInstance.position.w) * (20.0+uboInstance.position.x);
	circlePosition.z = cos(uboInstance.position.w) * (20.0+uboInstance.position.x);
	circlePosition.y = uboInstance.position.y;
	vec3 dir = normalize(vec3(circlePosition.z, 0.0f,-circlePosition.x));
	mat4 trans = translationMatrix(circlePosition);
	mat4 rotmat = rotateTowards(vec3(0.0,0.0,1.0), dir);
	float angle = uboInstance.scale.w;
	mat4 rotmat2 = mat4(rotationMatrix(vec3(0.0,1.0,0.0), angle));
	vec3 worldPos = (trans * rotmat * rotmat2 * vec4(inPos * uboInstance.scale.xyz, 1.0)).xyz;
	
	outNormal = vec3(rotmat * rotmat2 * vec4(inNormal, 0.0));
	outUV = inUV;
	outPos = worldPos;
	outLightPos = ubo.light_pos.xyz;
	outCamPos = ubo.camera_pos;
	
	gl_Position = ubo.projection * ubo.view * vec4(worldPos, 1.0);
}