#include "VulkanCommandPool.h"
#include "VulkanCommandBuffer.h"

namespace engine
{
	namespace render
	{
		VulkanCommandPool::~VulkanCommandPool()
		{
			vkDestroyCommandPool(_device, m_vkCommandPool, nullptr);
		}

		void VulkanCommandPool::Create(VkDevice device, uint32_t queueIndex)
		{
			_device = device;
			m_queueIndex = queueIndex;
			VkCommandPoolCreateInfo cmdPoolInfo = {};
			cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			cmdPoolInfo.queueFamilyIndex = m_queueIndex;
			cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;//TODO make this more dynamic
			VK_CHECK_RESULT(vkCreateCommandPool(_device, &cmdPoolInfo, nullptr, &m_vkCommandPool));
		}

		CommandBuffer* VulkanCommandPool::GetCommandBuffer()
		{
			VulkanCommandBuffer* vkcommandBuffer = new VulkanCommandBuffer(this);
			vkcommandBuffer->_device = _device;
			vkcommandBuffer->_commandPool = m_vkCommandPool;
			VkCommandBufferAllocateInfo cmdBufAllocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO , nullptr, m_vkCommandPool, m_primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY, 1 };
			VK_CHECK_RESULT(vkAllocateCommandBuffers(_device, &cmdBufAllocateInfo, &vkcommandBuffer->m_vkCommandBuffer));

			m_commandBuffers.push_back(vkcommandBuffer);
			return vkcommandBuffer;
		}

		void VulkanCommandPool::DestroyCommandBuffer(class CommandBuffer* cmd)
		{
			std::vector<CommandBuffer*>::iterator it;
			it = find(m_commandBuffers.begin(), m_commandBuffers.end(), cmd);
			if (it != m_commandBuffers.end())
			{
					VulkanCommandBuffer* vkcmd = dynamic_cast<VulkanCommandBuffer*>(cmd);
					vkFreeCommandBuffers(_device, m_vkCommandPool, 1, &vkcmd->m_vkCommandBuffer);
					m_commandBuffers.erase(it);
			}
		}

		void VulkanCommandPool::Reset()
		{

		}
	}
}
