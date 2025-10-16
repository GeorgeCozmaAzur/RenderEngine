#include "VulkanDevice.h"
#include "VulkanTools.h"
#include "VulkanDescriptorPool.h"
#include "VulkanMesh.h"
#include "VulkanCommandBuffer.h"
#include <set>

namespace engine
{
    namespace render
    {
        VulkanDevice::VulkanDevice(VkInstance instance, VkPhysicalDeviceFeatures* wantedFeatures, std::vector<const char*> wantedExtensions, VkSurfaceKHR surface, void* pNextChain)
        {
            _instance = instance;
            GetPhysicalDevice(wantedFeatures, wantedExtensions, surface);
            CreateLogicalDevice({ "VK_LAYER_KHRONOS_validation" }, pNextChain);//TODO DEBUG
        }

        void VulkanDevice::GetPhysicalDevice(VkPhysicalDeviceFeatures* wantedFeatures, std::vector<const char*> wantedExtensions, VkSurfaceKHR surface)
        {
            uint32_t deviceCount = 0;
            vkEnumeratePhysicalDevices(_instance, &deviceCount, nullptr);
            assert(deviceCount > 0);

            std::vector<VkPhysicalDevice> devices(deviceCount);
            vkEnumeratePhysicalDevices(_instance, &deviceCount, devices.data());
            for (const auto& device : devices)
            {
                VkPhysicalDeviceProperties deviceProperties;
                vkGetPhysicalDeviceProperties(device, &deviceProperties);
                VkPhysicalDeviceFeatures deviceFeatures;
                vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

                uint32_t features_no = sizeof(VkPhysicalDeviceFeatures) / sizeof(VkBool32);
                VkBool32* currentDeviceFeatures = (VkBool32*)(&deviceFeatures);
                VkBool32* currentWantedFeatures = (VkBool32*)(wantedFeatures);
                bool allFeaturesOk = true;
                for (uint32_t i = 0; i < features_no; i++)
                {
                    if (currentWantedFeatures[i] && (currentDeviceFeatures[i] != currentWantedFeatures[i]))
                    {
                        allFeaturesOk = false;
                        break;
                    }
                }
                bool allExtensionsOk = true;
                uint32_t extensionCount;
                vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

                std::vector<VkExtensionProperties> availableExtensions(extensionCount);
                vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

                std::set<const char*> requiredExtensions(wantedExtensions.begin(), wantedExtensions.end());

                for (const auto& extension : availableExtensions) {
                    requiredExtensions.erase(extension.extensionName);
                }

                allExtensionsOk = requiredExtensions.empty();

                if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && allFeaturesOk)
                {
                    uint32_t queueFamilyCount = 0;
                    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

                    m_queueFamilyProperties.resize(queueFamilyCount);
                    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, m_queueFamilyProperties.data());

                    int i = 0;
                    for (const auto& queueFamily : m_queueFamilyProperties) {
                        VkBool32 presentSupport = false;
                        if(surface)
                            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
                        if (presentSupport) {
                            queueFamilyIndices.presentFamily = i;
                            queueFamilyIndices.hasPresentValue = true;
                        }

                        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                            queueFamilyIndices.graphicsFamily = i;
                            queueFamilyIndices.hasGraphivsValue = true;
                        }
                        //TODO compute

                        i++;
                    }

                    queueFamilyIndices.presentFamily = queueFamilyIndices.graphicsFamily;//George - for now it's faster on the gpu to use only one queue

                    if (!queueFamilyIndices.isComplete())
                        continue;

                    vkGetPhysicalDeviceMemoryProperties(device, &memoryProperties);

