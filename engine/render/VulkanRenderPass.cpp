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

			VkFramebufferCreateInfo framebufferCreateInfo{};
			framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferCreateInfo.renderPass = renderPass;
			framebufferCreateInfo.attachmentCount = (uint32_t)m_attachments.size();
			framebufferCreateInfo.pAttachments = m_attachments.data();
			framebufferCreateInfo.width = width;
			framebufferCreateInfo.height = height;
			framebufferCreateInfo.layers = 1;

			VK_CHECK_RESULT(vkCreateFramebuffer(device, &framebufferCreateInfo, nullptr, &m_vkFrameBuffer));

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
		void VulkanRenderPass::Create(VkDevice device, std::vector <std::pair<VkFormat, VkImageLayout>> layouts, bool bottom_of_pipe, std::vector<RenderSubpass> subpasses)
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
			}

			m_subpasses = subpasses;

			std::vector<VkSubpassDescription> subpassDescriptions;

			std::vector<VkAttachmentReference> allDepthReferences;

			if (m_subpasses.empty())
			{
				std::vector<uint32_t> outputAttachmanets;
				for (uint32_t i = 0; i < m_image_layouts.size(); i++)
				{
					outputAttachmanets.push_back(i);
				}
				m_subpasses.push_back(RenderSubpass({}, outputAttachmanets));
			}

			std::vector<std::vector<VkAttachmentReference>> allInputReferences;
			allInputReferences.resize(m_subpasses.size());
			std::vector<std::vector<VkAttachmentReference>> allColorReferences;
			allColorReferences.resize(m_subpasses.size());

			for (int p = 0;p< m_subpasses.size();p++)
			{
				auto pass = m_subpasses[p];

				VkSubpassDescription subpassDescription = {};
				subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
					
				for (uint32_t attachment : pass.inputAttachmanets)
				{
					VkAttachmentReference ref = { attachment, m_image_layouts[attachment] };
					allInputReferences[p].push_back(ref);
				}

				bool hasDepth = false;
				for (uint32_t attachment : pass.outputAttachmanets)
				{
					VkAttachmentReference ref = { attachment, m_image_layouts[attachment] };

					if (m_image_layouts[attachment] == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
						|| m_image_layouts[attachment] == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
					{
						hasDepth = true;
					}
					else
					{
						ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
						allColorReferences[p].push_back(ref);
					}
				}
				subpassDescription.inputAttachmentCount = (uint32_t)allInputReferences[p].size();
				subpassDescription.pInputAttachments = allInputReferences[p].data();
				subpassDescription.colorAttachmentCount = (uint32_t)allColorReferences[p].size();
				subpassDescription.pColorAttachments = allColorReferences[p].data();

				if(hasDepth && depthReference.layout != VK_IMAGE_LAYOUT_UNDEFINED)
					subpassDescription.pDepthStencilAttachment = &depthReference;
				subpassDescriptions.push_back(subpassDescription);
			}
			

			// Use subpass dependencies for layout transitions
			std::vector<VkSubpassDependency> dependencies;

			dependencies.resize(m_subpasses.size() + 1);
			dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[0].dstSubpass = 0;
			dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			int passIndex = 1;
			for (passIndex = 1; passIndex < m_subpasses.size();passIndex++)
			{
				dependencies[passIndex].srcSubpass = passIndex-1;
				dependencies[passIndex].dstSubpass = passIndex;
				dependencies[passIndex].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				dependencies[passIndex].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
				dependencies[passIndex].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				dependencies[passIndex].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				dependencies[passIndex].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
			}

			dependencies[passIndex].srcSubpass = passIndex - 1;
			dependencies[passIndex].dstSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[passIndex].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependencies[passIndex].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			dependencies[passIndex].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dependencies[passIndex].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			dependencies[passIndex].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			VkRenderPassCreateInfo renderPassCreateInfo{};
			renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassCreateInfo.attachmentCount = (uint32_t)attchmentDescriptions.size();
			renderPassCreateInfo.pAttachments = attchmentDescriptions.data();
			renderPassCreateInfo.subpassCount = (uint32_t)subpassDescriptions.size();
			renderPassCreateInfo.pSubpasses = subpassDescriptions.data();
			renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
			renderPassCreateInfo.pDependencies = dependencies.data();

			VK_CHECK_RESULT(vkCreateRenderPass(_device, &renderPassCreateInfo, nullptr, &m_vkRenderPass));

			m_renderPassBeginInfo = VkRenderPassBeginInfo{};
			m_renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			m_renderPassBeginInfo.renderPass = m_vkRenderPass;
			m_renderPassBeginInfo.framebuffer = VK_NULL_HANDLE;
			m_renderPassBeginInfo.renderArea.extent.width = 0;
			m_renderPassBeginInfo.renderArea.extent.height = 0;
			m_currentClearValues.resize(m_image_layouts.size());
			m_renderPassBeginInfo.clearValueCount = (uint32_t)m_currentClearValues.size();
			m_renderPassBeginInfo.pClearValues = m_currentClearValues.data();
		}

		void VulkanRenderPass::AddFrameBuffer(VulkanFrameBuffer* fb)
		{
			//TODO verify fb compatibility with render pass
			m_frameBuffers.push_back(fb);

			//TODO the clears should be per framebuffer not per render pass and test to see if we can have framebuffers with less textures than render pass image layouts
			VkClearColorValue           color = fb->m_clearColor;//{ { 0.95f, 0.95f, 0.95f, 1.0f } };
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
		}

		void VulkanRenderPass::Begin(VkCommandBuffer commandBuffer, int fbIndex, VkSubpassContents pass_constants)
		{
			if (m_currentFrameBuffer != m_frameBuffers[fbIndex])
			{
				m_renderPassBeginInfo.framebuffer = m_frameBuffers[fbIndex]->m_vkFrameBuffer;
				m_renderPassBeginInfo.renderArea.extent.width = m_frameBuffers[fbIndex]->m_width;
				m_renderPassBeginInfo.renderArea.extent.height = m_frameBuffers[fbIndex]->m_height;
				m_currentFrameBuffer = m_frameBuffers[fbIndex];
			}

			vkCmdBeginRenderPass(commandBuffer, &m_renderPassBeginInfo, pass_constants);

			if (pass_constants == VK_SUBPASS_CONTENTS_INLINE)//vulkan doesn't allow setting viewport and scrissors here when using secondary buffers. They must be set per secondary command buffers
			{
				VkViewport viewport = engine::initializers::viewport((float)m_frameBuffers[fbIndex]->m_width, (float)m_frameBuffers[fbIndex]->m_height, 0.0f, 1.0f);
				vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

				VkRect2D scissor = engine::initializers::rect2D(m_frameBuffers[fbIndex]->m_width, m_frameBuffers[fbIndex]->m_height, 0, 0);
				vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
			}
		}

		void VulkanRenderPass::End(VkCommandBuffer commandBuffer)
		{
			vkCmdEndRenderPass(commandBuffer);
		}

		void VulkanRenderPass::SetClearColor(VkClearColorValue value, int attachment)
		{
			m_currentClearValues[attachment].color = value;
		}
	}
}
