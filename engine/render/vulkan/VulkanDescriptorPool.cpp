#include "VulkanDescriptorPool.h"
#include "VulkanDescriptorSetlayout.h"

namespace engine
{
	namespace render
	{
		void VulkanDescriptorPool::Create(VkDevice device)
		{
			_device = device;

			std::vector<VkDescriptorPoolSize> vkPoolSizes(m_poolSizes.size());
			for (int i = 0; i < m_poolSizes.size(); i++)
			{
				vkPoolSizes[i].type = ToVkDescType(m_poolSizes[i].type);
				if (vkPoolSizes[i].type == VK_DESCRIPTOR_TYPE_MAX_ENUM)
					return;
				vkPoolSizes[i].descriptorCount = m_poolSizes[i].size;
			}

			VkDescriptorPoolCreateInfo descriptorPoolInfo{};
			descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(vkPoolSizes.size());
			descriptorPoolInfo.pPoolSizes = vkPoolSizes.data();
			descriptorPoolInfo.maxSets = m_maxSets;

			VK_CHECK_RESULT(vkCreateDescriptorPool(_device, &descriptorPoolInfo, nullptr, &m_vkPool));
		}

		VulkanDescriptorPool::~VulkanDescriptorPool()
		{
			if(m_vkPool != VK_NULL_HANDLE)
			vkDestroyDescriptorPool(_device, m_vkPool, nullptr);
		}
	}
}