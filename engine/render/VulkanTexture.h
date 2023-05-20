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
			VkDeviceSize m_imageSize;
			VkFormat m_format;
			uint32_t m_width, m_height;
			TextureExtent** m_extents;
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
		};

		struct TextureCubeMapData : TextureData
		{
			virtual void LoadFromFile(std::string filename, VkFormat format);
		};

		/** @brief Vulkan texture base class */
		class VulkanTexture {//TODO see how can be combined with buffer structure and how we can have different textures with the same image
		public:
			VkDevice _device = VK_NULL_HANDLE;

			VkImage m_vkImage;
			VkDeviceMemory m_deviceMemory;
			VkDescriptorImageInfo m_descriptor;//TODO make all this private

			VkFormat m_format;
			VkDeviceSize m_imageSize;

			uint32_t m_width, m_height, m_depth;
			uint32_t m_mipLevelsCount;
			uint32_t m_layerCount;

			VulkanTexture::~VulkanTexture() { Destroy(); }

			void Create(VkDevice device, VkPhysicalDeviceMemoryProperties* memoryProperties, VkExtent3D extent, VkFormat format,
				VkImageUsageFlags imageUsageFlags,
				VkImageLayout imageLayout,
				uint32_t mipLevelsCount = 1, uint32_t layersCount = 1, VkImageCreateFlags flags = 0
			);

			void Update(TextureData* textureData, VkCommandBuffer copyCmd, VkQueue copyQueue);

			void CreateDescriptor(VkSamplerAddressMode adressMode, VkImageViewType viewType, VkImageAspectFlags aspect, float maxAnisoropy = 1);

			/** @brief Release all Vulkan resources held by this texture */
			void Destroy();
		};
	}
}
