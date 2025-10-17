#include "VulkanCommandBuffer.h"

namespace engine
{
	namespace render
	{
		VulkanCommandBuffer::~VulkanCommandBuffer()
		{
			//vkFreeCommandBuffers(_device, _commandPool, 1, &m_vkCommandBuffer);TODO make the individual free
		}

		void VulkanCommandBuffer::Begin()
		{
			VkCommandBufferBeginInfo cmdBufInfo{};
			cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			VK_CHECK_RESULT(vkBeginCommandBuffer(m_vkCommandBuffer, &cmdBufInfo));
		}

		void VulkanCommandBuffer::End()
		{
			VK_CHECK_RESULT(vkEndCommandBuffer(m_vkCommandBuffer));
		}
	}
}
