#pragma once
#include <VulkanTools.h>
#include "render/CommandPool.h"

namespace engine
{
	namespace render
	{
		class VulkanCommandPool : public CommandPool
		{
		public:
			VkCommandPool m_vkCommandPool = VK_NULL_HANDLE;
			VkDevice _device = VK_NULL_HANDLE;

			~VulkanCommandPool();
			void Create(VkDevice device, uint32_t queueIndex);
			virtual CommandBuffer* GetCommandBuffer();
			virtual void DestroyCommandBuffer(class CommandBuffer* cmd);
			virtual void Reset();
		};
	}
}