                    m_properties = deviceProperties;
                    m_enabledFeatures = deviceFeatures;
                    m_enabledExtensions = wantedExtensions;
                    physicalDevice = device;
                    return;
                }
                else
                {
                    continue;
                }
            }

            if (physicalDevice == VK_NULL_HANDLE) {
                throw std::runtime_error("failed to find a suitable GPU!");
            }
        }

        void VulkanDevice::CreateLogicalDevice(const std::vector<const char*> layers, void* pNextChain)
        {
            std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
            std::set<uint32_t> uniqueQueueFamilies = { queueFamilyIndices.graphicsFamily };

            if (queueFamilyIndices.graphicsFamily != queueFamilyIndices.presentFamily)
            {
                uniqueQueueFamilies.emplace(queueFamilyIndices.presentFamily);
            }

            float queuePriority = 1.0f;
            for (uint32_t queueFamily : uniqueQueueFamilies) {
                VkDeviceQueueCreateInfo queueCreateInfo{};
                queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queueCreateInfo.queueFamilyIndex = queueFamily;
                queueCreateInfo.queueCount = 1;
                queueCreateInfo.pQueuePriorities = &queuePriority;
                queueCreateInfos.push_back(queueCreateInfo);
            }

            VkDeviceCreateInfo deviceCreateInfo = {};
            deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());;
            deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
            deviceCreateInfo.pEnabledFeatures = &m_enabledFeatures;

            deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(m_enabledExtensions.size());
            deviceCreateInfo.ppEnabledExtensionNames = m_enabledExtensions.data();

            deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
            deviceCreateInfo.ppEnabledLayerNames = layers.data();

            // If a pNext(Chain) has been passed, we need to add it to the device creation info
            if (pNextChain) {
                VkPhysicalDeviceFeatures2 physicalDeviceFeatures2{};
                physicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
                physicalDeviceFeatures2.features = m_enabledFeatures;
                physicalDeviceFeatures2.pNext = pNextChain;
                deviceCreateInfo.pEnabledFeatures = nullptr;
                deviceCreateInfo.pNext = &physicalDeviceFeatures2;
            }

            if (vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &logicalDevice) != VK_SUCCESS) {
                throw std::runtime_error("failed to create logical device!");
            }
        }

        VkBool32 VulkanDevice::GetSupportedDepthFormat(VkFormat* depthFormat)
        {
            // Since all depth formats may be optional, we need to find a suitable depth format to use
            // Start with the highest precision packed format
            std::vector<VkFormat> depthFormats = {
                VK_FORMAT_D32_SFLOAT_S8_UINT,
                VK_FORMAT_D32_SFLOAT,
                VK_FORMAT_D24_UNORM_S8_UINT,
                VK_FORMAT_D16_UNORM_S8_UINT,
                VK_FORMAT_D16_UNORM
            };

            for (auto& format : depthFormats)
            {
                VkFormatProperties formatProps;
                vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProps);
                // Format must support depth stencil attachment for optimal tiling
                if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
                {
                    *depthFormat = format;
                    return true;
                }
            }

            return false;
        }

        VulkanBuffer* VulkanDevice::CreateStagingBuffer(VkDeviceSize size, void* data)
        {
            VulkanBuffer* buffer = new VulkanBuffer;
            VkResult res = buffer->Create(logicalDevice, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &memoryProperties, size, data);
            if (res)
            {
                delete buffer;
                return nullptr;
            }
            m_stagingBuffers.push_back(buffer);
            return buffer;
        }

        void VulkanDevice::DestroyStagingBuffer(VulkanBuffer* buffer)
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

        VulkanBuffer* VulkanDevice::GetBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkDeviceSize size, void* data)
        {
            VulkanBuffer* buffer = new VulkanBuffer;
            VkResult res = buffer->Create(logicalDevice, usageFlags, memoryPropertyFlags, &memoryProperties, size, data);
            if (res)
            {
                delete buffer;
                return nullptr;
            }

            m_buffers.push_back(buffer);

            return buffer;
        }

        VulkanBuffer* VulkanDevice::GetUniformBuffer(VkDeviceSize size, bool frequentUpdate, VkQueue queue, void* data)
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
                VkCommandBuffer copyCmd = CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
                CopyBuffer(staging, buffer, copyCmd);
                FlushCommandBuffer(copyCmd, queue, true);

            }
            return buffer;
        }

        VulkanBuffer* VulkanDevice::GetGeometryBuffer(VkBufferUsageFlags usageFlags, VkQueue queue, VkDeviceSize size, void* data, VkMemoryPropertyFlags memoryPropertyFlags)
        {
            VulkanBuffer* outBuffer = GetBuffer(usageFlags, memoryPropertyFlags, size);

            if (data)
            {
                if ((usageFlags & VK_BUFFER_USAGE_TRANSFER_DST_BIT) && (memoryPropertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
                {
                    VulkanBuffer* stagingBuffer = CreateStagingBuffer(size, data);
                    VkCommandBuffer copyCmd = CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
                    CopyBuffer(stagingBuffer, outBuffer, copyCmd);
                    FlushCommandBuffer(copyCmd, queue);
                    DestroyStagingBuffer(stagingBuffer);
                }
                else
                    if ((usageFlags & VK_BUFFER_USAGE_TRANSFER_SRC_BIT) && (memoryPropertyFlags & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)))
                    {
                        //TODO should we need a staging buffer
                        VK_CHECK_RESULT(outBuffer->Map());
                        outBuffer->MemCopy(data,size);
                    }
            }

            return outBuffer;
        }
        void VulkanDevice::CopyBuffer(VulkanBuffer* src, VulkanBuffer* dst, VkCommandBuffer copyCmd, VkBufferCopy* copyRegion)
        {
            assert(dst->GetSize() <= src->GetSize());
            assert(src->GetVkBuffer());

            VkBufferCopy bufferCopy{};
            if (copyRegion == nullptr)
            {
                bufferCopy.size = src->GetSize();
            }
            else
            {
                bufferCopy = *copyRegion;
            }

            vkCmdCopyBuffer(copyCmd, src->GetVkBuffer(), dst->GetVkBuffer(), 1, &bufferCopy);
        }

        void VulkanDevice::DestroyBuffer(VulkanBuffer* buffer)
        {
            std::vector<Buffer*>::iterator it;
            it = find(m_buffers.begin(), m_buffers.end(), buffer);
            if (it != m_buffers.end())
            {
                delete buffer;
                m_buffers.erase(it);
                return;
            }
        }

        VulkanTexture* VulkanDevice::GetTexture(TextureData* data, VkQueue copyQueue,
            VkImageUsageFlags imageUsageFlags,
            VkImageLayout imageLayout,
            bool generateMipmaps,
            VkSamplerAddressMode sampleAdressMode)
        {
            VulkanTexture* tex = new VulkanTexture;

            uint32_t mipsNo = generateMipmaps ? (static_cast<uint32_t>(std::floor(std::log2(std::max(data->m_width, data->m_height)))) + 1) : data->m_mips_no;

            tex->Create(logicalDevice, &memoryProperties, { data->m_width, data->m_height, 1 }, ToVkFormat(data->m_format), imageUsageFlags,
                imageLayout,
                VK_IMAGE_ASPECT_COLOR_BIT,
                generateMipmaps ? mipsNo : data->m_mips_no, data->m_layers_no,
                !data->isCubeMap ? 0 : VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT
            );

            if (data->m_ram_data != nullptr)
            {
                VulkanBuffer* stagingBuffer = CreateStagingBuffer(data->m_imageSize, data->m_ram_data);
                VkCommandBuffer copyCmd = CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
                if (!generateMipmaps)
                    tex->Update(data->m_extents, stagingBuffer->GetVkBuffer(), copyCmd, copyQueue);
                else
                    tex->UpdateGeneratingMipmaps(data->m_extents, stagingBuffer->GetVkBuffer(), copyCmd, copyQueue);
                FlushCommandBuffer(copyCmd, copyQueue);
                DestroyStagingBuffer(stagingBuffer);
            }

            tex->CreateDescriptor(sampleAdressMode, 
                !data->isCubeMap ? (data->m_layers_no > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D) : VK_IMAGE_VIEW_TYPE_CUBE,
                m_enabledFeatures.samplerAnisotropy ? m_properties.limits.maxSamplerAnisotropy : 1.0f);

            m_textures.push_back(tex);

            return tex;
        }

        VulkanTexture* VulkanDevice::GetTextureStorage(VkExtent3D extent, VkFormat format, VkQueue copyQueue,
            VkImageViewType viewType,
            VkImageLayout imageLayout,
            uint32_t mipLevelsCount, uint32_t layersCount)
        {
            VulkanTexture* tex = new VulkanTexture;

            tex->Create(logicalDevice, &memoryProperties, extent, format, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, imageLayout, VK_IMAGE_ASPECT_COLOR_BIT, mipLevelsCount, layersCount);

            VkCommandBuffer layoutCmd = CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
            tex->m_descriptor.imageLayout = imageLayout;//TODO maybe also shader read only optimal
            tex->ChangeLayout(layoutCmd,
                VK_IMAGE_LAYOUT_UNDEFINED,
                tex->m_descriptor.imageLayout);

            FlushCommandBuffer(layoutCmd, copyQueue, true);

            tex->CreateDescriptor(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, viewType, m_enabledFeatures.samplerAnisotropy ? m_properties.limits.maxSamplerAnisotropy : 1.0f);

            m_textures.push_back(tex);

            return tex;
        }

        VulkanTexture* VulkanDevice::GetRenderTarget(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspect, VkImageLayout imageLayout)
        {
            VulkanTexture* tex = new VulkanTexture;
            tex->Create(logicalDevice, &memoryProperties, { width, height, 1 }, format, usage, imageLayout, aspect);
            tex->CreateDescriptor(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_IMAGE_VIEW_TYPE_2D);
            m_textures.push_back(tex);
            return tex;
        }

        VulkanTexture* VulkanDevice::GetColorRenderTarget(uint32_t width, uint32_t height, VkFormat format)
        {
            return GetRenderTarget(width, height, format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_IMAGE_ASPECT_COLOR_BIT,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }

        VulkanTexture* VulkanDevice::GetDepthRenderTarget(uint32_t width, uint32_t height, bool useInShaders, VkImageAspectFlags aspectMask, bool withStencil, bool updateLayout, VkQueue copyQueue)
        {
            VkFormat fbDepthFormat;
            VkBool32 validDepthFormat = GetSupportedDepthFormat(&fbDepthFormat);
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
                VkCommandBuffer layoutCmd = CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
                texture->ChangeLayout(layoutCmd,
                    VK_IMAGE_LAYOUT_UNDEFINED,
                    texture->m_descriptor.imageLayout);

                FlushCommandBuffer(layoutCmd, copyQueue, true);
            }
            return texture;
        }

        void VulkanDevice::DestroyTexture(VulkanTexture* texture)
        {
            std::vector<Texture*>::iterator it;
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

        VkPipelineCache VulkanDevice::CreatePipelineCache()
        {
            VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
            pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
            VK_CHECK_RESULT(vkCreatePipelineCache(logicalDevice, &pipelineCacheCreateInfo, nullptr, &pipelineCache));
            return pipelineCache;
        }

        void VulkanDevice::DestroyPipelineCache()
        {
            vkDestroyPipelineCache(logicalDevice, pipelineCache, nullptr);
        }

       /* VulkanPipeline* VulkanDevice::GetPipeline(VkDescriptorSetLayout descriptorSetLayout,
            std::vector<VkVertexInputBindingDescription> vertexInputBindings, std::vector<VkVertexInputAttributeDescription> vertexInputAttributes,
            std::string vertexFile, std::string fragmentFile,
            VkRenderPass renderPass, VkPipelineCache cache,
            bool blendEnable,
            VkPrimitiveTopology topology,
            uint32_t vertexConstantBlockSize,
            uint32_t* fConstants,
            uint32_t attachmentCount,
            const VkPipelineColorBlendAttachmentState* pAttachments,
            bool depthBias,
            bool depthTestEnable,
            bool depthWriteEnable,
            uint32_t subpass)
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

            VulkanPipeline* pipeline = new VulkanPipeline;
            pipeline->Create(logicalDevice, descriptorSetLayout, vertexInputBindings, vertexInputAttributes, vertexFile, fragmentFile, renderPass, cache, properties);
            m_pipelines.push_back(pipeline);
            return pipeline;
        }*/

        VulkanPipeline* VulkanDevice::GetPipeline(VkDescriptorSetLayout descriptorSetLayout,
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

        VulkanPipeline* VulkanDevice::GetComputePipeline(std::string file, VkDevice device, VkDescriptorSetLayout descriptorSetLayout, VkPipelineCache cache, uint32_t constanBlockSize)
        {
            VulkanPipeline* pipeline = new VulkanPipeline;
            PipelineProperties props;
            props.vertexConstantBlockSize = constanBlockSize;
            pipeline->CreateCompute(file, device, descriptorSetLayout, cache, props);
            m_pipelines.push_back(pipeline);
            return pipeline;
        }

        VkDescriptorPool VulkanDevice::CreateDescriptorSetsPool(std::vector<VkDescriptorPoolSize> poolSizes, uint32_t maxSets)
        {
            VkDescriptorPoolCreateInfo descriptorPoolInfo{};
            descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
            descriptorPoolInfo.pPoolSizes = poolSizes.data();
            descriptorPoolInfo.maxSets = maxSets;

            VkDescriptorPool pool;

            VulkanDescriptorPool* dpool = new VulkanDescriptorPool({}, maxSets);//TODO  a lot

            VK_CHECK_RESULT(vkCreateDescriptorPool(logicalDevice, &descriptorPoolInfo, nullptr, &pool));

            dpool->m_vkPool = pool;
            dpool->_device = logicalDevice;

            m_descriptorPools.push_back(dpool);

            return pool;
        }

        VulkanDescriptorSetLayout* VulkanDevice::GetDescriptorSetLayout(std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> layoutbindigs)
        {
            VulkanDescriptorSetLayout* layout = new VulkanDescriptorSetLayout;

            layout->Create(logicalDevice, layoutbindigs);
            m_descriptorSetLayouts.push_back(layout);

            return layout;
        }

        VulkanDescriptorSet* VulkanDevice::GetDescriptorSet(VkDescriptorPool pool, std::vector<VkDescriptorBufferInfo*> buffersDescriptors,
            std::vector<VkDescriptorImageInfo*> texturesDescriptors,
            VkDescriptorSetLayout layout, std::vector<VkDescriptorSetLayoutBinding> layoutBindings, uint32_t dynamicAllingment)
        {
            //TODO add a fast function that checks if we already have a descriptor like this
            VulkanDescriptorSet* set = new VulkanDescriptorSet();

            set->AddBufferDescriptors(buffersDescriptors);
            set->SetDynamicAlignment(dynamicAllingment);
            for (auto tex : texturesDescriptors)
                set->AddTextureDescriptor(tex);

            set->SetupDescriptors(logicalDevice, pool, layout, layoutBindings);
            m_descriptorSets.push_back(set);
            return set;
        }

        VulkanFrameBuffer* VulkanDevice::GetFrameBuffer(VkRenderPass renderPass, int32_t width, int32_t height, std::vector<VkImageView> textures, VkClearColorValue  clearColor)
        {
            VulkanFrameBuffer* fb = new VulkanFrameBuffer;
            fb->Create(logicalDevice, renderPass, width, height, textures, clearColor);
            m_frameBuffers.push_back(fb);
            return fb;
        }

        void VulkanDevice::DestroyFrameBuffer(VulkanFrameBuffer* fb)
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

        VulkanRenderPass* VulkanDevice::GetRenderPass(std::vector <VulkanTexture*> textures, std::vector<RenderSubpass> subpasses)
        {
            std::vector <std::pair<VkFormat, VkImageLayout>> layouts;
            VulkanRenderPass* pass = new VulkanRenderPass;

            for (auto tex : textures)
            {
                layouts.push_back({ tex->m_format, tex->m_descriptor.imageLayout });
            }
            pass->Create(logicalDevice, layouts, false, subpasses);
            m_renderPasses.push_back(pass);
            return pass;
        }

        VulkanRenderPass* VulkanDevice::GetRenderPass(std::vector <std::pair<VkFormat, VkImageLayout>> layouts)
        {
            VulkanRenderPass* pass = new VulkanRenderPass;
            pass->Create(logicalDevice, layouts);
            m_renderPasses.push_back(pass);
            return pass;
        }
        
        VkCommandPool VulkanDevice::CreateCommandPool(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags createFlags)
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

        VkCommandPool VulkanDevice::GetCommandPool(uint32_t queueFamilyIndex)
        {
            VkCommandPool cmdPool = VK_NULL_HANDLE;
            std::map<uint32_t, VkCommandPool>::iterator it;
            it = m_commandPools.find(queueFamilyIndex);
            if (it != m_commandPools.end())
                cmdPool = it->second;
            if (cmdPool != VK_NULL_HANDLE)
            {
                return cmdPool;
            }
            else
            {
                return CreateCommandPool(queueFamilyIndex);
            }
        }

        void VulkanDevice::DestroyCommandPool(VkCommandPool cmdPool)
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

        VkCommandBuffer VulkanDevice::CreateCommandBuffer(VkCommandBufferLevel level, bool begin)
        {
            VkCommandPool commandPool = GetCommandPool(queueFamilyIndices.graphicsFamily);

            VkCommandBufferAllocateInfo cmdBufAllocateInfo { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO , nullptr, commandPool, level, 1 };

            VkCommandBuffer cmdBuffer;
            VK_CHECK_RESULT(vkAllocateCommandBuffers(logicalDevice, &cmdBufAllocateInfo, &cmdBuffer));

            // If requested, also start recording for the new command buffer
            if (begin)
            {
                VkCommandBufferBeginInfo cmdBufInfo{};
                cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));
            }

            return cmdBuffer;
        }

        /*std::vector<VkCommandBuffer> VulkanDevice::CreatedrawCommandBuffers(uint32_t size, uint32_t queueFamilyIndex)
        {
            std::vector<VkCommandBuffer> newCommandBuffers(size);

            for (int i = 0; i < size; i++)
            {
                VulkanCommandBuffer* vkcommandBuffer = new VulkanCommandBuffer();
                vkcommandBuffer->_device = logicalDevice;
                vkcommandBuffer->_commandPool = GetCommandPool(queueFamilyIndex);
                vkcommandBuffer->m_vkCommandBuffer = CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, false);
                m_commandBuffers.push_back(vkcommandBuffer);
                newCommandBuffers[i] = vkcommandBuffer->m_vkCommandBuffer;
            }

            return newCommandBuffers;
        }*/

        void VulkanDevice::FreeDrawCommandBuffers()
        {
            /*if (drawCommandBuffers.empty())
                return;
            VkCommandPool commandPool = GetCommandPool(drawCommandBuffersPoolIndex);
            vkFreeCommandBuffers(logicalDevice, commandPool, static_cast<uint32_t>(drawCommandBuffers.size()), drawCommandBuffers.data());
            drawCommandBuffers.clear();*/
        }

        VkCommandBuffer VulkanDevice::CreateComputeCommandBuffer(uint32_t queueFamilyIndex)
        {
            VkCommandPool commandPool = GetCommandPool(queueFamilyIndex);

            VkCommandBufferAllocateInfo cmdBufAllocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO , nullptr, commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1};// = commandBufferAllocateInfo(commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);

            VK_CHECK_RESULT(vkAllocateCommandBuffers(logicalDevice, &cmdBufAllocateInfo, &computeCommandBuffer));

            computeCommandBuffersPoolIndex = queueFamilyIndex;

            return computeCommandBuffer;
        }

        void VulkanDevice::FreeComputeCommandBuffer()
        {
            if (computeCommandBuffer == VK_NULL_HANDLE)
                return;
            VkCommandPool commandPool = GetCommandPool(computeCommandBuffersPoolIndex);
            vkFreeCommandBuffers(logicalDevice, commandPool, 1, &computeCommandBuffer);
        }

        void VulkanDevice::FlushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, bool free)
        {
            if (commandBuffer == VK_NULL_HANDLE)
            {
                return;
            }

            VkCommandPool commandPool = GetCommandPool(queueFamilyIndices.graphicsFamily);

            VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &commandBuffer;

            // Create fence to ensure that the command buffer has finished executing
            VkFenceCreateInfo fenceCreateInfo{};
            fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceCreateInfo.flags = VK_FLAGS_NONE;
            VkFence fence;
            VK_CHECK_RESULT(vkCreateFence(logicalDevice, &fenceCreateInfo, nullptr, &fence));

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

        void VulkanDevice::DestroyDrawCommandBuffer(render::CommandBuffer* commandBuffer)
        {
            VkCommandPool commandPool = GetCommandPool(queueFamilyIndices.graphicsFamily);
            std::vector<CommandBuffer*>::iterator it;
            it = find(m_commandBuffers.begin(), m_commandBuffers.end(), commandBuffer);
            if (it != m_commandBuffers.end())
            {
                VulkanCommandBuffer* vkvcmd = dynamic_cast<VulkanCommandBuffer*>(commandBuffer);
                vkFreeCommandBuffers(logicalDevice, commandPool, 1, &vkvcmd->m_vkCommandBuffer);
                m_commandBuffers.erase(it);
                return;
            }
        }

        VkSemaphore VulkanDevice::GetSemaphore()
        {
            VkSemaphore outSemaphore = VK_NULL_HANDLE;
            VkSemaphoreCreateInfo semaphoreCreateInfo{};
            semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            VK_CHECK_RESULT(vkCreateSemaphore(logicalDevice, &semaphoreCreateInfo, nullptr, &outSemaphore));
            m_semaphores.push_back(outSemaphore);
            return outSemaphore;
        }

        void VulkanDevice::DestroySemaphore(VkSemaphore semaphore)
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

        VkFence VulkanDevice::GetSignaledFence()
        {
            VkFence outFence = VK_NULL_HANDLE;
            VkFenceCreateInfo fenceInfo{};
            fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
            VK_CHECK_RESULT(vkCreateFence(logicalDevice, &fenceInfo, nullptr, &outFence));
            m_fences.push_back(outFence);
            return outFence;
        }

        void VulkanDevice::DestroyFence(VkFence fence)
        {
            std::vector<VkFence>::iterator it;
            it = find(m_fences.begin(), m_fences.end(), fence);
            if (it != m_fences.end())
            {
                vkDestroyFence(logicalDevice, fence, nullptr);
                m_fences.erase(it);
                return;
            }
        }

        Buffer* VulkanDevice::GetUniformBuffer(size_t size, void* data, DescriptorPool* descriptorPool)
        {
            VulkanBuffer* buffer = GetUniformBuffer(size,true,copyQueue,data);
            buffer->Map();
            //m_buffers.push_back(buffer);
            return buffer;
        }

        Texture* VulkanDevice::GetTexture(TextureData* data, DescriptorPool* descriptorPool, CommandBuffer* commandBuffer)
        {
            VulkanTexture* texture = GetTexture(data, copyQueue);
           // m_textures.push_back(texture);
            return texture;
        }

        Texture* VulkanDevice::GetRenderTarget(uint32_t width, uint32_t height, GfxFormat format, DescriptorPool* srvDescriptorPool, DescriptorPool* rtvDescriptorPool, CommandBuffer* commandBuffer)
        {
            Texture* texture = GetColorRenderTarget(width, height, ToVkFormat(format));
            //m_textures.push_back(texture);
            return texture;
        }

        Texture* VulkanDevice::GetDepthRenderTarget(uint32_t width, uint32_t height, GfxFormat format, DescriptorPool* srvDescriptorPool, DescriptorPool* rtvDescriptorPool, CommandBuffer* commandBuffer, bool useInShaders, bool withStencil)
        {
            Texture* texture = GetDepthRenderTarget(width, height, useInShaders, VK_IMAGE_ASPECT_DEPTH_BIT, withStencil);
           // m_textures.push_back(texture);
            return texture;
        }

        DescriptorPool* VulkanDevice::GetDescriptorPool(std::vector<DescriptorPoolSize> poolSizes, uint32_t maxSets)
        {
            VulkanDescriptorPool* pool = new VulkanDescriptorPool(poolSizes, maxSets);
            pool->Create(logicalDevice);
            m_descriptorPools.push_back(pool);
            return pool;
        }

        VertexLayout* VulkanDevice::GetVertexLayout(std::initializer_list<Component> vComponents, std::initializer_list<Component> iComponents)
        {
            VulkanVertexLayout* vlayout = new VulkanVertexLayout(vComponents, iComponents);
            m_vertexLayouts.push_back(vlayout);
            return vlayout;
        }

        DescriptorSetLayout* VulkanDevice::GetDescriptorSetLayout(std::vector<LayoutBinding> bindings)
        {
            VulkanDescriptorSetLayout* layout = new VulkanDescriptorSetLayout(bindings);
            layout->Create(logicalDevice);
            m_descriptorSetLayouts.push_back(layout);
            return layout;
        }

        DescriptorSet* VulkanDevice::GetDescriptorSet(DescriptorSetLayout* layout, DescriptorPool* pool, std::vector<Buffer*> buffers, std::vector <Texture*> textures)
        {
            VulkanDescriptorSetLayout* vklayout = dynamic_cast<VulkanDescriptorSetLayout*>(layout);
            VulkanDescriptorPool* vkpool = dynamic_cast<VulkanDescriptorPool*>(pool);
            std::vector<VkDescriptorBufferInfo*> buffersDescriptors(buffers.size());
            for (int i = 0; i < buffers.size(); i++)
            {
                VulkanBuffer* vkbuffer = dynamic_cast<VulkanBuffer*>(buffers[i]);
                buffersDescriptors[i] = &vkbuffer->m_descriptor;
            }
            std::vector<VkDescriptorImageInfo*> texturesDescriptors(textures.size());
            for (int i = 0; i < textures.size(); i++)
            {
                VulkanTexture* vktexture = dynamic_cast<VulkanTexture*>(textures[i]);
                texturesDescriptors[i]= &vktexture->m_descriptor;
            }
            VulkanDescriptorSet* set = GetDescriptorSet(vkpool->m_vkPool, buffersDescriptors, texturesDescriptors, vklayout->m_descriptorSetLayout, vklayout->m_setLayoutBindings);
            //m_descriptorSets.push_back(set);
            return set;
        }

        RenderPass* VulkanDevice::GetRenderPass(uint32_t width, uint32_t height, Texture* colorTexture, Texture* depthTexture)
        {
            VulkanTexture* vktexture = dynamic_cast<VulkanTexture*>(colorTexture);
            VulkanTexture* vkdtexture = dynamic_cast<VulkanTexture*>(depthTexture);
            VulkanRenderPass* scenepass = GetRenderPass({ vktexture , vkdtexture }, {});
            VulkanFrameBuffer* fb = GetFrameBuffer(scenepass->GetRenderPass(), width, height, { vktexture->m_descriptor.imageView, vkdtexture->m_descriptor.imageView }, { { 0.0f, 0.0f, 0.0f, 1.0f } });
            scenepass->AddFrameBuffer(fb);
            //m_renderPasses.push_back(scenepass);
            return scenepass;
        }

        Pipeline* VulkanDevice::GetPipeline(std::string vertexFileName, std::string vertexEntry, std::string fragmentFilename, std::string fragmentEntry, VertexLayout* vertexLayout, DescriptorSetLayout* descriptorSetlayout, PipelineProperties properties, RenderPass* renderPass)
        {
            VulkanVertexLayout* vkvlayout = dynamic_cast<VulkanVertexLayout*>(vertexLayout);
            VulkanDescriptorSetLayout* vkdlayout = dynamic_cast<VulkanDescriptorSetLayout*>(descriptorSetlayout);
            VulkanRenderPass* pass = dynamic_cast<VulkanRenderPass*>(renderPass);
            VulkanPipeline* pipeline = GetPipeline(vkdlayout->m_descriptorSetLayout, vkvlayout->m_vertexInputBindings, vkvlayout->m_vertexInputAttributes, 
                vertexFileName, fragmentFilename, pass->GetRenderPass(), pipelineCache, properties);
            //m_pipelines.push_back(pipeline);
            return pipeline;
        }

        CommandBuffer* VulkanDevice::GetCommandBuffer()
        {
            VulkanCommandBuffer* vkcommandBuffer = new VulkanCommandBuffer();
            vkcommandBuffer->_device = logicalDevice;
            vkcommandBuffer->_commandPool = GetCommandPool(queueFamilyIndices.graphicsFamily);
            vkcommandBuffer->m_vkCommandBuffer = CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, false);
            m_commandBuffers.push_back(vkcommandBuffer);
            return vkcommandBuffer;
        }

        Mesh* VulkanDevice::GetMesh(MeshData* data, VertexLayout* vlayout, CommandBuffer* commandBuffer)
        {
            VulkanVertexLayout* vkvlayout = dynamic_cast<VulkanVertexLayout*>(vlayout);
            VulkanMesh* mesh = new VulkanMesh(logicalDevice, vkvlayout->GetVertexInputBinding(VK_VERTEX_INPUT_RATE_VERTEX), vkvlayout->GetVertexInputBinding(VK_VERTEX_INPUT_RATE_INSTANCE));
            mesh->_indexBuffer = GetGeometryBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, copyQueue, data->m_indexCount * sizeof(uint32_t), data->m_indices);
            VkDeviceSize vertexBufferSize = data->m_vertexCount * vlayout->GetVertexSize(0);
            mesh->_vertexBuffer = GetGeometryBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, copyQueue, vertexBufferSize, data->m_vertices);
            mesh->m_indexCount = data->m_indexCount;
            m_meshes.push_back(mesh);
            return mesh;
        }

        VulkanDevice::~VulkanDevice()
        {
            FreeDrawCommandBuffers();
            FreeComputeCommandBuffer();
            for (auto cmd : m_commandBuffers)
                delete cmd;
            m_commandBuffers.clear();
            for (auto cmdPool : m_commandPools)
            {
                vkDestroyCommandPool(logicalDevice, cmdPool.second, nullptr);
            }
            m_commandPools.clear();

            for (auto layout : m_descriptorSetLayouts)
                delete layout;
            m_descriptorSetLayouts.clear();
            for (auto desc : m_descriptorSets)
                delete desc;
            m_descriptorSets.clear();
            for (auto pipeline : m_pipelines)
                delete pipeline;
            m_pipelines.clear();
            for (auto buffer : m_buffers)
                delete buffer;
            m_buffers.clear();
            for (auto buffer : m_stagingBuffers)
                delete buffer;
            m_stagingBuffers.clear();
            for (auto tex : m_textures)
                delete tex;
            m_textures.clear();
            for (auto pass : m_renderPasses)
                delete pass;
            m_renderPasses.clear();
            for (auto fb : m_frameBuffers)
                delete fb;
            m_frameBuffers.clear();
            for (auto sm : m_semaphores)
                vkDestroySemaphore(logicalDevice, sm, nullptr);
            for (auto fence : m_fences)
                vkDestroyFence(logicalDevice, fence, nullptr);

            for (auto pool : m_descriptorPools)
                delete pool;
            m_descriptorPools.clear();
           /* for (auto pool : m_descriptorPools)
            if (pool != VK_NULL_HANDLE)
            {
                vkDestroyDescriptorPool(logicalDevice, pool, nullptr);
            }*/

            if (logicalDevice)
            {
                vkDestroyDevice(logicalDevice, nullptr);
            }
        }
    }
}

