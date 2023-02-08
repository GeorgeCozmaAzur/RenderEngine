#pragma once

#include "SimpleModel.h"
#include "render/VulkanDevice.hpp"

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

			void Init(render::VulkanBuffer *ub, render::VulkanDevice *device, VkQueue queue, VkRenderPass renderPass, VkPipelineCache pipelineCache, int lightsNumber, render::VulkanTexture *positions, render::VulkanTexture* normals);
			void Update();
			~DeferredLights();
		};
	}
}