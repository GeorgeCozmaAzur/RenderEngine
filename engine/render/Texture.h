#pragma once
#include <stdlib.h>
#include <string>
#include <vector>

namespace engine
{
	namespace render
	{
		enum class GfxFormat {
			UNKNOWN,
			R8_UNORM,
			R8G8B8A8_UNORM,
			B8G8R8A8_UNORM,
			R16G16B16A16_UNORM,
			R16G16B16A16_SFLOAT,
			R32G32B32A32_SFLOAT,
			D24_UNORM_S8_UINT,
			D32_FLOAT,
			BC3_UNORM_BLOCK,
			ASTC_8x8_UNORM_BLOCK,
			ETC2_R8G8B8_UNORM_BLOCK
		};

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
			size_t m_imageSize;
			GfxFormat m_format;
			uint32_t m_width, m_height;
			TextureExtent** m_extents = nullptr;
			uint32_t m_layers_no, m_mips_no;
			bool isCubeMap = false;

			virtual void LoadFromFile(std::string filename, GfxFormat format) = 0;
			void Destroy();
			~TextureData() { Destroy(); }
		};

		struct Texture2DData : TextureData
		{
			virtual void LoadFromFile(std::string filename, GfxFormat format);
			virtual void LoadFromFiles(std::vector<std::string> filenames, GfxFormat format);
			void CreateFromBuffer(unsigned char* buffer, size_t bufferSize, uint32_t width, uint32_t height, GfxFormat format = GfxFormat::R8G8B8A8_UNORM);
		};

		struct TextureCubeMapData : TextureData
		{
			virtual void LoadFromFile(std::string filename, GfxFormat format);
		};

		/** @brief Vulkan texture base class */
		class Texture {//TODO see how can be combined with buffer structure and how we can have different textures with the same image
		public:

			GfxFormat m_format;
			size_t m_imageSize;

			uint32_t m_width, m_height, m_depth;
			uint32_t m_mipLevelsCount = 1;
			uint32_t m_layerCount = 1;

			virtual Texture::~Texture() { Destroy(); }

			/*void Create(VkDevice device, VkPhysicalDeviceMemoryProperties* memoryProperties, VkExtent3D extent, VkFormat format,
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

			void CreateDescriptor(VkSamplerAddressMode adressMode, VkImageViewType viewType, float maxAnisoropy = 1);*/

			/** @brief Release all Vulkan resources held by this texture */
			void Destroy();
		};
	}
}
