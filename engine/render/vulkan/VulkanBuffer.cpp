#include "VulkanBuffer.h"

namespace engine
{
	namespace render
	{
		VkResult VulkanBuffer::Create(VkDevice device,
			VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkPhysicalDeviceMemoryProperties* memoryProperties,
			VkDeviceSize size, void* data)
		{
			_device = device;

			VkBufferCreateInfo bufferCreateInfo{};
			bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferCreateInfo.usage = usageFlags;
			bufferCreateInfo.size = size;
			VK_CHECK_RESULT(vkCreateBuffer(device, &bufferCreateInfo, nullptr, &m_buffer));

			VkMemoryRequirements memRequirements;
			vkGetBufferMemoryRequirements(_device, m_buffer, &memRequirements);

			VkMemoryAllocateInfo memAllocInfo{};
			memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			memAllocInfo.allocationSize = memRequirements.size;
			memAllocInfo.memoryTypeIndex = tools::getMemoryType(memRequirements.memoryTypeBits, memoryPropertyFlags, memoryProperties);

			// If the buffer has VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT set we also need to enable the appropriate flag during allocation
			VkMemoryAllocateFlagsInfoKHR allocFlagsInfo{};
			if (usageFlags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
				allocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR;
				allocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
				memAllocInfo.pNext = &allocFlagsInfo;
			}
			VK_CHECK_RESULT(vkAllocateMemory(device, &memAllocInfo, nullptr, &m_memory));

			m_alignment = memRequirements.alignment;
			m_size = size;//memAlloc.allocationSize;
			m_usageFlags = usageFlags;
			m_memoryPropertyFlags = memoryPropertyFlags;

			// If a pointer to the buffer data has been passed, map the buffer and copy over the data
			if (data != nullptr)
			{
				VK_CHECK_RESULT(Map());
				memcpy(m_mapped, data, size);
				if ((m_memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
					Flush();

				Unmap();
			}

			m_descriptor.offset = 0;
			m_descriptor.buffer = m_buffer;
			m_descriptor.range = size;

			return vkBindBufferMemory(_device, m_buffer, m_memory, 0);
		}

		VkResult VulkanBuffer::Map(VkDeviceSize size, VkDeviceSize offset)
		{
			return vkMapMemory(_device, m_memory, offset, size, 0, &m_mapped);
		}

		void VulkanBuffer::Unmap()
		{
			if (m_mapped)
			{
				vkUnmapMemory(_device, m_memory);
				m_mapped = nullptr;
			}
		}

		void VulkanBuffer::MemCopy(void* data, VkDeviceSize size, VkDeviceSize offset)
		{
			assert(m_mapped);
			
			memcpy((char*)m_mapped + offset, data, size);
		}

		VkResult VulkanBuffer::Flush(VkDeviceSize size, VkDeviceSize offset)
		{
			VkMappedMemoryRange mappedRange = {};
			mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			mappedRange.memory = m_memory;
			mappedRange.offset = offset;
			mappedRange.size = size;
			return vkFlushMappedMemoryRanges(_device, 1, &mappedRange);
		}

		void VulkanBuffer::Destroy()
		{
			if (m_buffer)
			{
				vkDestroyBuffer(_device, m_buffer, nullptr);
			}
			if (m_memory)
			{
				vkFreeMemory(_device, m_memory, nullptr);
			}
		}
	}
}