#pragma once
#include <VulkanTools.h>
#include "render/CommandBuffer.h"

namespace engine
{
	namespace render
	{
		class VulkanCommandBuffer : public CommandBuffer
		{
		public:
			VkCommandPool _commandPool = VK_NULL_HANDLE;
			VkDevice _device = VK_NULL_HANDLE;
			VkCommandBuffer m_vkCommandBuffer = VK_NULL_HANDLE;

			~VulkanCommandBuffer();
			virtual void Begin();
			virtual void End();
		};
	}
}