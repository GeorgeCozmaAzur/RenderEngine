#pragma once
#include "vulkan/vulkan.h"
#include <vector>
#include <map>
#include "VulkanBuffer.hpp"
#include "render/VulkanTexture.h"
#include "scene/RenderObject.h"
#include "VulkanRenderPass.h"
#include "threadpool.hpp"

namespace engine
{
	namespace render
	{
		class VulkanDevice
		{
		public:
			// Structure to store queue family indices
			struct QueueFamilyIndices {
				bool hasGraphivsValue = false;  // Indicates if graphics family index is set
				uint32_t graphicsFamily;        // Graphics family index
				bool hasPresentValue = false;   // Indicates if present family index is set
				uint32_t presentFamily;         // Present family index
				// Checks if both graphics and present family indices are set
				bool isComplete() { return hasGraphivsValue && hasPresentValue; }
			};
			QueueFamilyIndices queueFamilyIndices;

			VkInstance _instance;  // Vulkan instance
			VkPhysicalDeviceProperties m_properties;  // Properties of the physical device
			VkPhysicalDeviceFeatures m_enabledFeatures;  // Enabled features of the physical device
			VkPhysicalDeviceMemoryProperties memoryProperties;  // Memory properties of the physical device
			std::vector<VkQueueFamilyProperties> m_queueFamilyProperties;  // Queue family properties
			std::vector<const char*> m_enabledExtensions;  // Enabled extensions
			VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;  // Handle to the physical device
			VkDevice logicalDevice = VK_NULL_HANDLE;  // Handle to the logical device
			std::map<uint32_t, VkCommandPool> m_commandPools;  // Command pools for each queue family index
			std::vector<VkCommandBuffer> drawCommandBuffers;  // Draw command buffers
			uint32_t drawCommandBuffersPoolIndex = 0;  // Index of the draw command buffers pool
			VkCommandBuffer computeCommandBuffer = VK_NULL_HANDLE;  // Compute command buffer
			uint32_t computeCommandBuffersPoolIndex = 0;  // Index of the compute command buffers pool

			bool enableDebugMarkers = false;  // Indicates if debug markers extension is enabled
			VkPipelineCache pipelineCache = VK_NULL_HANDLE;  // Pipeline cache object

			VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;  // Descriptor pool
			std::vector<VulkanBuffer*> m_buffers;  // Graphical resources - buffers
			std::vector<VulkanBuffer*> m_stagingBuffers;  // Graphical resources - staging buffers
			std::vector<VulkanTexture*> m_textures;  // Graphical resources - textures
			std::vector<VulkanPipeline*> m_pipelines;  // Graphical resources - pipelines
			std::vector<VulkanDescriptorSet*> m_descriptorSets;  // Graphical resources - descriptor sets
			std::vector<VulkanDescriptorSetLayout*> m_descriptorSetLayouts;  // Graphical resources - descriptor set layouts
			std::vector<VulkanRenderPass*> m_renderPasses;  // Graphical resources - render passes
			std::vector<VulkanFrameBuffer*> m_frameBuffers;  // Graphical resources - frame buffers
			std::vector<VkSemaphore> m_semaphores;  // Semaphores

			// Typecast to VkDevice
			operator VkDevice() { return logicalDevice; }

		public:
			// Constructor
			VulkanDevice(VkInstance instance, VkPhysicalDeviceFeatures* wantedFeatures, std::vector<const char*> wantedExtensions, VkSurfaceKHR surface, void* pNextChain);

			// Selects a physical device
			void GetPhysicalDevice(VkPhysicalDeviceFeatures* wantedFeatures, std::vector<const char*> wantedExtensions, VkSurfaceKHR surface);

			// Creates a logical device
			void CreateLogicalDevice(const std::vector<const char*> layers, void* pNextChain = nullptr);

			// Returns the name of the device
			char* GetDeviceName()
			{
				return m_properties.deviceName;
			}

			// Selected a suitable supported depth format starting with 32 bit down to 16 bit
			// Returns false if none of the depth formats in the list is supported by the device
			VkBool32 GetSupportedDepthFormat(VkFormat* depthFormat);

			// Creates a staging buffer
			VulkanBuffer* CreateStagingBuffer(VkDeviceSize size, void* data = nullptr);

			// Destroys a staging buffer
			void DestroyStagingBuffer(VulkanBuffer* buffer);

			// Copies data from one buffer to another
			void CopyBuffer(VulkanBuffer* src, VulkanBuffer* dst, VkCommandBuffer copyCmd, VkBufferCopy* copyRegion = nullptr);

			// Gets a buffer with specified usage and memory properties
			VulkanBuffer* GetBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceSize size, void* data = nullptr);

			// Gets a uniform buffer
			VulkanBuffer* GetUniformBuffer(VkDeviceSize size, bool frequentUpdate = true, VkQueue queue = VK_NULL_HANDLE, void* data = nullptr);

