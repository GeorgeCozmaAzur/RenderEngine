#pragma once

#include <vector>

namespace engine
{
	namespace render
	{
		enum PrimitiveTopolgy
		{
			TRIANGLE_LIST,
			TRIANGLE_STRIP,
			LINE_LIST
		};

		enum CullMode
		{
			NONE,
			FRONT,
			BACK,
			FRONTBACK
		};

		struct BlendAttachmentState
		{
			bool blend_enable = false;
		};

		struct PipelineProperties
		{
			PrimitiveTopolgy topology = TRIANGLE_LIST;
			uint32_t vertexConstantBlockSize = 0;
			uint32_t* fragmentConstants = nullptr;
			bool blendEnable = false;
			bool depthBias = false;
			bool depthTestEnable = true;
			bool depthWriteEnable = true;
			CullMode cullMode = BACK;
			//VkCullModeFlagBits cullMode = VK_CULL_MODE_BACK_BIT;
			//VkBool32 primitiveRestart = VK_FALSE;
			bool primitiveRestart = false;
			uint32_t attachmentCount = 0;
			const BlendAttachmentState* pAttachments = nullptr;
			//const VkPipelineColorBlendAttachmentState* pAttachments = nullptr;
			uint32_t subpass = 0;
		};

		class Pipeline
		{
			
		};
	}
}