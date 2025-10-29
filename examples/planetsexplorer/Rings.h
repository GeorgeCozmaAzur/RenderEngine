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
			void Init(float innerRadius, float outerRadius, int resolution, render::VulkanDevice* vulkanDevice, render::DescriptorPool* descriptorPool, render::VertexLayout* vertex_layout, render::Buffer* globalUniformBufferVS, std::vector<render::Texture*> texturesDescriptors
				, std::string vertexShaderFilename
				, std::string fragmentShaderFilename
				, render::RenderPass* renderPass
				, VkPipelineCache pipelineCache
				, render::PipelineProperties pipelineProperties
				, VkQueue queue);
			uint32_t* BuildIndices(int offsetX, int offsetY, int width, int height, int& size);
			void UpdateUniforms(glm::mat4 &model);
		};
	}
}