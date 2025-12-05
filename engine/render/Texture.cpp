#pragma once
#include <stdlib.h>
#include <fstream>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <gli/gli.hpp>

#include "Texture.h"


namespace engine
{
	namespace render
	{
		/**
		* Load a 2D texture including all mip levels
		*
		* @param filename File to load (supports .ktx and .dds)
		* @param format Vulkan format of the image data stored in the file
		* @param device Vulkan device to create the texture on
		* @param copyQueue Queue used for the texture staging copy commands (must support transfer)
		* @param (Optional) imageUsageFlags Usage flags for the texture's image (defaults to VK_IMAGE_USAGE_SAMPLED_BIT)
		* @param (Optional) imageLayout Usage layout for the texture (defaults VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		* @param (Optional) forceLinear Force linear tiling (not advised, defaults to false)
		*
		*/
		void Texture2DData::LoadFromFile(
			std::string filename,
			GfxFormat format
		)
		{
			m_format = format;


			if (filename.find(".ktx") != std::string::npos)
			{
				/*if (!engine::tools::fileexists(filename)) {
					engine::tools::exitfatal("could not load texture from " + filename + "\n\nthe file may be part of the additional asset pack.\n\nrun \"download_assets.py\" in the repository root to download the latest version.", -1);
				}*/
				gli::texture2d tex2d(gli::load(filename.c_str()));
		
				assert(!tex2d.empty());

				m_width = static_cast<uint32_t>(tex2d[0].extent().x);
				m_height = static_cast<uint32_t>(tex2d[0].extent().y);

				m_imageSize = tex2d.size();
				m_ram_data = new char[m_imageSize];
				memcpy(m_ram_data, tex2d.data(), m_imageSize);

				m_layers_no = 1;
				m_mips_no = static_cast<uint32_t>(tex2d.levels());

				m_extents = new TextureExtent * [1];
				m_extents[0] = new TextureExtent[m_mips_no];

				for (uint32_t i = 0; i < m_mips_no; i++)
				{
					m_extents[0][i].width = tex2d[i].extent().x;
					m_extents[0][i].height = tex2d[i].extent().y;
					m_extents[0][i].size = tex2d[i].size();

				}
			}
			else
			{
				int texWidth, texHeight, texChannels;
				stbi_uc* pixels = stbi_load(filename.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

				assert(pixels);

				m_imageSize = texWidth * texHeight * 4;

				m_width = static_cast<uint32_t>(texWidth);
				m_height = static_cast<uint32_t>(texHeight);
				m_mips_no = 1;
				m_layers_no = 1;
				m_ram_data = new char[m_imageSize];
				memcpy(m_ram_data, pixels, m_imageSize);

				m_extents = new TextureExtent * [1];
				m_extents[0] = new TextureExtent[1];
				{
					m_extents[0][0].width = m_width;
					m_extents[0][0].height = m_height;
					m_extents[0][0].size = m_imageSize;

				}
				stbi_image_free(pixels);
			}
		}

		void Texture2DData::LoadFromFiles(
			std::vector<std::string> filenames,
			GfxFormat format
		)
		{
			m_format = format;

			m_layers_no = static_cast<uint32_t>(filenames.size());
			m_extents = new TextureExtent * [m_layers_no];

			m_imageSize = 0;

			std::vector<stbi_uc*> pixelsarrays;
			//int total_image_size = 0;
			int i = 0;
			for (auto filename : filenames)
			{
				int texWidth, texHeight, texChannels;
				stbi_uc* pixels = stbi_load(filename.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

				assert(pixels);

				pixelsarrays.push_back(pixels);

				m_imageSize += texWidth * texHeight * 4;
				//total_image_size += m_imageSize;

				m_width = static_cast<uint32_t>(texWidth);
				m_height = static_cast<uint32_t>(texHeight);
				m_mips_no = 1;

				m_extents[i] = new TextureExtent[1];
				m_extents[i][0].width = m_width;
				m_extents[i][0].height = m_height;
				i++;
			}

			m_ram_data = new char[m_imageSize];

			int offset = 0;
			for (i=0;i< pixelsarrays.size();i++)
			{
				stbi_uc* pixels = pixelsarrays[i];

				int image_face_size = m_extents[i][0].width * m_extents[i][0].height * 4;
				
				memcpy(m_ram_data + offset, pixels, image_face_size);

				offset += image_face_size;

				m_extents[i][0].size = image_face_size;
				
				stbi_image_free(pixels);
			}
		}

		void Texture2DData::CreateFromBuffer(unsigned char* buffer, size_t bufferSize, uint32_t width, uint32_t height, GfxFormat format)
		{
			owndata = false;
			m_width = width;
			m_height = height;
			m_imageSize = bufferSize;
			m_mips_no = 1;
			m_layers_no = 1;
			m_ram_data = (char*)buffer;
			m_format = format;

			m_extents = new TextureExtent * [1];
			m_extents[0] = new TextureExtent[1];
			{
				m_extents[0][0].width = m_width;
				m_extents[0][0].height = m_height;
				m_extents[0][0].size = m_imageSize;
			}
		}

		void TextureCubeMapData::LoadFromFile(
			std::string filename,
			GfxFormat format
		)
		{
			m_format = format;
			isCubeMap = true;
			/*if (!engine::tools::fileExists(filename)) {
				engine::tools::exitFatal("Could not load texture from " + filename + "\n\nThe file may be part of the additional asset pack.\n\nRun \"download_assets.py\" in the repository root to download the latest version.", -1);
			}*/
			gli::texture_cube texCube(gli::load(filename));
	
			assert(!texCube.empty());

			m_width = static_cast<uint32_t>(texCube.extent().x);
			m_height = static_cast<uint32_t>(texCube.extent().y);
			m_mips_no = static_cast<uint32_t>(texCube.levels());
			m_layers_no = static_cast<uint32_t>(texCube.faces());
			gli::texture::format_type f = texCube.format();
			

			m_imageSize = texCube.size();
			m_ram_data = new char[m_imageSize];
			memcpy(m_ram_data, texCube.data(), m_imageSize);

			m_extents = new TextureExtent * [m_layers_no];
			for (uint32_t i = 0; i < m_layers_no; i++)
			{
				m_extents[i] = new TextureExtent[m_mips_no];
			}

			for (uint32_t face = 0; face < m_layers_no; face++)
			{
				for (uint32_t level = 0; level < m_mips_no; level++)
				{
					m_extents[face][level].width = texCube[face][level].extent().x;
					m_extents[face][level].height = texCube[face][level].extent().y;
					m_extents[face][level].size = texCube[face][level].size(); 
					m_extents[face][level].data = new char[texCube[face][level].size()];//texCube[face][level].data();
					memcpy(m_extents[face][level].data, texCube[face][level].data(), texCube[face][level].size());
				}
			}
		}

		void TextureData::Destroy()
		{
			if (m_extents != nullptr)
			{
				for (uint32_t i = 0; i < m_layers_no; i++)
				{
					for (uint32_t j = 0; j < m_mips_no; j++)
					{
						if(m_extents[i][j].data)
						delete[]m_extents[i][j].data;
					}
					delete[]m_extents[i];
				}
				delete[]m_extents;
			}

			if(owndata)
				delete[] m_ram_data;

			// Clean up staging resources
			/*if (m_stagingMemory)
				vkFreeMemory(_device, m_stagingMemory, nullptr);
			if (m_stagingBuffer)
				vkDestroyBuffer(_device, m_stagingBuffer, nullptr);*/
		}

		//void VulkanTexture::Create(VkDevice device, VkPhysicalDeviceMemoryProperties* memoryProperties, VkExtent3D extent, VkFormat format,
		//	VkImageUsageFlags imageUsageFlags,
		//	VkImageLayout imageLayout,
		//	VkImageAspectFlags aspect,
		//	uint32_t mipLevelsCount, uint32_t layersCount,
		//	VkImageCreateFlags flags)
		//{
		//	_device = device;
		//	m_format = format;
		//	m_width = extent.width;
		//	m_height = extent.height;
		//	m_depth = extent.depth;
		//	m_mipLevelsCount = mipLevelsCount;
		//	m_layerCount = layersCount;
		//	m_descriptor.imageLayout = imageLayout;
		//	m_aspect = aspect;

		//	VkMemoryAllocateInfo memAllocInfo{};
		//	memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		//	VkMemoryRequirements memReqs;

		//	// Create optimal tiled target image
		//	VkImageCreateInfo imageCreateInfo{};
		//	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;

		//	extent.depth > 1 ? imageCreateInfo.imageType = VK_IMAGE_TYPE_3D : imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;	
		//	imageCreateInfo.format = m_format;
		//	imageCreateInfo.mipLevels = m_mipLevelsCount;
		//	imageCreateInfo.arrayLayers = m_layerCount;
		//	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		//	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		//	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		//	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		//	imageCreateInfo.extent = extent;
		//	imageCreateInfo.usage = imageUsageFlags;
		//	imageCreateInfo.flags = flags;

		//	VK_CHECK_RESULT(vkCreateImage(_device, &imageCreateInfo, nullptr, &m_vkImage));

		//	vkGetImageMemoryRequirements(_device, m_vkImage, &memReqs);//TODO see which is faster? call this or store it's contents?

		//	memAllocInfo.allocationSize = memReqs.size;

		//	memAllocInfo.memoryTypeIndex = tools::getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memoryProperties);
		//	VK_CHECK_RESULT(vkAllocateMemory(_device, &memAllocInfo, nullptr, &m_deviceMemory));
		//	VK_CHECK_RESULT(vkBindImageMemory(_device, m_vkImage, m_deviceMemory, 0));

		//}

		//void VulkanTexture::ChangeLayout(
		//	VkCommandBuffer cmdbuffer,
		//	VkImageLayout oldImageLayout,
		//	VkImageLayout newImageLayout,
		//	VkPipelineStageFlags srcStageMask,
		//	VkPipelineStageFlags dstStageMask)
		//{
		//	VkImageSubresourceRange subresourceRange = { m_aspect, 0, m_mipLevelsCount, 0, m_layerCount };

		//	// Create an image barrier object
		//	VkImageMemoryBarrier imageMemoryBarrier{};
		//	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		//	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		//	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		//	imageMemoryBarrier.oldLayout = oldImageLayout;
		//	imageMemoryBarrier.newLayout = newImageLayout;
		//	imageMemoryBarrier.image = m_vkImage;
		//	imageMemoryBarrier.subresourceRange = subresourceRange;

		//	// Source layouts (old)
		//	// Source access mask controls actions that have to be finished on the old layout
		//	// before it will be transitioned to the new layout
		//	switch (oldImageLayout)
		//	{
		//	case VK_IMAGE_LAYOUT_UNDEFINED:
		//		// Image layout is undefined (or does not matter)
		//		// Only valid as initial layout
		//		// No flags required, listed only for completeness
		//		imageMemoryBarrier.srcAccessMask = 0;
		//		break;

		//	case VK_IMAGE_LAYOUT_PREINITIALIZED:
		//		// Image is preinitialized
		//		// Only valid as initial layout for linear images, preserves memory contents
		//		// Make sure host writes have been finished
		//		imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		//		break;

		//	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		//		// Image is a color attachment
		//		// Make sure any writes to the color buffer have been finished
		//		imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		//		break;

		//	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		//		// Image is a depth/stencil attachment
		//		// Make sure any writes to the depth/stencil buffer have been finished
		//		imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		//		break;

		//	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		//		// Image is a transfer source 
		//		// Make sure any reads from the image have been finished
		//		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		//		break;

		//	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		//		// Image is a transfer destination
		//		// Make sure any writes to the image have been finished
		//		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		//		break;

		//	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		//		// Image is read by a shader
		//		// Make sure any shader reads from the image have been finished
		//		imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		//		break;
		//	default:
		//		// Other source layouts aren't handled (yet)
		//		break;
		//	}

		//	// Target layouts (new)
		//	// Destination access mask controls the dependency for the new image layout
		//	switch (newImageLayout)
		//	{
		//	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		//		// Image will be used as a transfer destination
		//		// Make sure any writes to the image have been finished
		//		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		//		break;

		//	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		//		// Image will be used as a transfer source
		//		// Make sure any reads from the image have been finished
		//		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		//		break;

		//	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		//		// Image will be used as a color attachment
		//		// Make sure any writes to the color buffer have been finished
		//		imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		//		break;

		//	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		//		// Image layout will be used as a depth/stencil attachment
		//		// Make sure any writes to depth/stencil buffer have been finished
		//		imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		//		break;

		//	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		//		// Image will be read in a shader (sampler, input attachment)
		//		// Make sure any writes to the image have been finished
		//		if (imageMemoryBarrier.srcAccessMask == 0)
		//		{
		//			imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
		//		}
		//		imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		//		break;
		//	default:
		//		// Other source layouts aren't handled (yet)
		//		break;
		//	}

		//	// Put barrier inside setup command buffer
		//	vkCmdPipelineBarrier(
		//		cmdbuffer,
		//		srcStageMask,
		//		dstStageMask,
		//		0,
		//		0, nullptr,
		//		0, nullptr,
		//		1, &imageMemoryBarrier);
		//}
		//void VulkanTexture::PipelineBarrier(
		//	VkCommandBuffer cmdbuffer,
		//	VkAccessFlags srcAccessMask,
		//	VkAccessFlags dstAccessMask,
		//	VkPipelineStageFlags srcStageMask,
		//	VkPipelineStageFlags dstStageMask
		//	)
		//{
		//	VkImageMemoryBarrier imageMemoryBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, nullptr, 
		//		srcAccessMask, dstAccessMask,
		//		m_descriptor.imageLayout, m_descriptor.imageLayout,
		//		VK_QUEUE_FAMILY_IGNORED,VK_QUEUE_FAMILY_IGNORED,
		//		m_vkImage,
		//		{ m_aspect, 0, m_mipLevelsCount, 0, m_layerCount }
		//	};

		//	vkCmdPipelineBarrier(
		//		cmdbuffer,
		//		srcStageMask,
		//		dstStageMask,
		//		VK_FLAGS_NONE,
		//		0, nullptr,
		//		0, nullptr,
		//		1, &imageMemoryBarrier);
		//}

		//void VulkanTexture::Update(TextureExtent** extents, VkBuffer stagingBuffer, VkCommandBuffer copyCmd, VkQueue copyQueue)
		//{
		//	// Image barrier for optimal image (target)
		//	ChangeLayout(copyCmd, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

		//	std::vector<VkBufferImageCopy> bufferCopyRegions;
		//	size_t offset = 0;

		//	for (uint32_t face = 0; face < m_layerCount; face++)
		//	{
		//		for (uint32_t level = 0; level < m_mipLevelsCount; level++)
		//		{
		//			VkBufferImageCopy bufferCopyRegion = {};
		//			bufferCopyRegion.imageSubresource.aspectMask = m_aspect;
		//			bufferCopyRegion.imageSubresource.mipLevel = level;
		//			bufferCopyRegion.imageSubresource.baseArrayLayer = face;
		//			bufferCopyRegion.imageSubresource.layerCount = 1;
		//			bufferCopyRegion.imageExtent.width = static_cast<uint32_t>(extents[face][level].width);
		//			bufferCopyRegion.imageExtent.height = static_cast<uint32_t>(extents[face][level].height);
		//			bufferCopyRegion.imageExtent.depth = 1;
		//			bufferCopyRegion.bufferOffset = offset;

		//			bufferCopyRegions.push_back(bufferCopyRegion);

		//			// Increase offset into staging buffer for next level / face
		//			offset += extents[face][level].size;
		//		}
		//	}

		//	// Copy the faces from the staging buffer to the optimal tiled image
		//	vkCmdCopyBufferToImage(
		//		copyCmd,
		//		stagingBuffer,
		//		m_vkImage,
		//		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		//		static_cast<uint32_t>(bufferCopyRegions.size()),
		//		bufferCopyRegions.data());

		//	// Change texture image layout to shader read after all faces have been copied
		//	ChangeLayout(copyCmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_descriptor.imageLayout, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
		//}

		//void VulkanTexture::UpdateGeneratingMipmaps(TextureExtent** extents, VkBuffer stagingBuffer, VkCommandBuffer copyCmd, VkQueue copyQueue)
		//{
		//	ChangeLayout(copyCmd, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

		//	size_t offset = 0;
		//	for (uint32_t face = 0; face < m_layerCount; face++)
		//	{
		//		VkBufferImageCopy bufferCopyRegion = {};
		//		bufferCopyRegion.imageSubresource.aspectMask = m_aspect;
		//		bufferCopyRegion.imageSubresource.mipLevel = 0;
		//		bufferCopyRegion.imageSubresource.baseArrayLayer = face;
		//		bufferCopyRegion.imageSubresource.layerCount = 1;
		//		bufferCopyRegion.imageExtent.width = static_cast<uint32_t>(extents[face][0].width);
		//		bufferCopyRegion.imageExtent.height = static_cast<uint32_t>(extents[face][0].height);
		//		bufferCopyRegion.imageExtent.depth = 1;
		//		bufferCopyRegion.bufferOffset = offset;

		//		offset += extents[face][0].size;

		//		// Copy the faces from the staging buffer to the optimal tiled image
		//		vkCmdCopyBufferToImage(
		//			copyCmd,
		//			stagingBuffer,
		//			m_vkImage,
		//			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		//			1,
		//			&bufferCopyRegion);

		//		VkImageMemoryBarrier barrier{};
		//		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		//		barrier.image = m_vkImage;
		//		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		//		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		//		barrier.subresourceRange.aspectMask = m_aspect;
		//		barrier.subresourceRange.baseArrayLayer = face;
		//		barrier.subresourceRange.layerCount = 1;
		//		barrier.subresourceRange.levelCount = 1;

		//		int32_t mipWidth = extents[face][0].width;
		//		int32_t mipHeight = extents[face][0].height;

		//		for (uint32_t i = 1; i < m_mipLevelsCount; i++)
		//		{
		//			barrier.subresourceRange.baseMipLevel = i - 1;
		//			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		//			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		//			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		//			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		//			vkCmdPipelineBarrier(copyCmd,
		//				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
		//				0, nullptr,
		//				0, nullptr,
		//				1, &barrier);

		//			VkImageBlit blit{};
		//			blit.srcOffsets[0] = { 0, 0, 0 };
		//			blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
		//			blit.srcSubresource.aspectMask = m_aspect;
		//			blit.srcSubresource.mipLevel = i - 1;
		//			blit.srcSubresource.baseArrayLayer = face;
		//			blit.srcSubresource.layerCount = 1;
		//			blit.dstOffsets[0] = { 0, 0, 0 };
		//			blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
		//			blit.dstSubresource.aspectMask = m_aspect;
		//			blit.dstSubresource.mipLevel = i;
		//			blit.dstSubresource.baseArrayLayer = face;
		//			blit.dstSubresource.layerCount = 1;

		//			vkCmdBlitImage(copyCmd,
		//				m_vkImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		//				m_vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		//				1, &blit,
		//				VK_FILTER_LINEAR);

		//			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		//			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		//			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		//			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		//			vkCmdPipelineBarrier(copyCmd,
		//				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
		//				0, nullptr,
		//				0, nullptr,
		//				1, &barrier);

		//			if (mipWidth > 1) mipWidth /= 2;
		//			if (mipHeight > 1) mipHeight /= 2;
		//		}
		//		barrier.subresourceRange.baseMipLevel = m_mipLevelsCount - 1;
		//		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		//		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		//		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		//		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		//		vkCmdPipelineBarrier(copyCmd,
		//			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
		//			0, nullptr,
		//			0, nullptr,
		//			1, &barrier);
		//	}
		//}

		//void VulkanTexture::CreateDescriptor(VkSamplerAddressMode adressMode, VkImageViewType viewType, float maxAnisoropy)
		//{
		//	//TODO if not used in shaders no point in creating a sampler
		//	VkSamplerCreateInfo samplerCreateInfo{};
		//	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		//	samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
		//	samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
		//	samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		//	samplerCreateInfo.addressModeU = adressMode;
		//	samplerCreateInfo.addressModeV = samplerCreateInfo.addressModeU;
		//	samplerCreateInfo.addressModeW = samplerCreateInfo.addressModeU;
		//	samplerCreateInfo.mipLodBias = 0.0f;
		//	samplerCreateInfo.maxAnisotropy = maxAnisoropy;
		//	if (maxAnisoropy > 1.0f)
		//		samplerCreateInfo.anisotropyEnable = VK_TRUE;
		//	samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
		//	samplerCreateInfo.minLod = 0.0f;
		//	samplerCreateInfo.maxLod = (float)m_mipLevelsCount;
		//	samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
		//	VK_CHECK_RESULT(vkCreateSampler(_device, &samplerCreateInfo, nullptr, &m_descriptor.sampler));

		//	// Create image view
		//	VkImageViewCreateInfo viewCreateInfo{};
		//	viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		//	viewCreateInfo.viewType = viewType;
		//	viewCreateInfo.format = m_format;
		//	viewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
		//	viewCreateInfo.subresourceRange = { m_aspect, 0, m_mipLevelsCount, 0, m_layerCount };
		//	viewCreateInfo.image = m_vkImage;
		//	VK_CHECK_RESULT(vkCreateImageView(_device, &viewCreateInfo, nullptr, &m_descriptor.imageView));
		//}

		void Texture::Destroy()
		{
			/*if(m_descriptor.imageView)
				vkDestroyImageView(_device, m_descriptor.imageView, nullptr);

			if(m_vkImage)
				vkDestroyImage(_device, m_vkImage, nullptr);

			if (m_descriptor.sampler)
				vkDestroySampler(_device, m_descriptor.sampler, nullptr);

			if(m_deviceMemory)
				vkFreeMemory(_device, m_deviceMemory, nullptr);*/
		}
	}
}
