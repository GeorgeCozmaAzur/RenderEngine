#include <stdlib.h>
#include <fstream>
#include <vector>

#if !defined(__ANDROID__)
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#endif

#include <gli/gli.hpp>

#include "VulkanTexture.h"
#include "VulkanTools.h"

#if defined(__ANDROID__)
#include <android/asset_manager.h>
#endif

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
			VkFormat format
		)
		{
			m_format = format;


			if (filename.find(".ktx") != std::string::npos)
			{
#if defined(__ANDROID__)
				// Textures are stored inside the apk on Android (compressed)
				// So they need to be loaded via the asset manager
				AAsset* asset = AAssetManager_open(androidApp->activity->assetManager, filename.c_str(), AASSET_MODE_STREAMING);
				if (!asset) {
					engine::tools::exitFatal("Could not load texture from " + filename + "\n\nThe file may be part of the additional asset pack.\n\nRun \"download_assets.py\" in the repository root to download the latest version.", -1);
				}
				size_t size = AAsset_getLength(asset);
				assert(size > 0);

				void* textureData = malloc(size);
				AAsset_read(asset, textureData, size);
				AAsset_close(asset);

				gli::texture2d tex2D(gli::load((const char*)textureData, size));

				free(textureData);
#else
				if (!engine::tools::fileExists(filename)) {
					engine::tools::exitFatal("Could not load texture from " + filename + "\n\nThe file may be part of the additional asset pack.\n\nRun \"download_assets.py\" in the repository root to download the latest version.", -1);
				}
				gli::texture2d tex2D(gli::load(filename.c_str()));
#endif		
				assert(!tex2D.empty());

				m_width = static_cast<uint32_t>(tex2D[0].extent().x);
				m_height = static_cast<uint32_t>(tex2D[0].extent().y);

				m_imageSize = tex2D.size();
				m_ram_data = new char[m_imageSize];
				memcpy(m_ram_data, tex2D.data(), m_imageSize);

				m_layers_no = 1;
				m_mips_no = static_cast<uint32_t>(tex2D.levels());

				m_extents = new TextureExtent * [1];
				m_extents[0] = new TextureExtent[m_mips_no];

				for (uint32_t i = 0; i < m_mips_no; i++)
				{
					m_extents[0][i].width = tex2D[i].extent().x;
					m_extents[0][i].height = tex2D[i].extent().y;
					m_extents[0][i].size = tex2D[i].size();

				}
			}
#if !defined(__ANDROID__)
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
#endif
		}
		void TextureCubeMapData::LoadFromFile(
			std::string filename,
			VkFormat format
		)
		{
			m_format = format;
#if defined(__ANDROID__)
			// Textures are stored inside the apk on Android (compressed)
			// So they need to be loaded via the asset manager
			AAsset* asset = AAssetManager_open(androidApp->activity->assetManager, filename.c_str(), AASSET_MODE_STREAMING);
			if (!asset) {
				engine::tools::exitFatal("Could not load texture from " + filename + "\n\nThe file may be part of the additional asset pack.\n\nRun \"download_assets.py\" in the repository root to download the latest version.", -1);
			}
			size_t size = AAsset_getLength(asset);
			assert(size > 0);

			void* textureData = malloc(size);
			AAsset_read(asset, textureData, size);
			AAsset_close(asset);

			gli::texture_cube texCube(gli::load((const char*)textureData, size));

			free(textureData);
#else
			if (!engine::tools::fileExists(filename)) {
				engine::tools::exitFatal("Could not load texture from " + filename + "\n\nThe file may be part of the additional asset pack.\n\nRun \"download_assets.py\" in the repository root to download the latest version.", -1);
			}
			gli::texture_cube texCube(gli::load(filename));
#endif	
			assert(!texCube.empty());

			m_width = static_cast<uint32_t>(texCube.extent().x);
			m_height = static_cast<uint32_t>(texCube.extent().y);
			m_mips_no = static_cast<uint32_t>(texCube.levels());
			m_layers_no = 6;

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
				}
			}
		}

		void TextureData::CreateStagingBuffer(VkDevice _device, VkPhysicalDeviceMemoryProperties* memoryProperties)
		{
			VkMemoryAllocateInfo memAllocInfo = engine::initializers::memoryAllocateInfo();
			VkMemoryRequirements memReqs;

			VkBufferCreateInfo bufferCreateInfo = engine::initializers::bufferCreateInfo();
			bufferCreateInfo.size = m_imageSize;
			// This buffer is used as a transfer source for the buffer copy
			bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			VK_CHECK_RESULT(vkCreateBuffer(_device, &bufferCreateInfo, nullptr, &m_stagingBuffer));

			// Get memory requirements for the staging buffer (alignment, memory type bits)
			vkGetBufferMemoryRequirements(_device, m_stagingBuffer, &memReqs);

			memAllocInfo.allocationSize = memReqs.size;
			// Get memory type index for a host visible buffer
			memAllocInfo.memoryTypeIndex = tools::getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, memoryProperties);

			VK_CHECK_RESULT(vkAllocateMemory(_device, &memAllocInfo, nullptr, &m_stagingMemory));
			VK_CHECK_RESULT(vkBindBufferMemory(_device, m_stagingBuffer, m_stagingMemory, 0));

			// Copy texture data into staging buffer
			uint8_t* data;
			VK_CHECK_RESULT(vkMapMemory(_device, m_stagingMemory, 0, memReqs.size, 0, (void**)&data));
			memcpy(data, m_ram_data, m_imageSize);
			vkUnmapMemory(_device, m_stagingMemory);
		}

		void TextureData::Destroy(VkDevice _device)
		{
			for (uint32_t i = 0; i < m_layers_no; i++)
			{
				delete[]m_extents[i];
			}
			delete[]m_extents;

			delete[] m_ram_data;

			// Clean up staging resources
			if (m_stagingMemory)
				vkFreeMemory(_device, m_stagingMemory, nullptr);
			if (m_stagingBuffer)
				vkDestroyBuffer(_device, m_stagingBuffer, nullptr);
		}

		void VulkanTexture::Create(VkDevice device, VkPhysicalDeviceMemoryProperties* memoryProperties, VkExtent3D extent, VkFormat format,
			VkImageUsageFlags imageUsageFlags,
			VkImageLayout imageLayout,
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

			VkMemoryAllocateInfo memAllocInfo = engine::initializers::memoryAllocateInfo();
			VkMemoryRequirements memReqs;

			// Create optimal tiled target image
			VkImageCreateInfo imageCreateInfo = engine::initializers::imageCreateInfo();

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

		void VulkanTexture::Update(TextureData* textureData, VkCommandBuffer copyCmd, VkQueue copyQueue)
		{
			std::vector<VkBufferImageCopy> bufferCopyRegions;
			size_t offset = 0;

			for (uint32_t face = 0; face < m_layerCount; face++)
			{
				for (uint32_t level = 0; level < m_mipLevelsCount; level++)
				{
					VkBufferImageCopy bufferCopyRegion = {};
					bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					bufferCopyRegion.imageSubresource.mipLevel = level;
					bufferCopyRegion.imageSubresource.baseArrayLayer = face;
					bufferCopyRegion.imageSubresource.layerCount = 1;
					bufferCopyRegion.imageExtent.width = static_cast<uint32_t>(textureData->m_extents[face][level].width);
					bufferCopyRegion.imageExtent.height = static_cast<uint32_t>(textureData->m_extents[face][level].height);
					bufferCopyRegion.imageExtent.depth = 1;
					bufferCopyRegion.bufferOffset = offset;

					bufferCopyRegions.push_back(bufferCopyRegion);

					// Increase offset into staging buffer for next level / face
					offset += textureData->m_extents[face][level].size;
				}
			}

			// Image barrier for optimal image (target)
			// Set initial layout for all array layers (faces) of the optimal (target) tiled texture
			VkImageSubresourceRange subresourceRange = {};
			subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subresourceRange.baseMipLevel = 0;
			subresourceRange.levelCount = m_mipLevelsCount;
			subresourceRange.layerCount = m_layerCount;

			engine::tools::setImageLayout(
				copyCmd,
				m_vkImage,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				subresourceRange);

			// Copy the cube map faces from the staging buffer to the optimal tiled image
			vkCmdCopyBufferToImage(
				copyCmd,
				textureData->m_stagingBuffer,
				m_vkImage,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				static_cast<uint32_t>(bufferCopyRegions.size()),
				bufferCopyRegions.data());

			// Change texture image layout to shader read after all faces have been copied
			//this->descriptor.imageLayout = imageLayout;
			engine::tools::setImageLayout(
				copyCmd,
				m_vkImage,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				m_descriptor.imageLayout,
				subresourceRange);
		}

		void VulkanTexture::CreateDescriptor(VkSamplerAddressMode adressMode, VkImageViewType viewType, VkImageAspectFlags aspect, float maxAnisoropy)
		{
			//TODO if not used in shaders no point in creating a sampler
			VkSamplerCreateInfo samplerCreateInfo = engine::initializers::samplerCreateInfo();
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
			samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
			VK_CHECK_RESULT(vkCreateSampler(_device, &samplerCreateInfo, nullptr, &m_descriptor.sampler));

			// Create image view
			VkImageViewCreateInfo viewCreateInfo = engine::initializers::imageViewCreateInfo();
			viewCreateInfo.viewType = viewType;
			viewCreateInfo.format = m_format;
			viewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
			viewCreateInfo.subresourceRange = { aspect, 0, 1, 0, 1 };
			viewCreateInfo.subresourceRange.layerCount = m_layerCount;
			viewCreateInfo.subresourceRange.levelCount = m_mipLevelsCount;
			viewCreateInfo.image = m_vkImage;
			VK_CHECK_RESULT(vkCreateImageView(_device, &viewCreateInfo, nullptr, &m_descriptor.imageView));
		}

		void VulkanTexture::Destroy()
		{
			vkDestroyImageView(_device, m_descriptor.imageView, nullptr);
			vkDestroyImage(_device, m_vkImage, nullptr);
			if (m_descriptor.sampler)
			{
				vkDestroySampler(_device, m_descriptor.sampler, nullptr);
			}
			vkFreeMemory(_device, m_deviceMemory, nullptr);
		}
	}
}
