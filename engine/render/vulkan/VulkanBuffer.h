#pragma once

#include <vector>
#include "Buffer.h"
#include "VulkanTools.h"

namespace engine
{
	namespace render
	{
		class VulkanBuffer : public Buffer
		{
			VkDevice _device = VK_NULL_HANDLE;
			VkBuffer m_buffer = VK_NULL_HANDLE;
			VkDeviceMemory m_memory = VK_NULL_HANDLE;		
			VkDeviceSize m_alignment = 0;

			VkBufferUsageFlags m_usageFlags;
			VkMemoryPropertyFlags m_memoryPropertyFlags;

		public:

			VkDescriptorBufferInfo m_descriptor;

			const VkBuffer GetVkBuffer() const { return m_buffer; }

			VkDeviceSize GetSize() { return VkDeviceSize(m_size); }

			VkResult Create(VkDevice device,
				VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkPhysicalDeviceMemoryProperties* memoryProperties, 
				VkDeviceSize size, void* data = nullptr);

			VkResult Map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

			void Unmap();

			VkResult Flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

			void Destroy();

			~VulkanBuffer() { Destroy(); }
		};
	}
}