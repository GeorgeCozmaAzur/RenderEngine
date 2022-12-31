#pragma once
#include <VulkanTools.h>
#include <vector>

namespace engine
{
	namespace render
	{
		//TODO 
		//make all uniforms not own
		//do we need to add multiple buffers
		//add desc per swapchain image
		class VulkanDescriptorSet
		{
			VkDevice _device = VK_NULL_HANDLE;
			VkDescriptorPool _descriptorPool = VK_NULL_HANDLE;
			VkDescriptorSetLayout _layout = VK_NULL_HANDLE;
			VkDescriptorSet m_vkDescriptorSet = VK_NULL_HANDLE;
			std::vector<VkDescriptorBufferInfo*> _BuffersDescriptors;
			std::vector<VkDescriptorImageInfo*> _TexturesDescriptors;
			uint32_t m_dynamicAlignment = 0;

		public:
			~VulkanDescriptorSet();

			void Draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t indexInDynamicUniformBuffer, VkPipelineBindPoint pipelineBindpoint = VK_PIPELINE_BIND_POINT_GRAPHICS);
			void SetDynamicAlignment(uint32_t value) { m_dynamicAlignment = value; }

			void SetupDescriptors(VkDevice, VkDescriptorPool, VkDescriptorSetLayout, std::vector<VkDescriptorSetLayoutBinding>);

			void AddTextureDescriptor(VkDescriptorImageInfo* desc) { _TexturesDescriptors.push_back(desc); };
			size_t GetTexturesNo() { return _TexturesDescriptors.size(); }

			void AddBufferDescriptor(VkDescriptorBufferInfo* desc) { _BuffersDescriptors.push_back(desc); };
			void AddBufferDescriptors(std::vector<VkDescriptorBufferInfo*> inputArray) { _BuffersDescriptors.insert(_BuffersDescriptors.end(), inputArray.begin(), inputArray.end()); };

		};
	}
}