			// Gets a geometry buffer
			VulkanBuffer* GetGeometryBuffer(VkBufferUsageFlags usageFlags, VkQueue queue, VkDeviceSize size, void* data = nullptr, VkMemoryPropertyFlags memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			// Destroys a buffer
			void DestroyBuffer(VulkanBuffer* buffer);

			// Gets a texture from a file
			VulkanTexture* GetTexture(std::string filename, VkFormat format, VkQueue copyQueue,
				VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
				VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, bool generateMipmaps = false);

			// Gets a texture array from a list of files
			VulkanTexture* GetTextureArray(std::vector<std::string> filenames, VkFormat format, VkQueue copyQueue,
				VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
				VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, bool generateMipmaps = false);

			// Gets a cubemap texture from a file
			VulkanTexture* GetTextureCubeMap(std::string filename, VkFormat format, VkQueue copyQueue, VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
				VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

			// Gets a cubemap texture with specified dimensions
			VulkanTexture* GetTextureCubeMap(uint32_t dimension, VkFormat format, VkQueue copyQueue, VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
				VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

			// Gets a texture for storage
			VulkanTexture* GetTextureStorage(VkExtent3D extent, VkFormat format, VkQueue copyQueue,
				VkImageViewType viewType,
				VkImageLayout imageLayout = VK_IMAGE_LAYOUT_GENERAL,
				uint32_t mipLevelsCount = 1U, uint32_t layersCount = 1U);

			// Gets a render target texture
			VulkanTexture* GetRenderTarget(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspect, VkImageLayout imageLayout);

			// Gets a color render target texture
			VulkanTexture* GetColorRenderTarget(uint32_t width, uint32_t height, VkFormat format);

			// Gets a depth render target texture
			VulkanTexture* GetDepthRenderTarget(uint32_t width, uint32_t height, bool useInShaders, VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT, bool withStencil = true, bool updateLayout = false, VkQueue copyQueue = VK_NULL_HANDLE);

			// Destroys a texture
			void DestroyTexture(VulkanTexture* texture);

			// Creates a pipeline cache
			VkPipelineCache CreatePipelineCache();

			// Destroys the pipeline cache
			void DestroyPipelineCache();

			// Gets a graphics pipeline
			VulkanPipeline* GetPipeline(VkDescriptorSetLayout descriptorSetLayout,
				std::vector<VkVertexInputBindingDescription> vertexInputBindings, std::vector<VkVertexInputAttributeDescription> vertexInputAttributes,
				std::string vertexFile, std::string fragmentFile,
				VkRenderPass renderPass, VkPipelineCache cache,
				bool blendEnable = false,
				VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
				uint32_t vertexConstantBlockSize = 0,
				uint32_t* fConstants = nullptr,
				uint32_t attachmentCount = 0,
				const VkPipelineColorBlendAttachmentState* pAttachments = nullptr,
				bool depthBias = false,
				bool depthTestEnable = true,
				bool depthWriteEnable = true,
				uint32_t subpass = 0);

			// Gets a graphics pipeline with additional properties
			VulkanPipeline* GetPipeline(VkDescriptorSetLayout descriptorSetLayout,
				std::vector<VkVertexInputBindingDescription> vertexInputBindings, std::vector<VkVertexInputAttributeDescription> vertexInputAttributes,
				std::string vertexFile, std::string fragmentFile,
				VkRenderPass renderPass, VkPipelineCache cache,
				PipelineProperties properties);

			// Gets a compute pipeline
			VulkanPipeline* GetComputePipeline(std::string file, VkDevice device, VkDescriptorSetLayout descriptorSetLayout, VkPipelineCache cache, uint32_t constanBlockSize = 0);

			// Creates a descriptor sets pool
			void CreateDescriptorSetsPool(std::vector<VkDescriptorPoolSize> poolSizes, uint32_t maxSets);

			// Gets a descriptor set layout
			VulkanDescriptorSetLayout* GetDescriptorSetLayout(std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> layoutbindigs);

			// Gets a descriptor set
			VulkanDescriptorSet* GetDescriptorSet(std::vector<VkDescriptorBufferInfo*> buffersDescriptors,
				std::vector<VkDescriptorImageInfo*> texturesDescriptors,
				VkDescriptorSetLayout layout, std::vector<VkDescriptorSetLayoutBinding> layoutBindings, uint32_t dynamicAllingment = 0);

			// Gets a framebuffer
			VulkanFrameBuffer* GetFrameBuffer(VkRenderPass renderPass, int32_t width, int32_t height, std::vector<VkImageView> textures, VkClearColorValue  clearColor = { { 0.0f, 0.0f, 0.0f, 0.0f } });

			// Destroys a framebuffer
			void DestroyFrameBuffer(VulkanFrameBuffer* fb);

			// Gets a render pass
			VulkanRenderPass* GetRenderPass(std::vector <VulkanTexture*> textures, std::vector<RenderSubpass> subpasses = {});

			// Overloaded function to get a render pass with additional properties
			VulkanRenderPass* GetRenderPass(std::vector <std::pair<VkFormat, VkImageLayout>> layouts);

			// Creates a command pool
			VkCommandPool CreateCommandPool(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags createFlags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

			// Gets a command pool for a specific queue family index
			VkCommandPool GetCommandPool(uint32_t queueFamilyIndex);

			// Destroys a command pool
			void DestroyCommandPool(VkCommandPool cmdPool);

			// Creates a command buffer
			VkCommandBuffer CreateCommandBuffer(VkCommandBufferLevel level, bool begin = false);

			// Creates draw command buffers
			std::vector<VkCommandBuffer> CreatedrawCommandBuffers(uint32_t size, uint32_t queueFamilyIndex);

			// Frees draw command buffers
			void FreeDrawCommandBuffers();

			// Creates a compute command buffer
			VkCommandBuffer CreateComputeCommandBuffer(uint32_t queueFamilyIndex);

			// Frees the compute command buffer
			void FreeComputeCommandBuffer();

			// Flushes a command buffer
			void FlushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, bool free = true);

			// Gets a semaphore
			VkSemaphore GetSemaphore();

			// Destroys a semaphore
			void DestroySemaphore(VkSemaphore semaphore);

			// Destructor
			~VulkanDevice();
		};
	}
}