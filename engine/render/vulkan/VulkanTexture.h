#pragma once

#include <stdlib.h>
#include <string>

#include "VulkanTools.h"
#include "render/Texture.h"

#if defined(__ANDROID__)
#include <android/asset_manager.h>
#endif

namespace engine
{
	namespace render
	{
		inline VkFormat ToVkFormat(GfxFormat format) {
			switch (format) {
			case GfxFormat::R8_UNORM: return VK_FORMAT_R8_UNORM;
			case GfxFormat::R8G8B8A8_UNORM: return VK_FORMAT_R8G8B8A8_UNORM;
			case GfxFormat::B8G8R8A8_UNORM: return VK_FORMAT_B8G8R8A8_UNORM;
			case GfxFormat::R16G16B16A16_UNORM: return VK_FORMAT_R16G16B16A16_UNORM;
			case GfxFormat::R16G16B16A16_SFLOAT: return VK_FORMAT_R16G16B16A16_SFLOAT;
			case GfxFormat::R32G32B32A32_SFLOAT: return VK_FORMAT_R32G32B32A32_SFLOAT;
			case GfxFormat::D24_UNORM_S8_UINT: return VK_FORMAT_D24_UNORM_S8_UINT;
			case GfxFormat::D32_FLOAT: return VK_FORMAT_D32_SFLOAT_S8_UINT;
			case GfxFormat::BC3_UNORM_BLOCK: return VK_FORMAT_BC3_UNORM_BLOCK;
			case GfxFormat::ASTC_8x8_UNORM_BLOCK: return VK_FORMAT_ASTC_8x8_UNORM_BLOCK;
			case GfxFormat::ETC2_R8G8B8_UNORM_BLOCK: return VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;
			default: return VK_FORMAT_UNDEFINED;
			}
		}

		/** @brief Vulkan texture base class */
		class VulkanTexture : public Texture {//TODO see how can be combined with buffer structure and how we can have different textures with the same image
		public:
			VkDevice _device = VK_NULL_HANDLE;

			VkImage m_vkImage = VK_NULL_HANDLE;
			VkDeviceMemory m_deviceMemory = VK_NULL_HANDLE;
			VkDescriptorImageInfo m_descriptor{};//TODO make all this private

			VkFormat m_format;
			VkDeviceSize m_imageSize;

			VkImageAspectFlags m_aspect = VK_IMAGE_ASPECT_COLOR_BIT;

			VulkanTexture::~VulkanTexture() { Destroy(); }

			void Create(VkDevice device, VkPhysicalDeviceMemoryProperties* memoryProperties, VkExtent3D extent, VkFormat format,
				VkImageUsageFlags imageUsageFlags,
				VkImageLayout imageLayout,
				VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT,
				uint32_t mipLevelsCount = 1, uint32_t layersCount = 1, VkImageCreateFlags flags = 0
			);

			void ChangeLayout(
				VkCommandBuffer cmdbuffer,
				VkImageLayout oldImageLayout,
				VkImageLayout newImageLayout,
				VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
				VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

			void PipelineBarrier(
				VkCommandBuffer cmdbuffer,
				VkAccessFlags srcAccessMask,
				VkAccessFlags dstAccessMask,
				VkPipelineStageFlags srcStageMask,
				VkPipelineStageFlags dstStageMask
				);

			void Update(TextureExtent** extents, VkBuffer stagingBuffer, VkCommandBuffer copyCmd, VkQueue copyQueue);

			void UpdateGeneratingMipmaps(TextureExtent** extents, VkBuffer stagingBuffer, VkCommandBuffer copyCmd, VkQueue copyQueue);

			void CreateDescriptor(VkSamplerAddressMode adressMode, VkImageViewType viewType, float maxAnisoropy = 1);

			/** @brief Release all Vulkan resources held by this texture */
			void Destroy();
		};
	}
}
