/*
* UI overlay class using ImGui
*
* Copyright (C) 2017 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include "UIOverlay.h"
#include "render/VulkanTexture.h"

namespace engine 
{
	namespace scene
	{
		uint32_t touint(int32_t val)
		{
			if (val > 0)
				return static_cast<uint32_t>(val);
			else
				return 0;
		}
		UIOverlay::UIOverlay()
		{
#if defined(__ANDROID__)		
			if (engine::android::screenDensity >= ACONFIGURATION_DENSITY_XXHIGH) {
				scale = 3.5f;
			}
			else if (engine::android::screenDensity >= ACONFIGURATION_DENSITY_XHIGH) {
				scale = 2.5f;
			}
			else if (engine::android::screenDensity >= ACONFIGURATION_DENSITY_HIGH) {
				scale = 2.0f;
			};
#endif

			// Init ImGui
			ImGui::CreateContext();
			// Color scheme
			ImGuiStyle& style = ImGui::GetStyle();
			style.Colors[ImGuiCol_TitleBg] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
			style.Colors[ImGuiCol_TitleBgActive] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
			style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.0f, 0.0f, 0.0f, 0.1f);
			style.Colors[ImGuiCol_MenuBarBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
			style.Colors[ImGuiCol_Header] = ImVec4(0.8f, 0.0f, 0.0f, 0.4f);
			style.Colors[ImGuiCol_HeaderActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
			style.Colors[ImGuiCol_HeaderHovered] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
			style.Colors[ImGuiCol_FrameBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.8f);
			style.Colors[ImGuiCol_CheckMark] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
			style.Colors[ImGuiCol_SliderGrab] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
			style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
			style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(1.0f, 1.0f, 1.0f, 0.1f);
			style.Colors[ImGuiCol_FrameBgActive] = ImVec4(1.0f, 1.0f, 1.0f, 0.2f);
			style.Colors[ImGuiCol_Button] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
			style.Colors[ImGuiCol_ButtonHovered] = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
			style.Colors[ImGuiCol_ButtonActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
			// Dimensions
			ImGuiIO& io = ImGui::GetIO();
			io.FontGlobalScale = m_scale;
		}

		UIOverlay::~UIOverlay() { }

		/** Prepare all vulkan resources required to render the UI overlay */
		void UIOverlay::prepareResources()
		{
			ImGuiIO& io = ImGui::GetIO();

			// Create font texture
			unsigned char* fontData;
			int texWidth, texHeight;
#if defined(__ANDROID__)
			float scale = (float)engine::android::screenDensity / (float)ACONFIGURATION_DENSITY_MEDIUM;
			AAsset* asset = AAssetManager_open(androidApp->activity->assetManager, "Roboto-Medium.ttf", AASSET_MODE_STREAMING);
			if (asset) {
				size_t size = AAsset_getLength(asset);
				assert(size > 0);
				char* fontAsset = new char[size];
				AAsset_read(asset, fontAsset, size);
				AAsset_close(asset);
				io.Fonts->AddFontFromMemoryTTF(fontAsset, size, 14.0f * scale);
				delete[] fontAsset;
			}
#else
			io.Fonts->AddFontFromFileTTF("./../data/Roboto-Medium.ttf", 16.0f);
#endif		
			io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);
			VkDeviceSize uploadSize = texWidth * texHeight * 4 * sizeof(char);

			m_fontTexture = new render::VulkanTexture;

			m_fontTexture->Create(_device->logicalDevice, &_device->memoryProperties, { touint(texWidth), touint(texHeight), 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_LAYOUT_UNDEFINED);

			// Staging buffers for font data upload
			render::VulkanBuffer* stagingBuffer = _device->CreateStagingBuffer(uploadSize, fontData);;

			// Copy buffer data to font image
			VkCommandBuffer copyCmd = _device->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

			// Prepare for transfer
			m_fontTexture->ChangeLayout(copyCmd, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

			// Copy
			VkBufferImageCopy bufferCopyRegion = {};
			bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			bufferCopyRegion.imageSubresource.layerCount = 1;
			bufferCopyRegion.imageExtent.width = texWidth;
			bufferCopyRegion.imageExtent.height = texHeight;
			bufferCopyRegion.imageExtent.depth = 1;

			vkCmdCopyBufferToImage(
				copyCmd,
				stagingBuffer->GetVkBuffer(),
				m_fontTexture->m_vkImage,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1,
				&bufferCopyRegion
			);

			// Prepare for shader read
			m_fontTexture->ChangeLayout(copyCmd, 
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

			_device->FlushCommandBuffer(copyCmd, _queue, true);

			_device->DestroyStagingBuffer(stagingBuffer);

			m_fontTexture->CreateDescriptor(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_IMAGE_VIEW_TYPE_2D, 0);

			std::vector<VkDescriptorPoolSize> poolSizes = {
			VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}
			};
			descriptorPool = _device->CreateDescriptorSetsPool(poolSizes, 1);

			std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> setLayoutBindings
			{
				{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
			};
			_descriptorLayout = _device->GetDescriptorSetLayout(setLayoutBindings);

			// Descriptor set
			VkDescriptorImageInfo fontDescriptorImageInfo{};
			fontDescriptorImageInfo.sampler = m_fontTexture->m_descriptor.sampler;
			fontDescriptorImageInfo.imageView = m_fontTexture->m_descriptor.imageView;
			fontDescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			render::VulkanDescriptorSet* set = _device->GetDescriptorSet(descriptorPool, {}, { &fontDescriptorImageInfo }, _descriptorLayout->m_descriptorSetLayout, _descriptorLayout->m_setLayoutBindings);
			m_descriptorSets.push_back(set);

			m_geometries.push_back(new Geometry);
		}

		/** Prepare a separate pipeline for the UI overlay rendering decoupled from the main application */
		void UIOverlay::preparePipeline(const VkPipelineCache pipelineCache, const VkRenderPass renderPass)
		{
			render::VertexLayout vertexLayout = render::VertexLayout({
			render::VERTEX_COMPONENT_POSITION,
			render::VERTEX_COMPONENT_UV,
			render::VERTEX_COMPONENT_COLOR,
			render::VERTEX_COMPONENT_NORMAL
				}, {});

			std::vector<VkVertexInputBindingDescription> vertexInputBindings = {
			   	VkVertexInputBindingDescription{0, sizeof(ImDrawVert), VK_VERTEX_INPUT_RATE_VERTEX}
			};
			std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = {
				VkVertexInputAttributeDescription{0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, pos)},	// Location 0: Position
				VkVertexInputAttributeDescription{1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, uv)},	// Location 1: UV
				VkVertexInputAttributeDescription{2, 0, VK_FORMAT_R8G8B8A8_UNORM, offsetof(ImDrawVert, col)},	// Location 0: Color
			};
			_pipeline = _device->GetPipeline(_descriptorLayout->m_descriptorSetLayout, vertexInputBindings, vertexInputAttributes,
				engine::tools::getAssetPath() + "shaders/overlay/uioverlay.vert.spv", engine::tools::getAssetPath() + "shaders/overlay/uioverlay.frag.spv", renderPass, pipelineCache, true, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, sizeof(pushConstBlock));
		}

		bool UIOverlay::shouldRecreateBuffers()
		{
			ImDrawData* imDrawData = ImGui::GetDrawData();
			bool updateCmdBuffers = false;

			if (!imDrawData) { return false; };

			// Note: Alignment is done inside buffer creation
			VkDeviceSize vertexBufferSize = imDrawData->TotalVtxCount * sizeof(ImDrawVert);
			VkDeviceSize indexBufferSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);

			// Update buffers only if vertex or index count has been changed compared to current buffer size
			if ((vertexBufferSize == 0) || (indexBufferSize == 0)) {
				return false;
			}

			// Vertex buffer
			return  (m_geometries[0]->m_vertexCount != imDrawData->TotalVtxCount
				|| m_geometries[0]->m_indexCount < touint(imDrawData->TotalIdxCount));
		}

		/** Update vertex and index buffer containing the imGui elements when required */
		bool UIOverlay::update()
		{
			ImDrawData* imDrawData = ImGui::GetDrawData();
			bool updateCmdBuffers = false;

			if (!imDrawData) { return false; };

			// Note: Alignment is done inside buffer creation
			VkDeviceSize vertexBufferSize = imDrawData->TotalVtxCount * sizeof(ImDrawVert);
			VkDeviceSize indexBufferSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);

			// Update buffers only if vertex or index count has been changed compared to current buffer size
			if ((vertexBufferSize == 0) || (indexBufferSize == 0)) {
				return false;
			}

			// Vertex buffer
			if (m_geometries[0]->m_vertexCount != imDrawData->TotalVtxCount) 
			{
				if (m_geometries[0]->_vertexBuffer)
				{
					m_geometries[0]->_vertexBuffer->Unmap();
					_device->DestroyBuffer(m_geometries[0]->_vertexBuffer);
				}

				m_geometries[0]->_vertexBuffer = _device->GetBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, vertexBufferSize);
				m_geometries[0]->m_vertexCount = imDrawData->TotalVtxCount;
				m_geometries[0]->_vertexBuffer->Unmap();
				m_geometries[0]->_vertexBuffer->Map();
				updateCmdBuffers = true;
			}

			// Index buffer
			VkDeviceSize indexSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);
			if (m_geometries[0]->m_indexCount < touint(imDrawData->TotalIdxCount)) 
			{
				if (m_geometries[0]->_indexBuffer)
				{
					m_geometries[0]->_indexBuffer->Unmap();
					_device->DestroyBuffer(m_geometries[0]->_indexBuffer);
				}

				m_geometries[0]->_indexBuffer = _device->GetBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, indexBufferSize);
				m_geometries[0]->m_indexCount = imDrawData->TotalIdxCount;
				m_geometries[0]->_indexBuffer->Map();
				updateCmdBuffers = true;
			}

			// Upload data
			int vtxOffset = 0;
			int idxOffset = 0;

			for (int n = 0; n < imDrawData->CmdListsCount; n++) 
			{
				const ImDrawList* cmd_list = imDrawData->CmdLists[n];
				m_geometries[0]->_vertexBuffer->MemCopy(cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert), vtxOffset);
				m_geometries[0]->_indexBuffer->MemCopy(cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx), idxOffset);
				vtxOffset += cmd_list->VtxBuffer.Size * sizeof(ImDrawVert);
				idxOffset += cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx);
			}

			// Flush to make writes visible to GPU
			m_geometries[0]->_vertexBuffer->Flush();
			m_geometries[0]->_indexBuffer->Flush();

			return updateCmdBuffers;
		}

		void UIOverlay::draw(const VkCommandBuffer commandBuffer)
		{
			ImDrawData* imDrawData = ImGui::GetDrawData();
			int32_t vertexOffset = 0;
			int32_t indexOffset = 0;

			if ((!imDrawData) || (imDrawData->CmdListsCount == 0)) {
				return;
			}

			ImGuiIO& io = ImGui::GetIO();

			_pipeline->Draw(commandBuffer);
			pushConstBlock.scale = glm::vec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y);
			pushConstBlock.translate = glm::vec2(-1.0f);
			vkCmdPushConstants(commandBuffer, _pipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstBlock), &pushConstBlock);

			m_descriptorSets[0]->Draw(commandBuffer, _pipeline->getPipelineLayout(), 0);

			VkDeviceSize offsets[1] = { 0 };
			const VkBuffer vertexBuffer = m_geometries[0]->_vertexBuffer->GetVkBuffer();
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, offsets);
			vkCmdBindIndexBuffer(commandBuffer, m_geometries[0]->_indexBuffer->GetVkBuffer(), 0, VK_INDEX_TYPE_UINT16);

			for (int32_t i = 0; i < imDrawData->CmdListsCount; i++)
			{
				const ImDrawList* cmd_list = imDrawData->CmdLists[i];
				for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++)
				{
					const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[j];
					VkRect2D scissorRect;
					scissorRect.offset.x = std::max((int32_t)(pcmd->ClipRect.x), 0);
					scissorRect.offset.y = std::max((int32_t)(pcmd->ClipRect.y), 0);
					scissorRect.extent.width = (uint32_t)(pcmd->ClipRect.z - pcmd->ClipRect.x);
					scissorRect.extent.height = (uint32_t)(pcmd->ClipRect.w - pcmd->ClipRect.y);
					vkCmdSetScissor(commandBuffer, 0, 1, &scissorRect);
					vkCmdDrawIndexed(commandBuffer, pcmd->ElemCount, 1, indexOffset, vertexOffset, 0);
					indexOffset += pcmd->ElemCount;
				}
				vertexOffset += cmd_list->VtxBuffer.Size;
			}
		}

		void UIOverlay::resize(uint32_t width, uint32_t height)
		{
			ImGuiIO& io = ImGui::GetIO();
			io.DisplaySize = ImVec2((float)(width), (float)(height));
		}

		void UIOverlay::freeResources()
		{
			if (ImGui::GetCurrentContext())
				ImGui::DestroyContext();

			if (!_device)
				return;

			delete m_fontTexture;
		}

		bool UIOverlay::header(const char* caption)
		{
			return ImGui::CollapsingHeader(caption, ImGuiTreeNodeFlags_DefaultOpen);
		}

		bool UIOverlay::checkBox(const char* caption, bool* value)
		{
			bool res = ImGui::Checkbox(caption, value);
			if (res) { m_updated = true; };
			return res;
		}

		bool UIOverlay::checkBox(const char* caption, int32_t* value)
		{
			bool val = (*value == 1);
			bool res = ImGui::Checkbox(caption, &val);
			*value = val;
			if (res) { m_updated = true; };
			return res;
		}

		bool UIOverlay::inputFloat(const char* caption, float* value, float step)
		{
			bool res = ImGui::InputFloat(caption, value, step, step * 10.0f);
			if (res) { m_updated = true; };
			return res;
		}

		bool UIOverlay::sliderFloat(const char* caption, float* value, float min, float max)
		{
			bool res = ImGui::SliderFloat(caption, value, min, max);
			if (res) { m_updated = true; };
			return res;
		}

		bool UIOverlay::sliderInt(const char* caption, int32_t* value, int32_t min, int32_t max)
		{
			bool res = ImGui::SliderInt(caption, value, min, max);
			if (res) { m_updated = true; };
			return res;
		}

		bool UIOverlay::comboBox(const char* caption, int32_t* itemindex, std::vector<std::string> items)
		{
			if (items.empty()) {
				return false;
			}
			std::vector<const char*> charitems;
			charitems.reserve(items.size());
			for (size_t i = 0; i < items.size(); i++) {
				charitems.push_back(items[i].c_str());
			}
			uint32_t itemCount = static_cast<uint32_t>(charitems.size());
			bool res = ImGui::Combo(caption, itemindex, &charitems[0], itemCount, itemCount);
			if (res) { m_updated = true; };
			return res;
		}

		bool UIOverlay::button(const char* caption)
		{
			bool res = ImGui::Button(caption);
			if (res) { m_updated = true; };
			return res;
		}

		void UIOverlay::text(const char* formatstr, ...)
		{
			va_list args;
			va_start(args, formatstr);
			ImGui::TextV(formatstr, args);
			va_end(args);
		}
	}
}