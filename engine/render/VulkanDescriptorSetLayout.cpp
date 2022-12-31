#include "VulkanDescriptorSetLayout.h"

namespace engine
{
	namespace render
	{
		void VulkanDescriptorSetLayout::AddDescriptorSetLayoutBinding(VkDescriptorType type, VkShaderStageFlags stageFlags)
		{
			m_setLayoutBindings.push_back(engine::initializers::descriptorSetLayoutBinding(
				type,
				stageFlags,
				(uint32_t)m_setLayoutBindings.size()));
		}

		void VulkanDescriptorSetLayout::Create(VkDevice device, std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> layoutBindigs)
		{
			_device = device;

			for (auto binding : layoutBindigs)
			{
				AddDescriptorSetLayoutBinding(binding.first, binding.second);
			}

			VkDescriptorSetLayoutCreateInfo descriptorLayoutCI{};
			descriptorLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			descriptorLayoutCI.bindingCount = static_cast<uint32_t>(m_setLayoutBindings.size());
			descriptorLayoutCI.pBindings = m_setLayoutBindings.data();

			VK_CHECK_RESULT(vkCreateDescriptorSetLayout(_device, &descriptorLayoutCI, nullptr, &m_descriptorSetLayout));
		}

		void VulkanDescriptorSetLayout::Destroy()
		{
			vkDestroyDescriptorSetLayout(_device, m_descriptorSetLayout, nullptr);
		}
	}
}