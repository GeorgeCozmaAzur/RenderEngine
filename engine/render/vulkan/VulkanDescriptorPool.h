#pragma once
#include <VulkanTools.h>
#include "DescriptorPool.h"

namespace engine
{
	namespace render
	{
		class VulkanDescriptorPool : public DescriptorPool
		{
		public:
			VkDevice _device = VK_NULL_HANDLE;
			VkDescriptorPool m_vkPool = VK_NULL_HANDLE;
			VulkanDescriptorPool(std::vector<DescriptorPoolSize> poolSizes, uint32_t maxSets) : DescriptorPool(poolSizes, maxSets) {};
			void Create(VkDevice device);
			~VulkanDescriptorPool();
		};
	}
}