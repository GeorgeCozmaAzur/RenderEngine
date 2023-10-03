/*
* Vulkan device class
*
* Encapsulates a physical Vulkan device and it's logical representation
*
* Copyright (C) by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#pragma once

#include <exception>
#include <assert.h>
#include <algorithm>
#include "vulkan/vulkan.h"
#include "VulkanTools.h"
#include "VulkanBuffer.hpp"
#include "render/VulkanTexture.h"
#include "scene/RenderObject.h"
#include "VulkanRenderPass.h"
#include "threadpool.hpp"

namespace engine
{	
	namespace render
	{
	struct VulkanDevice
	{
		/** @brief Physical device representation */
		VkPhysicalDevice physicalDevice;
		/** @brief Logical device representation (application's view of the device) */
		VkDevice logicalDevice;
		/** @brief Properties of the physical device including limits that the application can check against */
		VkPhysicalDeviceProperties properties;
		/** @brief Features of the physical device that an application can use to check if a feature is supported */
		VkPhysicalDeviceFeatures features;
		/** @brief Features that have been enabled for use on the physical device */
		VkPhysicalDeviceFeatures enabledFeatures;
		/** @brief Memory types and heaps of the physical device */
		VkPhysicalDeviceMemoryProperties memoryProperties;
		/** @brief Queue family properties of the physical device */
		std::vector<VkQueueFamilyProperties> queueFamilyProperties;
		/** @brief List of extensions supported by the device */
		std::vector<std::string> supportedExtensions;

		/** @brief All command buffers pools for each queue family index */
		std::map<uint32_t, VkCommandPool> m_commandPools;
		/** @brief All draw command buffers */
		std::vector<VkCommandBuffer> drawCommandBuffers;
		uint32_t drawCommandBuffersPoolIndex = 0;
		/** @brief All compute command buffers */
		VkCommandBuffer computeCommandBuffer = VK_NULL_HANDLE;
		uint32_t computeCommandBuffersPoolIndex = 0;

		/** @brief Set to true when the debug marker extension is detected */
		bool enableDebugMarkers = false;

		/** @brief Contains queue family indices */
		struct
		{
			uint32_t graphics;
			uint32_t compute;
			uint32_t transfer;
		} queueFamilyIndices;

		// Pipeline cache object
		VkPipelineCache pipelineCache = VK_NULL_HANDLE;

		/** @brief All graphical resources */
		// Descriptor set pool
		VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
		std::vector<VulkanBuffer*> m_buffers;
		std::vector<VulkanBuffer*> m_stagingBuffers;
		std::vector<VulkanTexture*> m_textures;
		std::vector<VulkanPipeline *> m_pipelines;
		std::vector<VulkanDescriptorSet*> m_descriptorSets;
		std::vector<VulkanDescriptorSetLayout*> m_descriptorSetLayouts;
		std::vector<VulkanRenderPass*> m_renderPasses;
		std::vector<VulkanFrameBuffer*> m_frameBuffers;
		std::vector<VkSemaphore> m_semaphores;

		/**  @brief Typecast to VkDevice */
		operator VkDevice() { return logicalDevice; };

		/**
		* Default constructor
		*
		* @param physicalDevice Physical device that is to be used
		*/
		//VulkanDevice(VkPhysicalDevice physicalDevice)
		VulkanDevice(VkInstance instance, uint32_t gpuIndex, VkPhysicalDeviceFeatures *_enabledFeatures, std::vector<const char*> enabledDeviceExtensions, void *deviceCreatepNextChain)//TODO maybe add this code in a create function
		{
			VkResult err;

			// Physical device
			uint32_t gpuCount = 0;
			// Get number of available physical devices
			VK_CHECK_RESULT(vkEnumeratePhysicalDevices(instance, &gpuCount, nullptr));
			assert(gpuCount > 0);
			// Enumerate devices
			std::vector<VkPhysicalDevice> physicalDevices(gpuCount);
			err = vkEnumeratePhysicalDevices(instance, &gpuCount, physicalDevices.data());
			if (err) {
				engine::tools::exitFatal("Could not enumerate physical devices : \n" + engine::tools::errorString(err), err);
			}

			uint32_t selectedDevice = 0;
			physicalDevice = physicalDevices[selectedDevice];

			assert(physicalDevice);
			this->physicalDevice = physicalDevice;

			// Store Properties features, limits and properties of the physical device for later use
			// Device properties also contain limits and sparse properties
			vkGetPhysicalDeviceProperties(physicalDevice, &properties);
			// Features should be checked by the examples before using them
			vkGetPhysicalDeviceFeatures(physicalDevice, &features);

			VkBool32 *features_bool = (VkBool32*)(&features);
			VkBool32 *features_wanted_bool = (VkBool32*)_enabledFeatures;
			VkBool32 *features_activated_bool = (VkBool32*)(&enabledFeatures);
			uint32_t features_no = sizeof(VkPhysicalDeviceFeatures) / sizeof(VkBool32);
			for (uint32_t i = 0; i < features_no; i++)
			{
				features_activated_bool[i] = features_bool[i] & features_wanted_bool[i];
			}

			// Memory properties are used regularly for creating all kinds of buffers
			vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
			// Queue family properties, used for setting up requested queues upon device creation
			uint32_t queueFamilyCount;
			vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
			assert(queueFamilyCount > 0);
			queueFamilyProperties.resize(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProperties.data());

			// Get list of supported extensions
			uint32_t extCount = 0;
			vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extCount, nullptr);
			if (extCount > 0)
			{
				std::vector<VkExtensionProperties> extensions(extCount);
				if (vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extCount, &extensions.front()) == VK_SUCCESS)
				{
					for (auto ext : extensions)
					{
						supportedExtensions.push_back(ext.extensionName);
					}
				}
			}

			VkResult res = createLogicalDevice(enabledFeatures, enabledDeviceExtensions, deviceCreatepNextChain);
			if (res != VK_SUCCESS) {
				engine::tools::exitFatal("Could not create Vulkan device: \n" + engine::tools::errorString(res), res);
				//return false;
			}
		}

		/** 
		* Default destructor
		*
		* @note Frees the logical device
		*/
		~VulkanDevice()
		{
			freeDrawCommandBuffers();
			freeComputeCommandBuffer();
			for (auto cmdPool : m_commandPools)
			{
				vkDestroyCommandPool(logicalDevice, cmdPool.second, nullptr);
			}
			m_commandPools.clear();

			for (auto layout : m_descriptorSetLayouts)
				delete layout;
			for (auto desc : m_descriptorSets)
				delete desc;
			for (auto pipeline : m_pipelines)
				delete pipeline;
			for (auto buffer : m_buffers)
				delete buffer;
			for (auto buffer : m_stagingBuffers)
				delete buffer;
			for (auto tex : m_textures)
				delete tex;
			for (auto pass : m_renderPasses)
				delete pass;
			for (auto fb : m_frameBuffers)
				delete fb;		
			for (auto sm : m_semaphores)
				vkDestroySemaphore(logicalDevice, sm, nullptr);

			if (m_descriptorPool != VK_NULL_HANDLE)
			{
				vkDestroyDescriptorPool(logicalDevice, m_descriptorPool, nullptr);
			}

			if (logicalDevice)
			{
				vkDestroyDevice(logicalDevice, nullptr);
			}
		}

		char *GetDeviceName()
		{
			return properties.deviceName;;
		}

		/**
		* Get the index of a queue family that supports the requested queue flags
		*
		* @param queueFlags Queue flags to find a queue family index for
		*
		* @return Index of the queue family index that matches the flags
		*
		* @throw Throws an exception if no queue family index could be found that supports the requested flags
		*/
		uint32_t getQueueFamilyIndex(VkQueueFlagBits queueFlags)
		{
			
			// Dedicated queue for compute
			// Try to find a queue family index that supports compute but not graphics
			if (queueFlags & VK_QUEUE_COMPUTE_BIT)
			{
				for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++)
				{
					if ((queueFamilyProperties[i].queueFlags & queueFlags) && ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
					{
						return i;
						break;
					}
				}
			}

			// Dedicated queue for transfer
			// Try to find a queue family index that supports transfer but not graphics and compute
			if (queueFlags & VK_QUEUE_TRANSFER_BIT)
			{
				for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++)
				{
					if ((queueFamilyProperties[i].queueFlags & queueFlags) && ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) && ((queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0))
					{
						return i;
						break;
					}
				}
			}

			// For other queue types or if no separate compute queue is present, return the first one to support the requested flags
			for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++)
			{
				if (queueFamilyProperties[i].queueFlags & queueFlags)
				{
					return i;
					break;
				}
			}

			throw std::runtime_error("Could not find a matching queue family index");
		}

		/**
		* Create the logical device based on the assigned physical device, also gets default queue family indices
		*
		* @param enabledFeatures Can be used to enable certain features upon device creation
		* @param pNextChain Optional chain of pointer to extension structures
		* @param useSwapChain Set to false for headless rendering to omit the swapchain device extensions
		* @param requestedQueueTypes Bit flags specifying the queue types to be requested from the device  
		*
		* @return VkResult of the device creation call
		*/
		VkResult createLogicalDevice(VkPhysicalDeviceFeatures enabledFeatures, std::vector<const char*> enabledExtensions, void* pNextChain, bool useSwapChain = true, VkQueueFlags requestedQueueTypes = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)
		{			
			// Desired queues need to be requested upon logical device creation
			// Due to differing queue family configurations of Vulkan implementations this can be a bit tricky, especially if the application
			// requests different queue types

			std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};

			// Get queue family indices for the requested queue family types
			// Note that the indices may overlap depending on the implementation

			const float defaultQueuePriority(0.0f);

			// Graphics queue
			if (requestedQueueTypes & VK_QUEUE_GRAPHICS_BIT)
			{
				queueFamilyIndices.graphics = getQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT);
				VkDeviceQueueCreateInfo queueInfo{};
				queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queueInfo.queueFamilyIndex = queueFamilyIndices.graphics;
				queueInfo.queueCount = 1;
				queueInfo.pQueuePriorities = &defaultQueuePriority;
				queueCreateInfos.push_back(queueInfo);
			}
			else
			{
				queueFamilyIndices.graphics = 0;
			}

			// Dedicated compute queue
			if (requestedQueueTypes & VK_QUEUE_COMPUTE_BIT)
			{
				queueFamilyIndices.compute = getQueueFamilyIndex(VK_QUEUE_COMPUTE_BIT);
				if (queueFamilyIndices.compute != queueFamilyIndices.graphics)
				{
					// If compute family index differs, we need an additional queue create info for the compute queue
					VkDeviceQueueCreateInfo queueInfo{};
					queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
					queueInfo.queueFamilyIndex = queueFamilyIndices.compute;
					queueInfo.queueCount = 1;
					queueInfo.pQueuePriorities = &defaultQueuePriority;
					queueCreateInfos.push_back(queueInfo);
				}
			}
			else
			{
				// Else we use the same queue
				queueFamilyIndices.compute = queueFamilyIndices.graphics;
			}

			// Dedicated transfer queue
			if (requestedQueueTypes & VK_QUEUE_TRANSFER_BIT)
			{
				queueFamilyIndices.transfer = getQueueFamilyIndex(VK_QUEUE_TRANSFER_BIT);
				if ((queueFamilyIndices.transfer != queueFamilyIndices.graphics) && (queueFamilyIndices.transfer != queueFamilyIndices.compute))
				{
					// If compute family index differs, we need an additional queue create info for the compute queue
					VkDeviceQueueCreateInfo queueInfo{};
					queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
					queueInfo.queueFamilyIndex = queueFamilyIndices.transfer;
					queueInfo.queueCount = 1;
					queueInfo.pQueuePriorities = &defaultQueuePriority;
					queueCreateInfos.push_back(queueInfo);
				}
			}
			else
			{
				// Else we use the same queue
				queueFamilyIndices.transfer = queueFamilyIndices.graphics;
			}

			// Create the logical device representation
			std::vector<const char*> deviceExtensions(enabledExtensions);
			if (useSwapChain)
			{
				// If the device will be used for presenting to a display via a swapchain we need to request the swapchain extension
				deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
			}

			VkDeviceCreateInfo deviceCreateInfo = {};
			deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());;
			deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
			deviceCreateInfo.pEnabledFeatures = &enabledFeatures;
		
			// If a pNext(Chain) has been passed, we need to add it to the device creation info
			if (pNextChain) {
				VkPhysicalDeviceFeatures2 physicalDeviceFeatures2{};
				physicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
				physicalDeviceFeatures2.features = enabledFeatures;
				physicalDeviceFeatures2.pNext = pNextChain;
				deviceCreateInfo.pEnabledFeatures = nullptr;
				deviceCreateInfo.pNext = &physicalDeviceFeatures2;
			}

			// Enable the debug marker extension if it is present (likely meaning a debugging tool is present)
			if (extensionSupported(VK_EXT_DEBUG_MARKER_EXTENSION_NAME))
			{
				deviceExtensions.push_back(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
				enableDebugMarkers = true;
			}

			if (deviceExtensions.size() > 0)
			{
				deviceCreateInfo.enabledExtensionCount = (uint32_t)deviceExtensions.size();
				deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
			}

			VkResult result = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &logicalDevice);

			if (result == VK_SUCCESS)
			{
				// Create a default command pool for graphics command buffers
				 createCommandPool(queueFamilyIndices.graphics);
			}

			this->enabledFeatures = enabledFeatures;

			return result;
		}

		VulkanBuffer *CreateStagingBuffer(VkDeviceSize size, void *data = nullptr)
		{
			VulkanBuffer * buffer = new VulkanBuffer;
			VkResult res = buffer->createBuffer(logicalDevice, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &memoryProperties, size, data);
			if (res)
			{
				delete buffer;
				return nullptr;
			}
			m_stagingBuffers.push_back(buffer);
			return buffer;			
		}

		void DestroyStagingBuffer(VulkanBuffer* buffer)
		{
			std::vector<VulkanBuffer*>::iterator it;
			it = find(m_stagingBuffers.begin(), m_stagingBuffers.end(), buffer);
			if (it != m_stagingBuffers.end())
			{
				delete buffer;
				m_stagingBuffers.erase(it);
				return;
			}
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
		VulkanBuffer* GetBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceSize size, void *data = nullptr)
		{
			VulkanBuffer * buffer = new VulkanBuffer;
			VkResult res = buffer->createBuffer(logicalDevice, usageFlags, memoryPropertyFlags, &memoryProperties, size, data);
			if(res)
			{
				delete buffer;
				return nullptr;
			}

			m_buffers.push_back(buffer);

			return buffer;
		}

		VulkanBuffer *GetUniformBuffer(VkDeviceSize size, bool frequentUpdate = true, VkQueue queue = VK_NULL_HANDLE, void *data = nullptr)
		{
			VulkanBuffer* buffer = nullptr;
				
			if (frequentUpdate)
			{
				buffer = GetBuffer(
					VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
					size);
			}
			else
			{
				assert(queue);
				assert(data);

				buffer = GetBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
					size);
				VulkanBuffer* staging = CreateStagingBuffer(size, data);
				VkBufferCopy bufferCopy{};
				bufferCopy.size = size;
				VkCommandBuffer copyCmd = createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
				vkCmdCopyBuffer(copyCmd, staging->m_buffer, buffer->m_buffer, 1, &bufferCopy);
				flushCommandBuffer(copyCmd, queue, true);
				
			}
			return buffer;		
		}

		VulkanBuffer *GetGeometryBuffer(VkBufferUsageFlags usageFlags, VkQueue queue, VkDeviceSize size, void* data = nullptr, VkMemoryPropertyFlags memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
		{		
			VulkanBuffer *outBuffer = GetBuffer(usageFlags, memoryPropertyFlags, size);

			if (data)
			{
				if ((usageFlags & VK_BUFFER_USAGE_TRANSFER_DST_BIT) && (memoryPropertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
				{
					VulkanBuffer* stagingBuffer = CreateStagingBuffer(size, data);
					VkCommandBuffer copyCmd = createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
					copyBuffer(stagingBuffer, outBuffer, copyCmd);
					flushCommandBuffer(copyCmd, queue);
					DestroyStagingBuffer(stagingBuffer);
				}
				else
				if ( (usageFlags & VK_BUFFER_USAGE_TRANSFER_SRC_BIT) && (memoryPropertyFlags & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) )
				{
					//TODO should we need a staging buffer
					VK_CHECK_RESULT(outBuffer->map());
					memcpy(outBuffer->m_mapped, data, size);
				}
			}

			return outBuffer;
		}

		/**
		* Copy buffer data from src to dst using VkCmdCopyBuffer
		* 
		* @param src Pointer to the source buffer to copy from
		* @param dst Pointer to the destination buffer to copy tp
		* @param queue Pointer - no more
		* @param copyRegion (Optional) Pointer to a copy region, if NULL, the whole buffer is copied
		*
		* @note Source and destionation pointers must have the approriate transfer usage flags set (TRANSFER_SRC / TRANSFER_DST)
		*/
		void copyBuffer(VulkanBuffer* src, VulkanBuffer* dst, VkCommandBuffer copyCmd, VkBufferCopy* copyRegion = nullptr)
		{
			assert(dst->m_size <= src->m_size);
			assert(src->m_buffer);
			//VkCommandBuffer copyCmd = createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
			VkBufferCopy bufferCopy{};
			if (copyRegion == nullptr)
			{
				bufferCopy.size = src->m_size;
			}
			else
			{
				bufferCopy = *copyRegion;
			}

			vkCmdCopyBuffer(copyCmd, src->m_buffer, dst->m_buffer, 1, &bufferCopy);
		}

		void DestroyBuffer(VulkanBuffer* buffer)
		{
			std::vector<VulkanBuffer*>::iterator it;
			it = find(m_buffers.begin(), m_buffers.end(), buffer);
			if (it != m_buffers.end())
			{
				delete buffer;
				m_buffers.erase(it);
				return;
			}
		}

		void CreateDescriptorSetsPool(std::vector<VkDescriptorPoolSize> poolSizes, uint32_t maxSets)
		{
			VkDescriptorPoolCreateInfo descriptorPoolInfo =
				engine::initializers::descriptorPoolCreateInfo(
					static_cast<uint32_t>(poolSizes.size()),
					poolSizes.data(),
					maxSets);

			VK_CHECK_RESULT(vkCreateDescriptorPool(logicalDevice, &descriptorPoolInfo, nullptr, &m_descriptorPool));
		}

		VulkanTexture *GetTexture(std::string filename, VkFormat format, VkQueue copyQueue,
			VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			VulkanTexture *tex = new VulkanTexture;
			Texture2DData data;
			data.LoadFromFile(filename, format);
			data.CreateStagingBuffer(logicalDevice, &memoryProperties);
			
			tex->Create(logicalDevice, &memoryProperties, { data.m_width, data.m_height, 1 }, data.m_format, imageUsageFlags,
				imageLayout,
				data.m_mips_no);

			VkCommandBuffer copyCmd = createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
			tex->Update(&data, copyCmd, copyQueue);
			flushCommandBuffer(copyCmd, copyQueue);

			tex->CreateDescriptor(VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT, enabledFeatures.samplerAnisotropy ? properties.limits.maxSamplerAnisotropy : 1.0f);

			data.Destroy(logicalDevice);

			m_textures.push_back(tex);

			return tex;
		}

		VulkanTexture* GetTextureArray(std::vector<std::string> filenames, VkFormat format, VkQueue copyQueue,
			VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			VulkanTexture* tex = new VulkanTexture;
			Texture2DData data;
			data.LoadFromFiles(filenames, format);
			data.CreateStagingBuffer(logicalDevice, &memoryProperties);

			tex->Create(logicalDevice, &memoryProperties, { data.m_width, data.m_height, 1 }, data.m_format, imageUsageFlags,
				imageLayout,
				data.m_mips_no,data.m_layers_no);

			VkCommandBuffer copyCmd = createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
			tex->Update(&data, copyCmd, copyQueue);
			flushCommandBuffer(copyCmd, copyQueue);

			tex->CreateDescriptor(VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_IMAGE_VIEW_TYPE_2D_ARRAY, VK_IMAGE_ASPECT_COLOR_BIT, enabledFeatures.samplerAnisotropy ? properties.limits.maxSamplerAnisotropy : 1.0f);

			data.Destroy(logicalDevice);

			m_textures.push_back(tex);

			return tex;
		}

		VulkanTexture *GetTextureCubeMap(std::string filename, VkFormat format, VkQueue copyQueue, VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			VulkanTexture *tex = new VulkanTexture;
			TextureCubeMapData data;
			data.LoadFromFile(filename, format);
			data.CreateStagingBuffer(logicalDevice, &memoryProperties);

			VkFormatProperties formatProperties;
			vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProperties);

			tex->Create(logicalDevice, &memoryProperties, { data.m_width, data.m_height, 1 }, data.m_format, imageUsageFlags,
				imageLayout,
				data.m_mips_no, data.m_layers_no, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT);

			VkCommandBuffer copyCmd = createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
			tex->Update(&data, copyCmd, copyQueue);
			flushCommandBuffer(copyCmd, copyQueue);

			tex->CreateDescriptor(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_IMAGE_VIEW_TYPE_CUBE, VK_IMAGE_ASPECT_COLOR_BIT, enabledFeatures.samplerAnisotropy ? properties.limits.maxSamplerAnisotropy : 1.0f);

			data.Destroy(logicalDevice);

			m_textures.push_back(tex);

			return tex;
		}

		VulkanTexture* GetTextureCubeMap(uint32_t dimension, VkFormat format, VkQueue copyQueue, VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			VulkanTexture* tex = new VulkanTexture;

			const uint32_t numMips = static_cast<uint32_t>(floor(log2(dimension))) + 1;

			VkFormatProperties formatProperties;
			vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProperties);

			tex->Create(logicalDevice, &memoryProperties, { dimension, dimension, 1}, format, imageUsageFlags,
				imageLayout,
				numMips, 6, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT);

			tex->CreateDescriptor(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_IMAGE_VIEW_TYPE_CUBE, VK_IMAGE_ASPECT_COLOR_BIT, enabledFeatures.samplerAnisotropy ? properties.limits.maxSamplerAnisotropy : 1.0f);

			m_textures.push_back(tex);

			return tex;
		}

		VulkanTexture* GetTextureStorage(VkExtent3D extent, VkFormat format, VkQueue copyQueue,
			VkImageViewType viewType,
			VkImageLayout imageLayout = VK_IMAGE_LAYOUT_GENERAL,
			uint32_t mipLevelsCount = 1U, uint32_t layersCount = 1U)
		{
			VulkanTexture* tex = new VulkanTexture;

			tex->Create(logicalDevice, &memoryProperties, extent, format, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, imageLayout, mipLevelsCount, layersCount);

			VkCommandBuffer layoutCmd = createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
			tex->m_descriptor.imageLayout = imageLayout;//TODO maybe also shader read only optimal
			tools::setImageLayout(
				layoutCmd, tex->m_vkImage,
				VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED,
				tex->m_descriptor.imageLayout);

			flushCommandBuffer(layoutCmd, copyQueue, true);

			tex->CreateDescriptor(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, viewType, VK_IMAGE_ASPECT_COLOR_BIT, enabledFeatures.samplerAnisotropy ? properties.limits.maxSamplerAnisotropy : 1.0f);

			m_textures.push_back(tex);

			return tex;
		}

		VulkanTexture* GetRenderTarget(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspect, VkImageLayout imageLayout)
		{
			VulkanTexture* tex = new VulkanTexture;
			tex->Create(logicalDevice, &memoryProperties, { width, height, 1 }, format, usage, imageLayout);
			tex->CreateDescriptor(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_IMAGE_VIEW_TYPE_2D, aspect);
			m_textures.push_back(tex);
			return tex;
		}

		VulkanTexture *GetColorRenderTarget(uint32_t width, uint32_t height, VkFormat format)
		{
			return GetRenderTarget(width, height, format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}

		VulkanTexture *GetDepthRenderTarget(uint32_t width, uint32_t height, bool useInShaders, VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT, bool withStencil = true, bool updateLayout = false, VkQueue copyQueue = VK_NULL_HANDLE)
		{
			VkFormat fbDepthFormat;
			VkBool32 validDepthFormat = engine::tools::getSupportedDepthFormat(physicalDevice, &fbDepthFormat);
			assert(validDepthFormat);
			if (!withStencil)
				fbDepthFormat = VK_FORMAT_D32_SFLOAT;

			VkImageUsageFlags usageflags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
			VkImageLayout imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			if (useInShaders)
			{
				usageflags |= VK_IMAGE_USAGE_SAMPLED_BIT;
				imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			}

			VulkanTexture* texture = GetRenderTarget(width, height, fbDepthFormat, usageflags, aspectMask, imageLayout);

			if (updateLayout && copyQueue)
			{
				VkCommandBuffer layoutCmd = createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
				tools::setImageLayout(
					layoutCmd, texture->m_vkImage,
					//aspectMask,
					VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,//TODO why
					VK_IMAGE_LAYOUT_UNDEFINED,
					texture->m_descriptor.imageLayout);

				flushCommandBuffer(layoutCmd, copyQueue, true);
			}
			return texture;
		}

		void UpdateTexturelayout(VulkanTexture * texture, VkQueue copyQueue)
		{
			VkCommandBuffer layoutCmd = createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
			tools::setImageLayout(
				layoutCmd, texture->m_vkImage,
				VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED,
				texture->m_descriptor.imageLayout);

			flushCommandBuffer(layoutCmd, copyQueue, true);
		}

		void DestroyTexture(VulkanTexture *texture)
		{
			std::vector<VulkanTexture*>::iterator it;
			for (it = m_textures.begin(); it != m_textures.end(); ++it)
			{
				if (*it == texture)
				{
					delete texture;
					m_textures.erase(it);
					return;
				}
			}
		}

		VkPipelineCache createPipelineCache()
		{
			VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
			pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
			VK_CHECK_RESULT(vkCreatePipelineCache(logicalDevice, &pipelineCacheCreateInfo, nullptr, &pipelineCache));
			return pipelineCache;
		}

		void destroyPipelineCache()
		{
			vkDestroyPipelineCache(logicalDevice, pipelineCache, nullptr);
		}

		VulkanPipeline* GetPipeline(VkDescriptorSetLayout descriptorSetLayout,
			std::vector<VkVertexInputBindingDescription> vertexInputBindings, std::vector<VkVertexInputAttributeDescription> vertexInputAttributes,
			std::string vertexFile, std::string fragmentFile,
			VkRenderPass renderPass, VkPipelineCache cache,
			bool blendEnable = false,
			VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			uint32_t vertexConstantBlockSize = 0,
			uint32_t *fConstants = nullptr,
			uint32_t attachmentCount = 0,
			const VkPipelineColorBlendAttachmentState* pAttachments = nullptr,
			bool depthBias = false,
			bool depthTestEnable = true,
			bool depthWriteEnable = true,
			uint32_t subpass = 0)
		{
			PipelineProperties properties;
			properties.depthBias = depthBias;
			properties.depthTestEnable = depthTestEnable;
			properties.depthWriteEnable = depthWriteEnable;
			properties.subpass = subpass;
			properties.topology = topology;
			properties.vertexConstantBlockSize = vertexConstantBlockSize;
			properties.fragmentConstants = fConstants;
			properties.attachmentCount = attachmentCount;
			properties.pAttachments = pAttachments;
			properties.blendEnable = blendEnable;

			VulkanPipeline *pipeline = new VulkanPipeline;
			pipeline->Create(logicalDevice, descriptorSetLayout, vertexInputBindings, vertexInputAttributes, vertexFile, fragmentFile, renderPass, cache, properties);
			m_pipelines.push_back(pipeline);
			return pipeline;
		}

		VulkanPipeline* GetPipeline(VkDescriptorSetLayout descriptorSetLayout,
			std::vector<VkVertexInputBindingDescription> vertexInputBindings, std::vector<VkVertexInputAttributeDescription> vertexInputAttributes,
			std::string vertexFile, std::string fragmentFile,
			VkRenderPass renderPass, VkPipelineCache cache,
			PipelineProperties properties)
		{
			VulkanPipeline* pipeline = new VulkanPipeline;
			pipeline->Create(logicalDevice, descriptorSetLayout, vertexInputBindings, vertexInputAttributes, vertexFile, fragmentFile, renderPass, cache, properties);
			m_pipelines.push_back(pipeline);
			return pipeline;
		}

		VulkanPipeline* GetComputePipeline(std::string file, VkDevice device, VkDescriptorSetLayout descriptorSetLayout, VkPipelineCache cache, uint32_t constanBlockSize = 0)
		{
			VulkanPipeline* pipeline = new VulkanPipeline;
			PipelineProperties props;
			props.vertexConstantBlockSize = constanBlockSize;
			pipeline->CreateCompute(file, device, descriptorSetLayout, cache, props);
			m_pipelines.push_back(pipeline);
			return pipeline;
		}

		VulkanDescriptorSet *GetDescriptorSet(std::vector<VkDescriptorBufferInfo*> buffersDescriptors,
			std::vector<VkDescriptorImageInfo*> texturesDescriptors,
			VkDescriptorSetLayout layout, std::vector<VkDescriptorSetLayoutBinding> layoutBindings, uint32_t dynamicAllingment = 0)
		{
			//TODO add a fast function that checks if we already have a descriptor like this
			VulkanDescriptorSet* set = new VulkanDescriptorSet();

			set->AddBufferDescriptors(buffersDescriptors);
			set->SetDynamicAlignment(dynamicAllingment);
			for (auto tex : texturesDescriptors)
				set->AddTextureDescriptor(tex);

			set->SetupDescriptors(logicalDevice, m_descriptorPool, layout, layoutBindings);
			m_descriptorSets.push_back(set);
			return set;
		}

		VulkanDescriptorSetLayout *GetDescriptorSetLayout(std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> layoutbindigs)
		{
			VulkanDescriptorSetLayout *layout = new VulkanDescriptorSetLayout;

			layout->Create(logicalDevice, layoutbindigs);
			m_descriptorSetLayouts.push_back(layout);

			return layout;
		}

		VulkanFrameBuffer *GetFrameBuffer(VkRenderPass renderPass, int32_t width, int32_t height, std::vector<VkImageView> textures, VkClearColorValue  clearColor = { { 0.0f, 0.0f, 0.0f, 0.0f } })
		{
			VulkanFrameBuffer *fb = new VulkanFrameBuffer;
			fb->Create(logicalDevice, renderPass, width, height, textures, clearColor);
			m_frameBuffers.push_back(fb);
			return fb;
		}

		void DestroyFrameBuffer(VulkanFrameBuffer *fb)
		{
			std::vector<VulkanFrameBuffer*>::iterator it;
			it = find(m_frameBuffers.begin(), m_frameBuffers.end(), fb);
			if (it != m_frameBuffers.end())
			{
				delete fb;
				m_frameBuffers.erase(it);
				return;
			}
		}

		VulkanRenderPass *GetRenderPass(std::vector <VulkanTexture*> textures, int pula=0, std::vector<RenderSubpass> subpasses = {})
		{
			std::vector <std::pair<VkFormat, VkImageLayout>> layouts;
			VulkanRenderPass *pass = new VulkanRenderPass;

			for (auto tex : textures)
			{
				layouts.push_back({tex->m_format, tex->m_descriptor.imageLayout});
			}
			pass->Create(logicalDevice, layouts, false, subpasses);
			m_renderPasses.push_back(pass);
			return pass;
		}

		VulkanRenderPass *GetRenderPass(std::vector <std::pair<VkFormat, VkImageLayout>> layouts)
		{
			VulkanRenderPass *pass = new VulkanRenderPass;
			pass->Create(logicalDevice, layouts);
			m_renderPasses.push_back(pass);
			return pass;
		}

		/** 
		* Create a command pool for allocation command buffers from
		* 
		* @param queueFamilyIndex Family index of the queue to create the command pool for
		* @param createFlags (Optional) Command pool creation flags (Defaults to VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT)
		*
		* @note Command buffers allocated from the created pool can only be submitted to a queue with the same family index
		*
		* @return A handle to the created command buffer
		*/
		VkCommandPool createCommandPool(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags createFlags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT)
		{
			VkCommandPoolCreateInfo cmdPoolInfo = {};
			cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			cmdPoolInfo.queueFamilyIndex = queueFamilyIndex;
			cmdPoolInfo.flags = createFlags;
			VkCommandPool cmdPool;
			VK_CHECK_RESULT(vkCreateCommandPool(logicalDevice, &cmdPoolInfo, nullptr, &cmdPool));
			m_commandPools.insert(std::pair<uint32_t, VkCommandPool>(queueFamilyIndex, cmdPool));
			return cmdPool;
		}

		VkCommandPool getCommandPool(uint32_t queueFamilyIndex)
		{
			VkCommandPool cmdPool = VK_NULL_HANDLE;
			std::map<uint32_t, VkCommandPool>::iterator it;
			it = m_commandPools.find(queueFamilyIndex);
			if(it!= m_commandPools.end())
				cmdPool = it->second;
			if (cmdPool != VK_NULL_HANDLE)
			{
				return cmdPool;
			}
			else
			{
				return createCommandPool(queueFamilyIndex);
			}
		}

		void destroyCommandPool(VkCommandPool cmdPool)
		{
			std::map<uint32_t, VkCommandPool>::iterator it;
			for (it = m_commandPools.begin(); it != m_commandPools.end(); ++it)
			{
				if (it->second == cmdPool)
				{
					vkDestroyCommandPool(logicalDevice, cmdPool, nullptr);
					m_commandPools.erase(it);
					return;
				}
			}			
		}

		/**
		* Allocate a command buffer from the command pool
		*
		* @param level Level of the new command buffer (primary or secondary)
		* @param (Optional) begin If true, recording on the new command buffer will be started (vkBeginCommandBuffer) (Defaults to false)
		*
		* @return A handle to the allocated command buffer
		*/
		VkCommandBuffer createCommandBuffer(VkCommandBufferLevel level, bool begin = false)
		{
			VkCommandPool commandPool = getCommandPool(queueFamilyIndices.graphics);

			VkCommandBufferAllocateInfo cmdBufAllocateInfo = engine::initializers::commandBufferAllocateInfo(commandPool, level, 1);

			VkCommandBuffer cmdBuffer;
			VK_CHECK_RESULT(vkAllocateCommandBuffers(logicalDevice, &cmdBufAllocateInfo, &cmdBuffer));

			// If requested, also start recording for the new command buffer
			if (begin)
			{
				VkCommandBufferBeginInfo cmdBufInfo = engine::initializers::commandBufferBeginInfo();
				VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));
			}

			return cmdBuffer;
		}

		std::vector<VkCommandBuffer> createdrawCommandBuffers(uint32_t size, uint32_t queueFamilyIndex)
		{
			VkCommandPool commandPool = getCommandPool(queueFamilyIndex);

			drawCommandBuffersPoolIndex = queueFamilyIndex;

			drawCommandBuffers.resize(size);

			VkCommandBufferAllocateInfo cmdBufAllocateInfo = engine::initializers::commandBufferAllocateInfo(commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, drawCommandBuffers.size());

			VK_CHECK_RESULT(vkAllocateCommandBuffers(logicalDevice, &cmdBufAllocateInfo, drawCommandBuffers.data()));

			return drawCommandBuffers;
		}

		void freeDrawCommandBuffers()
		{
			if (drawCommandBuffers.empty())
				return;
			VkCommandPool commandPool = getCommandPool(drawCommandBuffersPoolIndex);
			vkFreeCommandBuffers(logicalDevice, commandPool, static_cast<uint32_t>(drawCommandBuffers.size()), drawCommandBuffers.data());
			drawCommandBuffers.clear();
		}

		VkCommandBuffer createComputeCommandBuffer(uint32_t queueFamilyIndex)
		{
			VkCommandPool commandPool = getCommandPool(queueFamilyIndex);

			VkCommandBufferAllocateInfo cmdBufAllocateInfo = engine::initializers::commandBufferAllocateInfo(commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);

			VK_CHECK_RESULT(vkAllocateCommandBuffers(logicalDevice, &cmdBufAllocateInfo, &computeCommandBuffer));

			computeCommandBuffersPoolIndex = queueFamilyIndex;

			return computeCommandBuffer;
		}

		void freeComputeCommandBuffer()
		{
			if (computeCommandBuffer == VK_NULL_HANDLE)
				return;
			VkCommandPool commandPool = getCommandPool(computeCommandBuffersPoolIndex);
			vkFreeCommandBuffers(logicalDevice, commandPool, 1, &computeCommandBuffer);
		}

		/**
		* Finish command buffer recording and submit it to a queue
		*
		* @param commandBuffer Command buffer to flush
		* @param queue Queue to submit the command buffer to 
		* @param free (Optional) Free the command buffer once it has been submitted (Defaults to true)
		*
		* @note The queue that the command buffer is submitted to must be from the same family index as the pool it was allocated from
		* @note Uses a fence to ensure command buffer has finished executing
		*/
		void flushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, bool free = true)
		{
			if (commandBuffer == VK_NULL_HANDLE)
			{
				return;
			}

			VkCommandPool commandPool = getCommandPool(queueFamilyIndices.graphics);

			VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));

			VkSubmitInfo submitInfo = engine::initializers::submitInfo();
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &commandBuffer;

			// Create fence to ensure that the command buffer has finished executing
			VkFenceCreateInfo fenceInfo = engine::initializers::fenceCreateInfo(VK_FLAGS_NONE);
			VkFence fence;
			VK_CHECK_RESULT(vkCreateFence(logicalDevice, &fenceInfo, nullptr, &fence));
			
			// Submit to the queue
			VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence));
			// Wait for the fence to signal that command buffer has finished executing
			VK_CHECK_RESULT(vkWaitForFences(logicalDevice, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT));

			vkDestroyFence(logicalDevice, fence, nullptr);

			if (free)
			{
				vkFreeCommandBuffers(logicalDevice, commandPool, 1, &commandBuffer);
			}
		}

		VkSemaphore GetSemaphore()
		{
			VkSemaphore outSemaphore = VK_NULL_HANDLE;
			VkSemaphoreCreateInfo semaphoreCreateInfo = engine::initializers::semaphoreCreateInfo();
			VK_CHECK_RESULT(vkCreateSemaphore(logicalDevice, &semaphoreCreateInfo, nullptr, &outSemaphore));
			m_semaphores.push_back(outSemaphore);
			return outSemaphore;
		}

		void DestroySemaphore(VkSemaphore semaphore)
		{
			std::vector<VkSemaphore>::iterator it;
			it = find(m_semaphores.begin(), m_semaphores.end(), semaphore);
			if (it != m_semaphores.end())
			{
				vkDestroySemaphore(logicalDevice, semaphore, nullptr);
				m_semaphores.erase(it);
				return;
			}
		}

		/**
		* Check if an extension is supported by the (physical device)
		*
		* @param extension Name of the extension to check
		*
		* @return True if the extension is supported (present in the list read at device creation time)
		*/
		bool extensionSupported(std::string extension)
		{
			return (std::find(supportedExtensions.begin(), supportedExtensions.end(), extension) != supportedExtensions.end());
		}

	};
	}
}
