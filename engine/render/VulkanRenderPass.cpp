#include "VulkanRenderPass.h"
#include <array>

namespace engine
{
	namespace render
	{
		void VulkanFrameBuffer::Create(VkDevice device, VkRenderPass renderPass, int32_t width, int32_t height, std::vector<VkImageView> imageViews, VkClearColorValue  clearColor)
		{
			_device = device;
			m_width = width;
			m_height = height;
			m_clearColor = clearColor;

			m_attachments = imageViews;

			VkFramebufferCreateInfo fbufCreateInfo = engine::initializers::framebufferCreateInfo();
			fbufCreateInfo.renderPass = renderPass;
			fbufCreateInfo.attachmentCount = (uint32_t)m_attachments.size();
			fbufCreateInfo.pAttachments = m_attachments.data();
			fbufCreateInfo.width = width;
			fbufCreateInfo.height = height;
			fbufCreateInfo.layers = 1;

			VK_CHECK_RESULT(vkCreateFramebuffer(device, &fbufCreateInfo, nullptr, &m_vkFrameBuffer));

		}

		VulkanFrameBuffer::~VulkanFrameBuffer()
		{
			vkDestroyFramebuffer(_device, m_vkFrameBuffer, nullptr);
		}

		void VulkanRenderPass::Destroy()
		{
			vkDestroyRenderPass(_device, m_vkRenderPass, nullptr);
		}

		// Set up a separate render pass for the offscreen frame buffer
			// This is necessary as the offscreen frame buffer attachments use formats different to those from the example render pass
		void VulkanRenderPass::Create(VkDevice device, std::vector <std::pair<VkFormat, VkImageLayout>> layouts, bool bottom_of_pipe)
		{
			_device = device;

			std::vector<VkAttachmentDescription> attchmentDescriptions;

			for (int i = 0;i < layouts.size();i++)
			{
				VkImageLayout img_layout = layouts[i].second;

				VkAttachmentDescription desc;
				desc.flags = 0;
				desc.format = layouts[i].first;
				desc.samples = VK_SAMPLE_COUNT_1_BIT;
				desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;

				if (VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL == img_layout)
					desc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;//TODO should it be VK_ATTACHMENT_STORE_OP_STORE
				else
					desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

				if (VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL == img_layout)
					desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
				else
					desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;

				desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

				desc.finalLayout = img_layout;

				attchmentDescriptions.push_back(desc);

				m_image_layouts.push_back(img_layout);
			}

			std::vector<VkAttachmentReference> color_references;
			VkAttachmentReference depthReference = { 0, VK_IMAGE_LAYOUT_UNDEFINED };

			for (uint32_t i = 0; i < m_image_layouts.size(); i++)
			{
				VkAttachmentReference reference;
				reference.attachment = i;
				if (m_image_layouts[i] == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
					|| m_image_layouts[i] == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
				{
					reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
					depthReference = reference;
				}
				else
				{
					reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
					color_references.push_back(reference);
				}

			}

			VkSubpassDescription subpassDescription = {};
			subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

			subpassDescription.colorAttachmentCount = (uint32_t)color_references.size();
			subpassDescription.pColorAttachments = color_references.data();
			if (depthReference.layout != VK_IMAGE_LAYOUT_UNDEFINED)
				subpassDescription.pDepthStencilAttachment = &depthReference;

			// Use subpass dependencies for layout transitions
			std::array<VkSubpassDependency, 2> dependencies;

			dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[0].dstSubpass = 0;
			if (bottom_of_pipe)
			{
				dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
				dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
				dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			}
			else
			{
				dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
				dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
				dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
				dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			}
			dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			dependencies[1].srcSubpass = 0;
			dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
			if (bottom_of_pipe)
			{
				dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
				dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			}
			else
			{
				dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
				dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
				dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			}
			dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			VkRenderPassCreateInfo renderPassCreateInfo = engine::initializers::renderPassCreateInfo();
			renderPassCreateInfo.attachmentCount = (uint32_t)attchmentDescriptions.size();
			renderPassCreateInfo.pAttachments = attchmentDescriptions.data();
			renderPassCreateInfo.subpassCount = 1;
			renderPassCreateInfo.pSubpasses = &subpassDescription;
			renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
			renderPassCreateInfo.pDependencies = dependencies.data();

			VK_CHECK_RESULT(vkCreateRenderPass(_device, &renderPassCreateInfo, nullptr, &m_vkRenderPass));

			m_renderPassBeginInfo = engine::initializers::renderPassBeginInfo();
			m_renderPassBeginInfo.renderPass = m_vkRenderPass;
			m_renderPassBeginInfo.framebuffer = VK_NULL_HANDLE;
			m_renderPassBeginInfo.renderArea.extent.width = 0;
			m_renderPassBeginInfo.renderArea.extent.height = 0;
			m_currentClearValues.resize(m_image_layouts.size());
			m_renderPassBeginInfo.clearValueCount = (uint32_t)m_currentClearValues.size();
			m_renderPassBeginInfo.pClearValues = m_currentClearValues.data();
		}

		void VulkanRenderPass::AddFrameBuffer(VulkanFrameBuffer* fb)
		{//TODO verify fb compatibility with render pass
			m_frameBuffers.push_back(fb);
		}

		void VulkanRenderPass::Begin(VkCommandBuffer commandBuffer, int fbIndex)
		{
			//TODO the clears should be per framebuffer not per render pass and test to see if we can have framebuffers with less textures than render pass image layouts
			VkClearColorValue           color = m_frameBuffers[fbIndex]->m_clearColor;//{ { 0.0f, 0.0f, 0.0f, 0.0f } };
			VkClearDepthStencilValue    depthStencil = { 1.0f, 0 };

			for (int i = 0;i < m_image_layouts.size();i++)
			{
				VkClearValue value;
				if (m_image_layouts[i] == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
					|| m_image_layouts[i] == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
				{
					value.depthStencil = depthStencil;
				}
				else
				{
					value.color = color;
				}
				m_currentClearValues[i] = value;
			}

			if (m_currentFrameBuffer != m_frameBuffers[fbIndex])
			{
				m_renderPassBeginInfo.framebuffer = m_frameBuffers[fbIndex]->m_vkFrameBuffer;
				m_renderPassBeginInfo.renderArea.extent.width = m_frameBuffers[fbIndex]->m_width;
				m_renderPassBeginInfo.renderArea.extent.height = m_frameBuffers[fbIndex]->m_height;
				m_currentFrameBuffer = m_frameBuffers[fbIndex];
			}

			vkCmdBeginRenderPass(commandBuffer, &m_renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			VkViewport viewport = engine::initializers::viewport((float)m_frameBuffers[fbIndex]->m_width, (float)m_frameBuffers[fbIndex]->m_height, 0.0f, 1.0f);
			vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

			VkRect2D scissor = engine::initializers::rect2D(m_frameBuffers[fbIndex]->m_width, m_frameBuffers[fbIndex]->m_height, 0, 0);
			vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
		}

		void VulkanRenderPass::End(VkCommandBuffer commandBuffer)
		{
			vkCmdEndRenderPass(commandBuffer);
		}
	}
}
