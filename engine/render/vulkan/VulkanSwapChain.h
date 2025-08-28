#pragma once
#include <vector>
#include "vulkan/vulkan.h"

namespace engine
{
    namespace render
    {
        class VulkanSwapChain
        {
        public:
            VkInstance _instance = VK_NULL_HANDLE;
            VkDevice _logicalDevice = VK_NULL_HANDLE;
            VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;

            VkSurfaceKHR surface = VK_NULL_HANDLE;
            const std::vector<const char*> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
            };
            VkSurfaceFormatKHR m_surfaceFormat{};
            VkPresentModeKHR m_presentMode{ VK_PRESENT_MODE_FIFO_KHR };
            VkSwapchainKHR swapChain = VK_NULL_HANDLE;
            std::vector<VkImage> m_images;

            VkExtent2D swapChainExtent{};
            std::vector<VkImageView> swapChainImageViews;

            void InitDevices(VkPhysicalDevice physicalDevice, VkDevice device);

            void InitSurface(VkInstance instance, void* platformHandle, void* platformWindow);

            void Create(VkPhysicalDevice physicalDevice, VkDevice device, uint32_t* width, uint32_t* height, uint32_t graphicsQueueIndex, uint32_t presentQueueIndex, bool vsync = false);

            VkResult acquireNextImage(VkSemaphore presentCompleteSemaphore, uint32_t* imageIndex);

            VkResult queuePresent(VkQueue queue, uint32_t imageIndex, VkSemaphore waitSemaphore = VK_NULL_HANDLE);

            void CleanUp();
        };
    }
}