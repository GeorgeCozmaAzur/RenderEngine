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
			uint32_t m_width, m_height;
			std::vector<VkImageView> m_attachments;
			VkClearColorValue m_clearColor;
			VkFramebuffer m_vkFrameBuffer;
			void Create(VkDevice device, VkRenderPass renderPass, uint32_t width, uint32_t height, std::vector<VkImageView> imageViews, VkClearColorValue  clearColor = { { 0.0f, 0.0f, 0.0f, 0.0f } });
			~VulkanFrameBuffer();
		};

		struct RenderSubpass
		{
			RenderSubpass(std::vector<uint32_t> i, std::vector<uint32_t> o) :
				inputAttachmanets(i), outputAttachmanets(o) {}		
			std::vector<uint32_t> inputAttachmanets;
			std::vector<uint32_t> outputAttachmanets;
		};

		class VulkanRenderPass {

			VkDevice _device = VK_NULL_HANDLE;
			std::vector <VkImageLayout> m_image_layouts;
			
			std::vector<VkClearValue> m_currentClearValues;
			VulkanFrameBuffer* m_currentFrameBuffer = nullptr;

			std::vector<RenderSubpass> m_subpasses;
			VkRenderPassBeginInfo m_renderPassBeginInfo;
			VkRenderPass m_vkRenderPass;

		public:
			std::vector<VulkanFrameBuffer*> m_frameBuffers;//george TODO make it private
			~VulkanRenderPass() {
				Destroy();
			}

			void Create(VkDevice device, std::vector <std::pair<VkFormat, VkImageLayout>> layouts, bool bottom_of_pipe = false, std::vector<RenderSubpass> subpasses = {});
			void AddFrameBuffer(VulkanFrameBuffer* fb);
			void ResetFrameBuffers() { m_frameBuffers.clear(); }
			VkRenderPass GetRenderPass() { return m_vkRenderPass; }
			void Begin(VkCommandBuffer command_buffer, int fb_index, VkSubpassContents pass_constants = VK_SUBPASS_CONTENTS_INLINE);
			void End(VkCommandBuffer command_buffer);
			void SetClearColor(VkClearColorValue value, int attachment);
			void Destroy();
		};
	}
}