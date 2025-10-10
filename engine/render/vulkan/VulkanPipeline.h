#pragma once
#include <VulkanTools.h>
#include "Pipeline.h"

namespace engine
{
	namespace render
	{
		//struct VulkanPipelineProperties : PipelineProperties
		//{
			/*VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			uint32_t vertexConstantBlockSize = 0;
			uint32_t* fragmentConstants = nullptr;
			bool blendEnable = false;
			bool depthBias = false;
			bool depthTestEnable = true;
			bool depthWriteEnable = true;
			VkCullModeFlagBits cullMode = VK_CULL_MODE_BACK_BIT;
			VkBool32 primitiveRestart = VK_FALSE;
			uint32_t attachmentCount = 0;*/
			//const VkPipelineColorBlendAttachmentState* pAttachments = nullptr;
			//uint32_t subpass = 0;
		//};

		class VulkanPipeline : public Pipeline
		{
			VkDevice _device = VK_NULL_HANDLE;

			VkPipeline m_vkPipeline = VK_NULL_HANDLE;
			VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;

			VkRenderPass _renderPass = VK_NULL_HANDLE;
			VkPipelineCache _pipelineCache = VK_NULL_HANDLE;

			// List of shader modules created (stored for cleanup)
			std::vector<VkShaderModule> m_shaderModules;//TODO remove this

			VkPipelineShaderStageCreateInfo LoadShader(std::string fileName, VkShaderStageFlagBits stage);

			bool m_depthBias = false;

		public:
			VulkanPipeline() {};
			~VulkanPipeline();

			void SetRenderPass(VkRenderPass value) { _renderPass = value; };
			void SetCache(VkPipelineCache value) { _pipelineCache = value; };

			//void SetBlending(bool value) { m_blendEnable = value; }

			VkPipeline			getPipeline() { return m_vkPipeline; }
			VkPipelineLayout	getPipelineLayout() { return m_pipelineLayout; }

			void Create(VkDevice device, VkDescriptorSetLayout descriptorSetLayout,
				std::vector<VkVertexInputBindingDescription> vertexInputBindings,
				std::vector<VkVertexInputAttributeDescription> vertexInputAttributes,
				std::string vertexFile, std::string fragmentFile,
				VkRenderPass renderPass, VkPipelineCache cache, 
				PipelineProperties properties
			);

			void Draw(VkCommandBuffer commandBuffer, VkPipelineBindPoint bindpoint = VK_PIPELINE_BIND_POINT_GRAPHICS);

			virtual void Draw(class CommandBuffer* commandBuffer, void* constantData = nullptr);

			void CreateCompute(std::string file, VkDevice device, VkDescriptorSetLayout descriptorSetLayout, VkPipelineCache cache, PipelineProperties properties);
		};
	}
}