#include <stdlib.h>
#include <fstream>
#include <vector>

#include "VulkanTexture.h"
#include "VulkanTools.h"


namespace engine
{
	namespace render
	{
		void VulkanTexture::Create(VkDevice device, VkPhysicalDeviceMemoryProperties* memoryProperties, VkExtent3D extent, VkFormat format,
			VkImageUsageFlags imageUsageFlags,
			VkImageLayout imageLayout,
			VkImageAspectFlags aspect,
			uint32_t mipLevelsCount, uint32_t layersCount,
			VkImageCreateFlags flags)
		{
			_device = device;
			m_format = format;
			m_width = extent.width;
			m_height = extent.height;
			m_depth = extent.depth;
			m_mipLevelsCount = mipLevelsCount;
			m_layerCount = layersCount;
			m_descriptor.imageLayout = imageLayout;
			m_aspect = aspect;

			VkMemoryAllocateInfo memAllocInfo{};
			memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			VkMemoryRequirements memReqs;

			// Create optimal tiled target image
			VkImageCreateInfo imageCreateInfo{};
			imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;

			extent.depth > 1 ? imageCreateInfo.imageType = VK_IMAGE_TYPE_3D : imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;	
			imageCreateInfo.format = m_format;
			imageCreateInfo.mipLevels = m_mipLevelsCount;
			imageCreateInfo.arrayLayers = m_layerCount;
			imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageCreateInfo.extent = extent;
			imageCreateInfo.usage = imageUsageFlags;
			imageCreateInfo.flags = flags;

			VK_CHECK_RESULT(vkCreateImage(_device, &imageCreateInfo, nullptr, &m_vkImage));

			vkGetImageMemoryRequirements(_device, m_vkImage, &memReqs);//TODO see which is faster? call this or store it's contents?

			memAllocInfo.allocationSize = memReqs.size;

			memAllocInfo.memoryTypeIndex = tools::getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memoryProperties);
			VK_CHECK_RESULT(vkAllocateMemory(_device, &memAllocInfo, nullptr, &m_deviceMemory));
			VK_CHECK_RESULT(vkBindImageMemory(_device, m_vkImage, m_deviceMemory, 0));

		}

		void VulkanTexture::ChangeLayout(
			VkCommandBuffer cmdbuffer,
			VkImageLayout oldImageLayout,
			VkImageLayout newImageLayout,
			VkPipelineStageFlags srcStageMask,
			VkPipelineStageFlags dstStageMask)
		{
			VkImageSubresourceRange subresourceRange = { m_aspect, 0, m_mipLevelsCount, 0, m_layerCount };

			// Create an image barrier object
			VkImageMemoryBarrier imageMemoryBarrier{};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageMemoryBarrier.oldLayout = oldImageLayout;
			imageMemoryBarrier.newLayout = newImageLayout;
			imageMemoryBarrier.image = m_vkImage;
			imageMemoryBarrier.subresourceRange = subresourceRange;

			// Source layouts (old)
			// Source access mask controls actions that have to be finished on the old layout
			// before it will be transitioned to the new layout
			switch (oldImageLayout)
			{
			case VK_IMAGE_LAYOUT_UNDEFINED:
				// Image layout is undefined (or does not matter)
				// Only valid as initial layout
				// No flags required, listed only for completeness
				imageMemoryBarrier.srcAccessMask = 0;
				break;

			case VK_IMAGE_LAYOUT_PREINITIALIZED:
				// Image is preinitialized
				// Only valid as initial layout for linear images, preserves memory contents
				// Make sure host writes have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
				// Image is a color attachment
				// Make sure any writes to the color buffer have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
				// Image is a depth/stencil attachment
				// Make sure any writes to the depth/stencil buffer have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
				// Image is a transfer source 
				// Make sure any reads from the image have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				break;

			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
				// Image is a transfer destination
				// Make sure any writes to the image have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
				// Image is read by a shader
				// Make sure any shader reads from the image have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
				break;
			default:
				// Other source layouts aren't handled (yet)
				break;
			}

			// Target layouts (new)
			// Destination access mask controls the dependency for the new image layout
			switch (newImageLayout)
			{
			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
				// Image will be used as a transfer destination
				// Make sure any writes to the image have been finished
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
				// Image will be used as a transfer source
				// Make sure any reads from the image have been finished
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				break;

			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
				// Image will be used as a color attachment
				// Make sure any writes to the color buffer have been finished
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
				// Image layout will be used as a depth/stencil attachment
				// Make sure any writes to depth/stencil buffer have been finished
				imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
				// Image will be read in a shader (sampler, input attachment)
				// Make sure any writes to the image have been finished
				if (imageMemoryBarrier.srcAccessMask == 0)
				{
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
				}
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				break;
			default:
				// Other source layouts aren't handled (yet)
				break;
			}

			// Put barrier inside setup command buffer
			vkCmdPipelineBarrier(
				cmdbuffer,
				srcStageMask,
				dstStageMask,
				0,
				0, nullptr,
				0, nullptr,
				1, &imageMemoryBarrier);
		}
		void VulkanTexture::PipelineBarrier(
			VkCommandBuffer cmdbuffer,
			VkAccessFlags srcAccessMask,
			VkAccessFlags dstAccessMask,
			VkPipelineStageFlags srcStageMask,
			VkPipelineStageFlags dstStageMask
			)
		{
			VkImageMemoryBarrier imageMemoryBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, nullptr, 
				srcAccessMask, dstAccessMask,
				m_descriptor.imageLayout, m_descriptor.imageLayout,
				VK_QUEUE_FAMILY_IGNORED,VK_QUEUE_FAMILY_IGNORED,
				m_vkImage,
				{ m_aspect, 0, m_mipLevelsCount, 0, m_layerCount }
			};

