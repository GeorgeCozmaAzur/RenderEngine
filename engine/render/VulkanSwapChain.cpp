#include "VulkanSwapChain.h"
#include <stdexcept>

namespace engine
{
    namespace render
    {
        void VulkanSwapChain::InitDevices(VkPhysicalDevice physicalDevice, VkDevice device)
        {
        }

        void VulkanSwapChain::InitSurface(VkInstance instance, void* platformHandle, void* platformWindow)
        {
            _instance = instance;

            VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
            surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
            surfaceCreateInfo.hinstance = (HINSTANCE)platformHandle;
            surfaceCreateInfo.hwnd = (HWND)platformWindow;
            VkResult err = VK_SUCCESS;
            err = vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfo, nullptr, &surface);
            if (err != VK_SUCCESS)
                throw std::runtime_error("failed to create surface!");
        }

        void VulkanSwapChain::Create(VkPhysicalDevice physicalDevice, VkDevice device, uint32_t* width, uint32_t* height, uint32_t graphicsQueueIndex, uint32_t presentQueueIndex, bool vsync)
        {
            _physicalDevice = physicalDevice;
            _logicalDevice = device;

            VkSwapchainKHR oldSwapChain = swapChain;

            VkSurfaceCapabilitiesKHR surfaceCapabilities;
            //capabilities
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_physicalDevice, surface, &surfaceCapabilities);

            //formats and color space
            uint32_t formatCount;
            vkGetPhysicalDeviceSurfaceFormatsKHR(_physicalDevice, surface, &formatCount, nullptr);

