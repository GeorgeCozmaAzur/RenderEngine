#pragma once
#include <VulkanTools.h>

namespace engine
{
	namespace render
	{
		#define FB_COLOR_FORMAT VK_FORMAT_R8G8B8A8_UNORM
		#define FB_COLOR_HDR_FORMAT VK_FORMAT_R32G32B32A32_SFLOAT

		struct VulkanFrameBuffer
		{
			VkDevice _device = VK_NULL_HANDLE;
			int32_t m_width, m_height;
			std::vector<VkImageView> m_attachments;
			VkClearColorValue m_clearColor;
			VkFramebuffer m_vkFrameBuffer;
			void Create(VkDevice device, VkRenderPass renderPass, int32_t width, int32_t height, std::vector<VkImageView> imageViews, VkClearColorValue  clearColor = { { 0.0f, 0.0f, 0.0f, 0.0f } });
			~VulkanFrameBuffer();
		};

		class VulkanRenderPass {

			VkDevice _device = VK_NULL_HANDLE;
			std::vector <VkImageLayout> m_image_layouts;
			
			std::vector<VkClearValue> m_currentClearValues;
			VulkanFrameBuffer* m_currentFrameBuffer = nullptr;
			VkRenderPassBeginInfo m_renderPassBeginInfo;
			VkRenderPass m_vkRenderPass;

		public:
			std::vector<VulkanFrameBuffer*> m_frameBuffers;//george TODO make it private
			~VulkanRenderPass() {
				Destroy();
			}

			void Create(VkDevice device, std::vector <std::pair<VkFormat, VkImageLayout>> layouts, bool bottom_of_pipe = false);
			void AddFrameBuffer(VulkanFrameBuffer* fb);
			void ResetFrameBuffers() { m_frameBuffers.clear(); }
			VkRenderPass GetRenderPass() { return m_vkRenderPass; }
			void Begin(VkCommandBuffer command_buffer, int fb_index);
			void End(VkCommandBuffer command_buffer);
			void Destroy();
		};
	}
}