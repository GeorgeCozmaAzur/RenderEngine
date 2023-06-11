#pragma once
#include <VulkanTools.h>
#include <vector>

namespace engine
{
	namespace render
	{
		struct PipelineProperties
		{
			VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			uint32_t vertexConstantBlockSize = 0;
			uint32_t* fragmentConstants = nullptr;
			bool blendEnable = false;
			bool depthBias = false;
			bool depthTestEnable = true;
			bool depthWriteEnable = true;
			VkCullModeFlagBits cullMode = VK_CULL_MODE_BACK_BIT;
			uint32_t attachmentCount = 0;
			const VkPipelineColorBlendAttachmentState* pAttachments = nullptr;
			uint32_t subpass = 0;
		};

		class VulkanPipeline
		{
			VkDevice _device = VK_NULL_HANDLE;

			VkPipeline m_vkPipeline = VK_NULL_HANDLE;
			VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;

			VkRenderPass _renderPass = VK_NULL_HANDLE;
			VkPipelineCache _pipelineCache = VK_NULL_HANDLE;

			// List of shader modules created (stored for cleanup)
			std::vector<VkShaderModule> m_shaderModules;//TODO remove this

			VkPipelineShaderStageCreateInfo LoadShader(std::string fileName, VkShaderStageFlagBits stage);

			bool m_blendEnable = false;

		public:
			VulkanPipeline() {};
			~VulkanPipeline();

			void SetRenderPass(VkRenderPass value) { _renderPass = value; };
			void SetCache(VkPipelineCache value) { _pipelineCache = value; };

			void SetBlending(bool value) { m_blendEnable = value; }

			VkPipeline			getPipeline() { return m_vkPipeline; }
			VkPipelineLayout	getPipelineLayout() { return m_pipelineLayout; }

			void Create(VkDevice device, VkDescriptorSetLayout descriptorSetLayout,
				std::vector<VkVertexInputBindingDescription> vertexInputBindings,
				std::vector<VkVertexInputAttributeDescription> vertexInputAttributes,
				std::string vertexFile, std::string fragmentFile,
				VkRenderPass renderPass, VkPipelineCache cache, VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
				uint32_t vertexConstantBlockSize = 0,
				uint32_t* fragmentConstants = nullptr,
				uint32_t attachmentCount = 0,
				const VkPipelineColorBlendAttachmentState* pAttachments = nullptr,
				bool depthBias = false,
				bool depthTestEnable = true,
				bool depthWriteEnable = true,
				uint32_t subpass = 0
			);

			void Create(VkDevice device, VkDescriptorSetLayout descriptorSetLayout,
				std::vector<VkVertexInputBindingDescription> vertexInputBindings,
				std::vector<VkVertexInputAttributeDescription> vertexInputAttributes,
				std::string vertexFile, std::string fragmentFile,
				VkRenderPass renderPass, VkPipelineCache cache, 
				PipelineProperties properties
			);

			void Draw(VkCommandBuffer commandBuffer, VkPipelineBindPoint bindpoint = VK_PIPELINE_BIND_POINT_GRAPHICS);

			void CreateCompute(std::string file, VkDevice device, VkDescriptorSetLayout descriptorSetLayout, VkPipelineCache cache);
		};
	}
}