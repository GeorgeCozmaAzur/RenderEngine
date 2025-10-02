#include "VulkanDescriptorSetLayout.h"

namespace engine
{
	namespace render
	{
		VkDescriptorSetLayoutBinding descriptorSetLayoutBinding(
			VkDescriptorType type,
			VkShaderStageFlags stageFlags,
			uint32_t binding,
			uint32_t descriptorCount = 1)
		{
			VkDescriptorSetLayoutBinding setLayoutBinding{};
			setLayoutBinding.descriptorType = type;
			setLayoutBinding.stageFlags = stageFlags;
			setLayoutBinding.binding = binding;
			setLayoutBinding.descriptorCount = descriptorCount;
			return setLayoutBinding;
		}

		void VulkanDescriptorSetLayout::AddDescriptorSetLayoutBinding(VkDescriptorType type, VkShaderStageFlags stageFlags)
		{
			m_setLayoutBindings.push_back(descriptorSetLayoutBinding(
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

		void VulkanDescriptorSetLayout::Create(VkDevice device)
		{
			_device = device;

			std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> layoutBindigs(m_bindings.size());
			for (int i = 0; i < m_bindings.size(); i++)
			{
				layoutBindigs[i].first = ToVkDescType(m_bindings[i].descriptorType);
				layoutBindigs[i].second = ToVkShaderStage(m_bindings[i].stage);
			}
			Create(device, layoutBindigs);
		}

		void VulkanDescriptorSetLayout::Destroy()
		{
			vkDestroyDescriptorSetLayout(_device, m_descriptorSetLayout, nullptr);
		}
	}
}