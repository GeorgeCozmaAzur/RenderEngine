#pragma once
#include "RenderObject.h"
#include <assimp/Importer.hpp> 
#include <assimp/scene.h>     
#include <assimp/postprocess.h>
#include <assimp/cimport.h>
#include <glm/glm.hpp>
#include <array>


namespace engine
{
	namespace scene
	{
		// Maximum number of bones per mesh
		// Must not be higher than same const in skinning shader
#define MAX_BONES 64
// Maximum number of bones per vertex
#define MAX_BONES_PER_VERTEX 4

// Vertex layout used in this example
		struct SkinningVertex {
			glm::vec3 pos;
			glm::vec3 normal;
			glm::vec2 uv;
			glm::vec3 color;
			// Max. four bones per vertex
			float boneWeights[4];
			uint32_t boneIDs[4];
		};

		// Skinned mesh class

		// Per-vertex bone IDs and weights
		struct VertexBoneData
		{
			std::array<uint32_t, MAX_BONES_PER_VERTEX> IDs;
			std::array<float, MAX_BONES_PER_VERTEX> weights;

			// Ad bone weighting to vertex info
			void add(uint32_t boneID, float weight)
			{
				for (uint32_t i = 0; i < MAX_BONES_PER_VERTEX; i++)
				{
					if (weights[i] == 0.0f)
					{
						IDs[i] = boneID;
						weights[i] = weight;
						return;
					}
				}
			}
		};

		// Stores information on a single bone
		struct BoneInfo
		{
			aiMatrix4x4 offset;
			aiMatrix4x4 finalTransformation;

			BoneInfo()
			{
				offset = aiMatrix4x4();
				finalTransformation = aiMatrix4x4();
			};
		};

		class SkinnedMesh : public RenderObject
		{
		public:
			// Bone related stuff
			// Maps bone name with index
			std::map<std::string, uint32_t> boneMapping;
			// Bone details
			std::vector<BoneInfo> boneInfo;
			// Number of bones present
			uint32_t numBones = 0;
			// Root inverese transform matrix
			aiMatrix4x4 globalInverseTransform;
			// Per-vertex bone info
			std::vector<VertexBoneData> bones;
			// Bone transformations
			std::vector<aiMatrix4x4> boneTransforms;

			// Modifier for the animation 
			float animationSpeed = 0.75f;
			// Currently active animation
			aiAnimation* pAnimation;

			// Vulkan buffers
			//engine::Model vertexBuffer;

			// Store reference to the ASSIMP scene for accessing properties of it during animation
			Assimp::Importer Importer;
			const aiScene* scene;

			// Set active animation by index
			void setAnimation(uint32_t animationIndex)
			{
				assert(animationIndex < scene->mNumAnimations);
				pAnimation = scene->mAnimations[animationIndex];
			}

			// Load bone information from ASSIMP mesh
			void loadBones(const aiMesh* pMesh, uint32_t vertexOffset, std::vector<VertexBoneData>& Bones);

			// Recursive bone transformation for given animation time
			void update(float time);

			~SkinnedMesh()
			{
				//vertexBuffer.vertices.destroy();
				//vertexBuffer.indices.destroy();
			}

			std::vector<render::MeshData*> LoadGeometry(const std::string& filename, render::VulkanVertexLayout* vertex_layout, float scale, int instance_no = 1, glm::vec3 atPos = glm::vec3(0.0f));

		private:
			// Find animation for a given node
			const aiNodeAnim* findNodeAnim(const aiAnimation* animation, const std::string nodeName);

			// Returns a 4x4 matrix with interpolated translation between current and next frame
			aiMatrix4x4 interpolateTranslation(float time, const aiNodeAnim* pNodeAnim);

			// Returns a 4x4 matrix with interpolated rotation between current and next frame
			aiMatrix4x4 interpolateRotation(float time, const aiNodeAnim* pNodeAnim);

			// Returns a 4x4 matrix with interpolated scaling between current and next frame
			aiMatrix4x4 interpolateScale(float time, const aiNodeAnim* pNodeAnim);

			// Get node hierarchy for current animation time
			void readNodeHierarchy(float AnimationTime, const aiNode* pNode, const aiMatrix4x4& ParentTransform);

		};
	}
}