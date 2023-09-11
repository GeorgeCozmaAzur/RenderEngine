#pragma once
#include <map>
#include "scene/RenderObject.h"
#include "render/VulkanDevice.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>

namespace engine
{
	namespace scene
	{	
		class ClothComputeObject : public scene::RenderObject
		{
		public:

			struct ClothVertex {
				glm::vec4 position;
				glm::vec4 velocity;
				glm::vec4 normal;
				glm::vec4 uv;			
			};

			glm::uvec2 m_gridSize = glm::uvec2(50, 50);
			glm::vec2 m_size = glm::vec2(4.0f);

			std::vector<ClothVertex> vertexBuffer;
			std::vector<uint32_t> m_indices;

			struct StorageBuffers {
				render::VulkanBuffer* inbuffer;
				render::VulkanBuffer* outbuffer;
			} storageBuffers;

			render::VulkanBuffer* m_uniformBuffer;

			std::vector<VkCommandBuffer> commandBuffers;

			struct UBO {
				float deltaT = 0.0f;
				float particleMass = 0.1f;
				float springStiffness = 2000.0f;
				float damping = 0.95f;
				float restDistH;
				float restDistV;
				float restDistD;
				float shearing = 1.0f;
				glm::vec4 externalForce = glm::vec4(0.0f, 9.8f, 0.0f, 0.0f);
				glm::vec4 pinnedCorners = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
				glm::vec2 projectionSizes;
				glm::ivec2 particleCount;
			} ubo;
			glm::ivec4 pinnedCorners = glm::ivec4(1, 1, 1, 1);

			void PrepareStorageBuffers(glm::vec2 size, glm::uvec2 gridSize, render::VulkanDevice* vulkanDevice, VkQueue queue);
			void PrepareUniformBuffer(render::VulkanDevice* vulkanDevice, float projectionWidth, float projectionDepth);
			void UpdateUniformBuffer();
		};
	}
}