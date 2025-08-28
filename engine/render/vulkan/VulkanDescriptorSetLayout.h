#pragma once
#include <VulkanTools.h>
#include <vector>

namespace engine
{
	namespace render
	{
		struct VulkanDescriptorSetLayout
		{
			VkDevice _device;

			VkDescriptorSetLayout m_descriptorSetLayout;
			std::vector<VkDescriptorSetLayoutBinding> m_setLayoutBindings;

			void AddDescriptorSetLayoutBinding(VkDescriptorType type, VkShaderStageFlags stageFlags);

			void Create(VkDevice device, std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> layoutBindigs);

			void Destroy();

			~VulkanDescriptorSetLayout() { Destroy(); }
		};
	}
}