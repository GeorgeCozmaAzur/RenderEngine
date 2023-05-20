#pragma once
#include <map>
#include "Terrain.h"
#include "render/VulkanDevice.hpp"

namespace engine
{
	namespace scene
	{	
		class UVSphere : public Terrain
		{
			struct {
				glm::mat4 modelView;
			} uboVS;
			render::VulkanBuffer* uniformBufferVS = nullptr;

		public:
			void Init(const std::string& filename, float radius, render::VulkanDevice* vulkanDevice, render::VertexLayout* vertex_layout, render::VulkanBuffer* globalUniformBufferVS, std::vector<VkDescriptorImageInfo*> texturesDescriptors
				, std::string vertexShaderFilename
				, std::string fragmentShaderFilename
				, VkRenderPass renderPass
				, VkPipelineCache pipelineCache
				, VkQueue queue);
			void UpdateUniforms(glm::mat4 &model);
		};
	}
}