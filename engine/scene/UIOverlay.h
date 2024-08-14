/*
* UI overlay class using ImGui
*
* Copyright (C) 2017 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <vector>
#include <sstream>
#include <iomanip>

#include <vulkan/vulkan.h>
#include "VulkanTools.h"
#include "VulkanDebug.h"
#include "render/VulkanBuffer.h"
#include "render/VulkanDevice.h"
#include "render/VulkanPipeline.h"

#include "../external/imgui/imgui.h"

#if defined(__ANDROID__)
#include "VulkanAndroid.h"
#endif

namespace engine 
{
	namespace scene
	{
		class UIOverlay : public RenderObject
		{
		public:
			render::VulkanDevice* _device = nullptr;
			VkQueue _queue;

			class render::VulkanTexture* m_fontTexture = nullptr;

			struct PushConstBlock {
				glm::vec2 scale;
				glm::vec2 translate;
			} pushConstBlock;

			bool m_visible = true;
			bool m_updated = false;
			float m_scale = 1.0f;

			UIOverlay();
			~UIOverlay();

			void preparePipeline(const VkPipelineCache pipelineCache, const VkRenderPass renderPass);
			void prepareResources();

			bool update();
			void draw(const VkCommandBuffer commandBuffer);
			void resize(uint32_t width, uint32_t height);

			void freeResources();

			bool header(const char* caption);
			bool checkBox(const char* caption, bool* value);
			bool checkBox(const char* caption, int32_t* value);
			bool inputFloat(const char* caption, float* value, float step);
			bool sliderFloat(const char* caption, float* value, float min, float max);
			bool sliderInt(const char* caption, int32_t* value, int32_t min, int32_t max);
			bool comboBox(const char* caption, int32_t* itemindex, std::vector<std::string> items);
			bool button(const char* caption);
			void text(const char* formatstr, ...);
		};
	}
}