            std::vector<VkSurfaceFormatKHR> availableFormats;
            if (formatCount != 0) {
                availableFormats.resize(formatCount);
                vkGetPhysicalDeviceSurfaceFormatsKHR(_physicalDevice, surface, &formatCount, availableFormats.data());
            }
            for (const auto& availableFormat : availableFormats) {
                //if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {//TODO george why not srgb
                if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                    m_surfaceFormat = availableFormat;
                }
            }
            if (m_surfaceFormat.format == VK_FORMAT_UNDEFINED)
            {
                m_surfaceFormat = availableFormats[0];
            }      

            //present modes
            uint32_t presentModeCount;
            vkGetPhysicalDeviceSurfacePresentModesKHR(_physicalDevice, surface, &presentModeCount, nullptr);

            std::vector<VkPresentModeKHR> availablePresentModes;
            if (presentModeCount != 0) {
                availablePresentModes.resize(presentModeCount);
                vkGetPhysicalDeviceSurfacePresentModesKHR(_physicalDevice, surface, &presentModeCount, availablePresentModes.data());
            }

            for (const auto& availablePresentMode : availablePresentModes) {
                if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                    m_presentMode = availablePresentMode;
                }
            }

            uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
            if (surfaceCapabilities.maxImageCount > 0 && imageCount > surfaceCapabilities.maxImageCount) {
                imageCount = surfaceCapabilities.maxImageCount;
            }

            swapChainExtent = {};
            // If width (and height) equals the special value 0xFFFFFFFF, the size of the surface will be set by the swapchain
            if (surfaceCapabilities.currentExtent.width == (uint32_t)-1)
            {
                // If the surface size is undefined, the size is set to
                // the size of the images requested.
                swapChainExtent.width = *width;
                swapChainExtent.height = *height;
            }
            else
            {
                // If the surface size is defined, the swap chain size must match
                swapChainExtent = surfaceCapabilities.currentExtent;
                *width = surfaceCapabilities.currentExtent.width;
                *height = surfaceCapabilities.currentExtent.height;
            }

            // Find a supported composite alpha format (not all devices support alpha opaque)
            VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
            // Simply select the first composite alpha format available
            std::vector<VkCompositeAlphaFlagBitsKHR> compositeAlphaFlags = {
                VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
                VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
                VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
            };
            for (auto& compositeAlphaFlag : compositeAlphaFlags) {
                if (surfaceCapabilities.supportedCompositeAlpha & compositeAlphaFlag) {
                    compositeAlpha = compositeAlphaFlag;
                    break;
                };
            }

            VkImageUsageFlags imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            // Enable transfer source on swap chain images if supported
            if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
                imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
            }

            // Enable transfer destination on swap chain images if supported
            if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
                imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            }

            VkSwapchainCreateInfoKHR createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            createInfo.surface = surface;
            createInfo.minImageCount = imageCount;
            createInfo.imageFormat = m_surfaceFormat.format;
            createInfo.imageColorSpace = m_surfaceFormat.colorSpace;
            createInfo.imageExtent = swapChainExtent;
            createInfo.imageArrayLayers = 1;
            createInfo.imageUsage = imageUsage;

            std::vector<uint32_t> queueFamilyIndices;
            if (graphicsQueueIndex != presentQueueIndex) {
                createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
                queueFamilyIndices = { graphicsQueueIndex , presentQueueIndex };
            }
            else {
                createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            }
            createInfo.queueFamilyIndexCount = queueFamilyIndices.size();
            createInfo.pQueueFamilyIndices = queueFamilyIndices.data();
            createInfo.preTransform = surfaceCapabilities.currentTransform;
            createInfo.compositeAlpha = compositeAlpha;
            createInfo.presentMode = m_presentMode;
            createInfo.clipped = VK_TRUE;
            createInfo.oldSwapchain = oldSwapChain;
            
            if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
                throw std::runtime_error("failed to create swap chain!");
            }

            if (oldSwapChain != VK_NULL_HANDLE)
            {
                for (uint32_t i = 0; i < imageCount; i++)
                {
                    vkDestroyImageView(device, swapChainImageViews[i], nullptr);
                }
                vkDestroySwapchainKHR(device, oldSwapChain, nullptr);
            }

            vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
            m_images.resize(imageCount);
            vkGetSwapchainImagesKHR(device, swapChain, &imageCount, m_images.data());

            swapChainImageViews.resize(m_images.size());
            for (size_t i = 0; i < m_images.size(); i++) {
                VkImageViewCreateInfo createInfo{};
                createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                createInfo.image = m_images[i];
                createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                createInfo.format = m_surfaceFormat.format;
                createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
                createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
                createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
                createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
                createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                createInfo.subresourceRange.baseMipLevel = 0;
                createInfo.subresourceRange.levelCount = 1;
                createInfo.subresourceRange.baseArrayLayer = 0;
                createInfo.subresourceRange.layerCount = 1;
                if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
                    throw std::runtime_error("failed to create image views!");
                }
            }

        }

        VkResult VulkanSwapChain::acquireNextImage(VkSemaphore presentCompleteSemaphore, uint32_t* imageIndex)
        {
            return vkAcquireNextImageKHR(_logicalDevice, swapChain, UINT64_MAX, presentCompleteSemaphore, VK_NULL_HANDLE, imageIndex);
        }

        VkResult VulkanSwapChain::queuePresent(VkQueue queue, uint32_t imageIndex, VkSemaphore waitSemaphore)
        {
            VkPresentInfoKHR presentInfo{};
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

            presentInfo.waitSemaphoreCount = 1;
            presentInfo.pWaitSemaphores = &waitSemaphore;
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = &swapChain;
            presentInfo.pImageIndices = &imageIndex;
            presentInfo.pResults = nullptr; // Optional
            return vkQueuePresentKHR(queue, &presentInfo);
        }

        void VulkanSwapChain::CleanUp()
        {
            for (auto imageView : swapChainImageViews) {
                vkDestroyImageView(_logicalDevice, imageView, nullptr);
            }
            vkDestroySwapchainKHR(_logicalDevice, swapChain, nullptr);
            vkDestroySurfaceKHR(_instance, surface, nullptr);
        }
    }
}