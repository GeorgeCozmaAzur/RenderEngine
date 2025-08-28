#pragma once
#include <map>
#include "scene/RenderObject.h"
#include "render/vulkan/VulkanDevice.h"

namespace engine
{
	namespace scene
	{	
		class Rings : public RenderObject
		{
			struct {
				glm::mat4 modelView;
			} uboVS;
			render::VulkanBuffer* uniformBufferVS = nullptr;

		public:
			void Init(float innerRadius, float outerRadius, int resolution, render::VulkanDevice* vulkanDevice, VkDescriptorPool descriptorPool, render::VertexLayout* vertex_layout, render::VulkanBuffer* globalUniformBufferVS, std::vector<VkDescriptorImageInfo*> texturesDescriptors
				, std::string vertexShaderFilename
				, std::string fragmentShaderFilename
				, VkRenderPass renderPass
				, VkPipelineCache pipelineCache
				, render::PipelineProperties pipelineProperties
				, VkQueue queue);
			uint32_t* BuildIndices(int offsetX, int offsetY, int width, int height, int& size);
			void UpdateUniforms(glm::mat4 &model);
		};
	}
}