			vkCmdPipelineBarrier(
				cmdbuffer,
				srcStageMask,
				dstStageMask,
				VK_FLAGS_NONE,
				0, nullptr,
				0, nullptr,
				1, &imageMemoryBarrier);
		}

		void VulkanTexture::Update(TextureExtent** extents, VkBuffer stagingBuffer, VkCommandBuffer copyCmd, VkQueue copyQueue)
		{
			// Image barrier for optimal image (target)
			ChangeLayout(copyCmd, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

			std::vector<VkBufferImageCopy> bufferCopyRegions;
			size_t offset = 0;

			for (uint32_t face = 0; face < m_layerCount; face++)
			{
				for (uint32_t level = 0; level < m_mipLevelsCount; level++)
				{
					VkBufferImageCopy bufferCopyRegion = {};
					bufferCopyRegion.imageSubresource.aspectMask = m_aspect;
					bufferCopyRegion.imageSubresource.mipLevel = level;
					bufferCopyRegion.imageSubresource.baseArrayLayer = face;
					bufferCopyRegion.imageSubresource.layerCount = 1;
					bufferCopyRegion.imageExtent.width = static_cast<uint32_t>(extents[face][level].width);
					bufferCopyRegion.imageExtent.height = static_cast<uint32_t>(extents[face][level].height);
					bufferCopyRegion.imageExtent.depth = 1;
					bufferCopyRegion.bufferOffset = offset;

					bufferCopyRegions.push_back(bufferCopyRegion);

					// Increase offset into staging buffer for next level / face
					offset += extents[face][level].size;
				}
			}

			// Copy the faces from the staging buffer to the optimal tiled image
			vkCmdCopyBufferToImage(
				copyCmd,
				stagingBuffer,
				m_vkImage,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				static_cast<uint32_t>(bufferCopyRegions.size()),
				bufferCopyRegions.data());

			// Change texture image layout to shader read after all faces have been copied
			ChangeLayout(copyCmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_descriptor.imageLayout, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
		}

		void VulkanTexture::UpdateGeneratingMipmaps(TextureExtent** extents, VkBuffer stagingBuffer, VkCommandBuffer copyCmd, VkQueue copyQueue)
		{
			ChangeLayout(copyCmd, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

			size_t offset = 0;
			for (uint32_t face = 0; face < m_layerCount; face++)
			{
				VkBufferImageCopy bufferCopyRegion = {};
				bufferCopyRegion.imageSubresource.aspectMask = m_aspect;
				bufferCopyRegion.imageSubresource.mipLevel = 0;
				bufferCopyRegion.imageSubresource.baseArrayLayer = face;
				bufferCopyRegion.imageSubresource.layerCount = 1;
				bufferCopyRegion.imageExtent.width = static_cast<uint32_t>(extents[face][0].width);
				bufferCopyRegion.imageExtent.height = static_cast<uint32_t>(extents[face][0].height);
				bufferCopyRegion.imageExtent.depth = 1;
				bufferCopyRegion.bufferOffset = offset;

				offset += extents[face][0].size;

				// Copy the faces from the staging buffer to the optimal tiled image
				vkCmdCopyBufferToImage(
					copyCmd,
					stagingBuffer,
					m_vkImage,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					1,
					&bufferCopyRegion);

				VkImageMemoryBarrier barrier{};
				barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				barrier.image = m_vkImage;
				barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.subresourceRange.aspectMask = m_aspect;
				barrier.subresourceRange.baseArrayLayer = face;
				barrier.subresourceRange.layerCount = 1;
				barrier.subresourceRange.levelCount = 1;

				int32_t mipWidth = extents[face][0].width;
				int32_t mipHeight = extents[face][0].height;

				for (uint32_t i = 1; i < m_mipLevelsCount; i++)
				{
					barrier.subresourceRange.baseMipLevel = i - 1;
					barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
					barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
					barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
					barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

					vkCmdPipelineBarrier(copyCmd,
						VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
						0, nullptr,
						0, nullptr,
						1, &barrier);

					VkImageBlit blit{};
					blit.srcOffsets[0] = { 0, 0, 0 };
					blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
					blit.srcSubresource.aspectMask = m_aspect;
					blit.srcSubresource.mipLevel = i - 1;
					blit.srcSubresource.baseArrayLayer = face;
					blit.srcSubresource.layerCount = 1;
					blit.dstOffsets[0] = { 0, 0, 0 };
					blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
					blit.dstSubresource.aspectMask = m_aspect;
					blit.dstSubresource.mipLevel = i;
					blit.dstSubresource.baseArrayLayer = face;
					blit.dstSubresource.layerCount = 1;

					vkCmdBlitImage(copyCmd,
						m_vkImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
						m_vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						1, &blit,
						VK_FILTER_LINEAR);

					barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
					barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
					barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

					vkCmdPipelineBarrier(copyCmd,
						VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
						0, nullptr,
						0, nullptr,
						1, &barrier);

					if (mipWidth > 1) mipWidth /= 2;
					if (mipHeight > 1) mipHeight /= 2;
				}
				barrier.subresourceRange.baseMipLevel = m_mipLevelsCount - 1;
				barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

				vkCmdPipelineBarrier(copyCmd,
					VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
					0, nullptr,
					0, nullptr,
					1, &barrier);
			}
		}

		void VulkanTexture::CreateDescriptor(VkSamplerAddressMode adressMode, VkImageViewType viewType, float maxAnisoropy)
		{
			//TODO if not used in shaders no point in creating a sampler
			VkSamplerCreateInfo samplerCreateInfo{};
			samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
			samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
			samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			samplerCreateInfo.addressModeU = adressMode;
			samplerCreateInfo.addressModeV = samplerCreateInfo.addressModeU;
			samplerCreateInfo.addressModeW = samplerCreateInfo.addressModeU;
			samplerCreateInfo.mipLodBias = 0.0f;
			samplerCreateInfo.maxAnisotropy = maxAnisoropy;
			if (maxAnisoropy > 1.0f)
				samplerCreateInfo.anisotropyEnable = VK_TRUE;
			samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
			samplerCreateInfo.minLod = 0.0f;
			samplerCreateInfo.maxLod = (float)m_mipLevelsCount;
			samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
			VK_CHECK_RESULT(vkCreateSampler(_device, &samplerCreateInfo, nullptr, &m_descriptor.sampler));

			// Create image view
			VkImageViewCreateInfo viewCreateInfo{};
			viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewCreateInfo.viewType = viewType;
			viewCreateInfo.format = m_format;
			viewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
			viewCreateInfo.subresourceRange = { m_aspect, 0, m_mipLevelsCount, 0, m_layerCount };
			viewCreateInfo.image = m_vkImage;
			VK_CHECK_RESULT(vkCreateImageView(_device, &viewCreateInfo, nullptr, &m_descriptor.imageView));
		}

		void VulkanTexture::Destroy()
		{
			if(m_descriptor.imageView)
				vkDestroyImageView(_device, m_descriptor.imageView, nullptr);

			if(m_vkImage)
				vkDestroyImage(_device, m_vkImage, nullptr);

			if (m_descriptor.sampler)
				vkDestroySampler(_device, m_descriptor.sampler, nullptr);

			if(m_deviceMemory)
				vkFreeMemory(_device, m_deviceMemory, nullptr);
		}
	}
}
