#pragma once

#include <stdlib.h>
#include <string>

#include "VulkanTools.h"

#if defined(__ANDROID__)
#include <android/asset_manager.h>
#endif

namespace engine
{
	namespace render
	{
		struct TextureExtent
		{
			int width;
			int height;
			size_t size;
		};

		struct TextureData
		{
			char* m_ram_data = nullptr;
			bool owndata = true;
			VkDeviceSize m_imageSize;
			VkFormat m_format;
			uint32_t m_width, m_height;
			TextureExtent** m_extents = nullptr;
			uint32_t m_layers_no, m_mips_no;

			VkBuffer m_stagingBuffer = VK_NULL_HANDLE;
			VkDeviceMemory m_stagingMemory = VK_NULL_HANDLE;

			virtual void LoadFromFile(std::string filename, VkFormat format) = 0;
			void CreateStagingBuffer(VkDevice, VkPhysicalDeviceMemoryProperties* memoryProperties);
			void Destroy(VkDevice);
		};

		struct Texture2DData : TextureData
		{
			virtual void LoadFromFile(std::string filename, VkFormat format);
			virtual void LoadFromFiles(std::vector<std::string> filenames, VkFormat format);
			void CreateFromBuffer(unsigned char* buffer, VkDeviceSize bufferSize, uint32_t width, uint32_t height);
		};

		struct TextureCubeMapData : TextureData
		{
			virtual void LoadFromFile(std::string filename, VkFormat format);
		};

		/** @brief Vulkan texture base class */
		class VulkanTexture {//TODO see how can be combined with buffer structure and how we can have different textures with the same image
		public:
			VkDevice _device = VK_NULL_HANDLE;

			VkImage m_vkImage = VK_NULL_HANDLE;
			VkDeviceMemory m_deviceMemory = VK_NULL_HANDLE;
			VkDescriptorImageInfo m_descriptor{};//TODO make all this private

			VkFormat m_format;
			VkDeviceSize m_imageSize;

			VkImageAspectFlags m_aspect = VK_IMAGE_ASPECT_COLOR_BIT;

			uint32_t m_width, m_height, m_depth;
			uint32_t m_mipLevelsCount = 1;
			uint32_t m_layerCount = 1;

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

			void Update(TextureData* textureData, VkCommandBuffer copyCmd, VkQueue copyQueue);

			void UpdateGeneratingMipmaps(TextureData* textureData, VkCommandBuffer copyCmd, VkQueue copyQueue);

			void CreateDescriptor(VkSamplerAddressMode adressMode, VkImageViewType viewType, float maxAnisoropy = 1);

			/** @brief Release all Vulkan resources held by this texture */
			void Destroy();
		};
	}
}
