#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable

#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_buffer_reference2 : require

layout(location = 0) rayPayloadInEXT vec3 hitValue;
layout(location = 2) rayPayloadEXT bool shadowed;
hitAttributeEXT vec2 attribs;

struct ObjDesc
{
  int      txtOffset;             // Texture index offset in the array of textures
  uint64_t vertexAddress;         // Address of the Vertex buffer
  uint64_t indexAddress;          // Address of the index buffer
  uint64_t materialAddress;       // Address of the material buffer
  uint64_t materialIndexAddress;  // Address of the triangle material index buffer
};

layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;
layout(binding = 2, set = 0) uniform UBO 
{
	mat4 viewInverse;
	mat4 projInverse;
	vec4 lightPos;
	int vertexSize;
} ubo;

struct Vertex
{
  vec3 pos;
  vec3 normal;
  vec2 uv;
  float pad;
  vec3 bt;
  vec4 padd;
 };
 struct ObjectMaterial
{
  vec3  diffuse;
  int   textureId;
};

layout(buffer_reference, scalar) buffer Verticesnew {Vertex v[]; }; // Positions of an object
layout(buffer_reference, scalar) buffer Indicesnew {ivec3 i[]; }; // Triangle indices
layout(buffer_reference, scalar) buffer Materials {ObjectMaterial m[]; }; // Array of all materials on an object
layout(buffer_reference, scalar) buffer MatIndices {int i[]; }; // Material ID for each triangle
layout(binding = 3, set = 0) buffer Vertices { vec4 v[]; } vertices;
layout(binding = 4, set = 0) buffer Indices { uint i[]; } indices;
layout(binding = 5, set = 0) uniform sampler2D textureSamplers[];
layout(binding = 6, set = 0, scalar) buffer ObjDesc_ { ObjDesc i[]; } objDesc;

void main()
{
	ObjDesc    objResource = objDesc.i[gl_InstanceID];
	MatIndices matIndices  = MatIndices(objResource.materialIndexAddress);
	Materials  materials   = Materials(objResource.materialAddress);
  
	Indicesnew    indicesnew     = Indicesnew(objResource.indexAddress);
	Verticesnew   verticesnew    = Verticesnew(objResource.vertexAddress);
	ivec3 ind = indicesnew.i[gl_PrimitiveID];
	
	Vertex v0 = verticesnew.v[ind.x];
	Vertex v1 = verticesnew.v[ind.y];
	Vertex v2 = verticesnew.v[ind.z];
	
	const vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
	vec3 normal = normalize(v0.normal * barycentricCoords.x + v1.normal * barycentricCoords.y + v2.normal * barycentricCoords.z);
	vec2 texCoord = v0.uv * barycentricCoords.x + v1.uv * barycentricCoords.y + v2.uv * barycentricCoords.z;
	
	int	matIdx = matIndices.i[gl_PrimitiveID];
	ObjectMaterial mat    = materials.m[matIdx];
	uint txtId    = mat.textureId + objDesc.i[gl_InstanceCustomIndexEXT].txtOffset;
  
	vec3 lightVector = normalize(ubo.lightPos.xyz);
	float dot_product = max(dot(-lightVector, normal), 0.2);
	vec3 finalColor = dot_product * mat.diffuse;
	if(mat.textureId >= 0)
	{
		uint txtId    = mat.textureId + objResource.txtOffset;
		finalColor *= texture(textureSamplers[txtId], texCoord).xyz;
	}
	hitValue = vec3(finalColor);
	
	float tmin = 0.001;
	float tmax = 10000.0;
	vec3 origin = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
	shadowed = true; 
	traceRayEXT(topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT, 0xFF, 1, 0, 1, origin, tmin, -lightVector, tmax, 2);
	if (shadowed) {
		hitValue *= 0.3;
	}
}
