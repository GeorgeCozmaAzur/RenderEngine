#include "SkinnedMesh.h"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace engine
{
	namespace scene
	{
		// Load bone information from ASSIMP mesh
		void SkinnedMesh::loadBones(const aiMesh* pMesh, uint32_t vertexOffset, std::vector<VertexBoneData>& Bones)
		{
			for (uint32_t i = 0; i < pMesh->mNumBones; i++)
			{
				uint32_t index = 0;

				assert(pMesh->mNumBones <= MAX_BONES);

				std::string name(pMesh->mBones[i]->mName.data);

				if (boneMapping.find(name) == boneMapping.end())
				{
					// Bone not present, add new one
					index = numBones;
					numBones++;
					BoneInfo bone;
					boneInfo.push_back(bone);
					boneInfo[index].offset = pMesh->mBones[i]->mOffsetMatrix;
					boneMapping[name] = index;
				}
				else
				{
					index = boneMapping[name];
				}

				for (uint32_t j = 0; j < pMesh->mBones[i]->mNumWeights; j++)
				{
					uint32_t vertexID = vertexOffset + pMesh->mBones[i]->mWeights[j].mVertexId;
					Bones[vertexID].add(index, pMesh->mBones[i]->mWeights[j].mWeight);
				}
			}
			boneTransforms.resize(numBones);
		}

		// Recursive bone transformation for given animation time
		void SkinnedMesh::update(float time)
		{
			float TicksPerSecond = (float)(scene->mAnimations[0]->mTicksPerSecond != 0 ? scene->mAnimations[0]->mTicksPerSecond : 25.0f);
			float TimeInTicks = time * TicksPerSecond;
			float AnimationTime = fmod(TimeInTicks, (float)scene->mAnimations[0]->mDuration);

			aiMatrix4x4 identity = aiMatrix4x4();
			readNodeHierarchy(AnimationTime, scene->mRootNode, identity);

			for (uint32_t i = 0; i < boneTransforms.size(); i++)
			{
				boneTransforms[i] = boneInfo[i].finalTransformation;
			}
		}

		const aiNodeAnim* SkinnedMesh::findNodeAnim(const aiAnimation* animation, const std::string nodeName)
		{
			for (uint32_t i = 0; i < animation->mNumChannels; i++)
			{
				const aiNodeAnim* nodeAnim = animation->mChannels[i];
				if (std::string(nodeAnim->mNodeName.data) == nodeName)
				{
					return nodeAnim;
				}
			}
			return nullptr;
		}

		// Returns a 4x4 matrix with interpolated translation between current and next frame
		aiMatrix4x4 SkinnedMesh::interpolateTranslation(float time, const aiNodeAnim* pNodeAnim)
		{
			aiVector3D translation;

			if (pNodeAnim->mNumPositionKeys == 1)
			{
				translation = pNodeAnim->mPositionKeys[0].mValue;
			}
			else
			{
				uint32_t frameIndex = 0;
				for (uint32_t i = 0; i < pNodeAnim->mNumPositionKeys - 1; i++)
				{
					if (time < (float)pNodeAnim->mPositionKeys[i + 1].mTime)
					{
						frameIndex = i;
						break;
					}
				}

				aiVectorKey currentFrame = pNodeAnim->mPositionKeys[frameIndex];
				aiVectorKey nextFrame = pNodeAnim->mPositionKeys[(frameIndex + 1) % pNodeAnim->mNumPositionKeys];

				float delta = (time - (float)currentFrame.mTime) / (float)(nextFrame.mTime - currentFrame.mTime);

				const aiVector3D& start = currentFrame.mValue;
				const aiVector3D& end = nextFrame.mValue;

				translation = (start + delta * (end - start));
			}

			aiMatrix4x4 mat;
			aiMatrix4x4::Translation(translation, mat);
			return mat;
		}

		// Returns a 4x4 matrix with interpolated rotation between current and next frame
		aiMatrix4x4 SkinnedMesh::interpolateRotation(float time, const aiNodeAnim* pNodeAnim)
		{
			aiQuaternion rotation;

			if (pNodeAnim->mNumRotationKeys == 1)
			{
				rotation = pNodeAnim->mRotationKeys[0].mValue;
			}
			else
			{
				uint32_t frameIndex = 0;
				for (uint32_t i = 0; i < pNodeAnim->mNumRotationKeys - 1; i++)
				{
					if (time < (float)pNodeAnim->mRotationKeys[i + 1].mTime)
					{
						frameIndex = i;
						break;
					}
				}

				aiQuatKey currentFrame = pNodeAnim->mRotationKeys[frameIndex];
				aiQuatKey nextFrame = pNodeAnim->mRotationKeys[(frameIndex + 1) % pNodeAnim->mNumRotationKeys];

				float delta = (time - (float)currentFrame.mTime) / (float)(nextFrame.mTime - currentFrame.mTime);

				const aiQuaternion& start = currentFrame.mValue;
				const aiQuaternion& end = nextFrame.mValue;

				aiQuaternion::Interpolate(rotation, start, end, delta);
				rotation.Normalize();
			}

			aiMatrix4x4 mat(rotation.GetMatrix());
			return mat;
		}


		// Returns a 4x4 matrix with interpolated scaling between current and next frame
		aiMatrix4x4 SkinnedMesh::interpolateScale(float time, const aiNodeAnim* pNodeAnim)
		{
			aiVector3D scale;

			if (pNodeAnim->mNumScalingKeys == 1)
			{
				scale = pNodeAnim->mScalingKeys[0].mValue;
			}
			else
			{
				uint32_t frameIndex = 0;
				for (uint32_t i = 0; i < pNodeAnim->mNumScalingKeys - 1; i++)
				{
					if (time < (float)pNodeAnim->mScalingKeys[i + 1].mTime)
					{
						frameIndex = i;
						break;
					}
				}

				aiVectorKey currentFrame = pNodeAnim->mScalingKeys[frameIndex];
				aiVectorKey nextFrame = pNodeAnim->mScalingKeys[(frameIndex + 1) % pNodeAnim->mNumScalingKeys];

				float delta = (time - (float)currentFrame.mTime) / (float)(nextFrame.mTime - currentFrame.mTime);

				const aiVector3D& start = currentFrame.mValue;
				const aiVector3D& end = nextFrame.mValue;

				scale = (start + delta * (end - start));
			}

			aiMatrix4x4 mat;
			aiMatrix4x4::Scaling(scale, mat);
			return mat;
		}

		// Get node hierarchy for current animation time
		void SkinnedMesh::readNodeHierarchy(float AnimationTime, const aiNode* pNode, const aiMatrix4x4& ParentTransform)
		{
			std::string NodeName(pNode->mName.data);

			aiMatrix4x4 NodeTransformation(pNode->mTransformation);

			const aiNodeAnim* pNodeAnim = findNodeAnim(pAnimation, NodeName);

			if (pNodeAnim)
			{
				// Get interpolated matrices between current and next frame
				aiMatrix4x4 matScale = interpolateScale(AnimationTime, pNodeAnim);
				aiMatrix4x4 matRotation = interpolateRotation(AnimationTime, pNodeAnim);
				aiMatrix4x4 matTranslation = interpolateTranslation(AnimationTime, pNodeAnim);

				NodeTransformation = matTranslation * matRotation * matScale;
			}

			aiMatrix4x4 GlobalTransformation = ParentTransform * NodeTransformation;

			if (boneMapping.find(NodeName) != boneMapping.end())
			{
				uint32_t BoneIndex = boneMapping[NodeName];
				boneInfo[BoneIndex].finalTransformation = globalInverseTransform * GlobalTransformation * boneInfo[BoneIndex].offset;
			}

			for (uint32_t i = 0; i < pNode->mNumChildren; i++)
			{
				readNodeHierarchy(AnimationTime, pNode->mChildren[i], GlobalTransformation);
			}
		}

		// Load a mesh based on data read via assimp 
		bool SkinnedMesh::LoadGeometry(const std::string& filename, render::VertexLayout* vertex_layout, float scale, int instance_no, glm::vec3 atPos)
		{
			_vertexLayout = vertex_layout;
			//skinnedMesh = new SkinnedMesh();

			//std::string filename = getAssetPath() + "models/goblin.dae";

#if defined(__ANDROID__)
	// Meshes are stored inside the apk on Android (compressed)
	// So they need to be loaded via the asset manager

			AAsset* asset = AAssetManager_open(androidApp->activity->assetManager, filename.c_str(), AASSET_MODE_STREAMING);
			assert(asset);
			size_t size = AAsset_getLength(asset);

			assert(size > 0);

			void* meshData = malloc(size);
			AAsset_read(asset, meshData, size);
			AAsset_close(asset);

			scene = Importer.ReadFileFromMemory(meshData, size, 0);

			free(meshData);
#else
			scene = Importer.ReadFile(filename.c_str(), 0);
#endif
			setAnimation(0);

			// Setup bones
			// One vertex bone info structure per vertex
			uint32_t vertexCount(0);
			for (uint32_t m = 0; m < scene->mNumMeshes; m++) {
				vertexCount += scene->mMeshes[m]->mNumVertices;
			};
			bones.resize(vertexCount);
			// Store global inverse transform matrix of root node 
			globalInverseTransform = scene->mRootNode->mTransformation;
			globalInverseTransform.Inverse();
			// Load bones (weights and IDs)
			uint32_t vertexBase(0);
			for (uint32_t m = 0; m < scene->mNumMeshes; m++) {
				aiMesh* paiMesh = scene->mMeshes[m];
				if (paiMesh->mNumBones > 0) {
					loadBones(paiMesh, vertexBase, bones);
				}
				vertexBase += scene->mMeshes[m]->mNumVertices;
			}

			// Generate vertex buffer
			std::vector<SkinningVertex> vertexBuffer;
			// Iterate through all meshes in the file and extract the vertex information used in this demo
			vertexBase = 0;
			for (uint32_t m = 0; m < scene->mNumMeshes; m++) {
				for (uint32_t v = 0; v < scene->mMeshes[m]->mNumVertices; v++) {
					SkinningVertex vertex;

					vertex.pos = glm::make_vec3(&scene->mMeshes[m]->mVertices[v].x);
					vertex.normal = glm::make_vec3(&scene->mMeshes[m]->mNormals[v].x);
					vertex.uv = (scene->mMeshes[m]->HasTextureCoords(0)) ? glm::make_vec2(&scene->mMeshes[m]->mTextureCoords[0][v].x) : glm::vec2(1.0f);
					vertex.color = (scene->mMeshes[m]->HasVertexColors(0)) ? glm::make_vec3(&scene->mMeshes[m]->mColors[0][v].r) : glm::vec3(1.0f);

					// Fetch bone weights and IDs
					for (uint32_t j = 0; j < MAX_BONES_PER_VERTEX; j++) {
						vertex.boneWeights[j] = bones[vertexBase + v].weights[j];
						vertex.boneIDs[j] = bones[vertexBase + v].IDs[j];
					}

					vertexBuffer.push_back(vertex);
				}
				vertexBase += scene->mMeshes[m]->mNumVertices;
			}
			VkDeviceSize vertexBufferSize = vertexBuffer.size() * sizeof(SkinningVertex);

			// Generate index buffer from loaded mesh file
			std::vector<uint32_t> indexBuffer;
			for (uint32_t m = 0; m < scene->mNumMeshes; m++) {
				uint32_t indexBase = static_cast<uint32_t>(indexBuffer.size());
				for (uint32_t f = 0; f < scene->mMeshes[m]->mNumFaces; f++) {
					for (uint32_t i = 0; i < 3; i++)
					{
						indexBuffer.push_back(scene->mMeshes[m]->mFaces[f].mIndices[i] + indexBase);
					}
				}
			}
			VkDeviceSize indexBufferSize = indexBuffer.size() * sizeof(uint32_t);
			//vertexBuffer.indexCount = static_cast<uint32_t>(indexBuffer.size());

			/*struct {
				VkBuffer buffer;
				VkDeviceMemory memory;
			} vertexStaging, indexStaging;*/

			Geometry* geometry = new Geometry();
			geometry->m_vertexCount = vertexBuffer.size();
			geometry->m_indexCount = static_cast<uint32_t>(indexBuffer.size());
			geometry->m_indices = new uint32_t[geometry->m_indexCount];
			memcpy(geometry->m_indices, indexBuffer.data(), geometry->m_indexCount * sizeof(uint32_t));

			geometry->m_verticesSize = vertexBuffer.size() * sizeof(SkinningVertex) / sizeof(float);//TODO dangerous - what if int differs in size from float
			geometry->m_vertices = new float[geometry->m_verticesSize];
			memcpy(geometry->m_vertices, vertexBuffer.data(), geometry->m_verticesSize * sizeof(float));

			/*engine::Buffer* vbuffer = new engine::Buffer;
			engine::Buffer* ibuffer = new engine::Buffer;

			// Create staging buffers
			// Vertex data
			VK_CHECK_RESULT(vulkanDevice->createBuffer(
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				vertexBufferSize,
				&vertexStaging.buffer,
				&vertexStaging.memory,
				vertexBuffer.data()));
			// Index data
			VK_CHECK_RESULT(vulkanDevice->createBuffer(
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				indexBufferSize,
				&indexStaging.buffer,
				&indexStaging.memory,
				indexBuffer.data()));

			// Create device local buffers
			// Vertex buffer
			VK_CHECK_RESULT(vulkanDevice->createBuffer(
				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				vbuffer,
				vertexBufferSize));
			// Index buffer
			VK_CHECK_RESULT(vulkanDevice->createBuffer(
				VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				ibuffer,
				indexBufferSize));

			// Copy from staging buffers
			//VkCommandBuffer copyCmd = VulkanExampleBase::createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
			// Copy from staging buffers
			VkCommandBuffer copyCmd = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

			VkBufferCopy copyRegion = {};

			copyRegion.size = vertexBufferSize;
			vkCmdCopyBuffer(
				copyCmd,
				vertexStaging.buffer,
				vbuffer->buffer,
				1,
				&copyRegion);

			copyRegion.size = indexBufferSize;
			vkCmdCopyBuffer(
				copyCmd,
				indexStaging.buffer,
				ibuffer->buffer,
				1,
				&copyRegion);

			//VulkanExampleBase::flushCommandBuffer(copyCmd, copyQueue, true);
			vulkanDevice->flushCommandBuffer(copyCmd, copyQueue);

			vkDestroyBuffer(m_device, vertexStaging.buffer, nullptr);
			vkFreeMemory(m_device, vertexStaging.memory, nullptr);
			vkDestroyBuffer(m_device, indexStaging.buffer, nullptr);
			vkFreeMemory(m_device, indexStaging.memory, nullptr);

			geometry->_vertices = vbuffer->buffer;
			geometry->_indices = ibuffer->buffer;
			m_vertex_buffers.push_back(vbuffer);
			m_index_buffers.push_back(ibuffer);
			*/
			m_geometries.push_back(geometry);

			return true;
		}
	}
}