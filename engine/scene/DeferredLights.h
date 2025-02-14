#pragma once

#include "SimpleModel.h"
#include "render/VulkanDevice.h"

namespace engine
{
	namespace scene
	{
		struct PointLight
		{
			glm::vec4 PositionAndRadius;
			glm::vec4 Color;
		};

		class DeferredLights : public SimpleModel
		{
			render::VulkanDevice* vulkanDevice;
		public:
			std::vector<glm::vec4> m_pointLights;

			void Init(render::VulkanBuffer *ub, render::VulkanDevice *device, VkQueue queue, VkRenderPass renderPass, VkPipelineCache pipelineCache, int lightsNumber, 
				render::VulkanTexture *positions, render::VulkanTexture* normals, render::VulkanTexture* roughnessMetallic = nullptr, render::VulkanTexture* albedo = nullptr);
			void Update();
			~DeferredLights();
		};
	}
}