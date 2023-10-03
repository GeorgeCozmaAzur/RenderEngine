/*
* Vulkan buffer class
*
* Encapsulates a Vulkan buffer
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#pragma once

#include <vector>
#include "VulkanTools.h"

namespace engine
{	
	namespace render
	{
		/**
		* @brief Encapsulates access to a Vulkan buffer backed up by device memory
		* @note To be filled by an external source like the VulkanDevice
		*/
		struct VulkanBuffer
		{
			VkDevice _device = VK_NULL_HANDLE;
			VkBuffer m_buffer = VK_NULL_HANDLE;
			VkDeviceMemory m_memory = VK_NULL_HANDLE;
			VkDescriptorBufferInfo m_descriptor;
			VkDeviceSize m_size = 0;
			VkDeviceSize m_alignment = 0;
			void* m_mapped = nullptr;

			/** @brief Usage flags to be filled by external source at buffer creation (to query at some later point) */
			VkBufferUsageFlags m_usageFlags;
			/** @brief Memory propertys flags to be filled by external source at buffer creation (to query at some later point) */
			VkMemoryPropertyFlags m_memoryPropertyFlags;

			VulkanBuffer::~VulkanBuffer()
			{
				destroy();
			}

			/**
			* Map a memory range of this buffer. If successful, mapped points to the specified buffer range.
			*
			* @param size (Optional) Size of the memory range to map. Pass VK_WHOLE_SIZE to map the complete buffer range.
			* @param offset (Optional) Byte offset from beginning
			*
			* @return VkResult of the buffer mapping call
			*/
			VkResult map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0)
			{
				return vkMapMemory(_device, m_memory, offset, size, 0, &m_mapped);
			}

			/**
			* Unmap a mapped memory range
			*
			* @note Does not return a result as vkUnmapMemory can't fail
			*/
			void unmap()
			{
				if (m_mapped)
				{
					vkUnmapMemory(_device, m_memory);
					m_mapped = nullptr;
				}
			}

			/**
			* Attach the allocated memory block to the buffer
			*
			* @param offset (Optional) Byte offset (from the beginning) for the memory region to bind
			*
			* @return VkResult of the bindBufferMemory call
			*/
			VkResult bind(VkDeviceSize offset = 0)
			{
				return vkBindBufferMemory(_device, m_buffer, m_memory, offset);
			}

			/**
			* Setup the default descriptor for this buffer
			*
			* @param size (Optional) Size of the memory range of the descriptor
			* @param offset (Optional) Byte offset from beginning
			*
			*/
			void setupDescriptor(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0)
			{
				m_descriptor.offset = offset;
				m_descriptor.buffer = m_buffer;
				m_descriptor.range = size;
			}

			/**
			* Copies the specified data to the mapped buffer
			*
			* @param data Pointer to the data to copy
			* @param size Size of the data to copy in machine units
			*
			*/
			void copyTo(void* data, VkDeviceSize size)
			{
				assert(m_mapped);
				memcpy(m_mapped, data, size);
			}

			/**
			* Flush a memory range of the buffer to make it visible to the device
			*
			* @note Only required for non-coherent memory
			*
			* @param size (Optional) Size of the memory range to flush. Pass VK_WHOLE_SIZE to flush the complete buffer range.
			* @param offset (Optional) Byte offset from beginning
			*
			* @return VkResult of the flush call
			*/
			VkResult flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0)
			{
				VkMappedMemoryRange mappedRange = {};
				mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
				mappedRange.memory = m_memory;
				mappedRange.offset = offset;
				mappedRange.size = size;
				return vkFlushMappedMemoryRanges(_device, 1, &mappedRange);
			}

			/**
			* Create a buffer on the device
			*
			* @param usageFlags Usage flag bitmask for the buffer (i.e. index, vertex, uniform buffer)
			* @param memoryPropertyFlags Memory properties for this buffer (i.e. device local, host visible, coherent)
			* @param buffer Pointer to a vk::Vulkan buffer object
			* @param size Size of the buffer in byes
			* @param data Pointer to the data that should be copied to the buffer after creation (optional, if not set, no data is copied over)
			*
			* @return VK_SUCCESS if buffer handle and memory have been created and (optionally passed) data has been copied
			*/
			VkResult createBuffer(VkDevice device, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkPhysicalDeviceMemoryProperties* memoryProperties, VkDeviceSize size, void* data = nullptr)
			{
				//TODO investigate if it's good to send memoryProperties as pointer
				_device = device;

				// Create the buffer handle
				VkBufferCreateInfo bufferCreateInfo = initializers::bufferCreateInfo(usageFlags, size);
				VK_CHECK_RESULT(vkCreateBuffer(device, &bufferCreateInfo, nullptr, &m_buffer));

				// Create the memory backing up the buffer handle
				VkMemoryRequirements memReqs;
				VkMemoryAllocateInfo memAlloc = initializers::memoryAllocateInfo();
				vkGetBufferMemoryRequirements(device, m_buffer, &memReqs);
				memAlloc.allocationSize = memReqs.size;
				// Find a memory type index that fits the properties of the buffer
				memAlloc.memoryTypeIndex = tools::getMemoryType(memReqs.memoryTypeBits, memoryPropertyFlags, memoryProperties);
				// If the buffer has VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT set we also need to enable the appropriate flag during allocation
				VkMemoryAllocateFlagsInfoKHR allocFlagsInfo{};
				if (usageFlags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
					allocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR;
					allocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
					memAlloc.pNext = &allocFlagsInfo;
				}
				VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &m_memory));

				m_alignment = memReqs.alignment;
				m_size = size;//memAlloc.allocationSize;
				m_usageFlags = usageFlags;
				m_memoryPropertyFlags = memoryPropertyFlags;

				// If a pointer to the buffer data has been passed, map the buffer and copy over the data
				if (data != nullptr)
				{
					VK_CHECK_RESULT(map());
					memcpy(m_mapped, data, size);
					if ((m_memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
						flush();

					unmap();
				}

				// Initialize a default descriptor that covers the whole buffer size
				setupDescriptor();

				// Attach the memory to the buffer object
				return bind();
			}

			/**
			* Invalidate a memory range of the buffer to make it visible to the host
			*
			* @note Only required for non-coherent memory
			*
			* @param size (Optional) Size of the memory range to invalidate. Pass VK_WHOLE_SIZE to invalidate the complete buffer range.
			* @param offset (Optional) Byte offset from beginning
			*
			* @return VkResult of the invalidate call
			*/
			VkResult invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0)
			{
				VkMappedMemoryRange mappedRange = {};
				mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
				mappedRange.memory = m_memory;
				mappedRange.offset = offset;
				mappedRange.size = size;
				return vkInvalidateMappedMemoryRanges(_device, 1, &mappedRange);
			}

			/**
			* Release all Vulkan resources held by this buffer
			*/
			void destroy()
			{
				//TODO should we unmap mem first
				if (m_buffer)
				{
					vkDestroyBuffer(_device, m_buffer, nullptr);
				}
				if (m_memory)
				{
					vkFreeMemory(_device, m_memory, nullptr);
				}
			}

		};
	}
}