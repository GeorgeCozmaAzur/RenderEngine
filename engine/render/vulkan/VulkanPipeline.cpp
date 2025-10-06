#include "VulkanPipeline.h"
#include "VulkanCommandBuffer.h"
#include <array>
namespace engine
{
	namespace render
	{
		inline VkPrimitiveTopology ToVkTopology(PrimitiveTopolgy topology) {
			switch (topology) {
			case PrimitiveTopolgy::TRIANGLE_LIST: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			case PrimitiveTopolgy::TRIANGLE_STRIP: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
			case PrimitiveTopolgy::LINE_LIST: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
			default: return VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
			}
		}

		inline VkCullModeFlagBits ToVkCullMode(CullMode cullMode) {
			switch (cullMode) {
			case CullMode::NONE: return VK_CULL_MODE_NONE;
			case CullMode::FRONT: return VK_CULL_MODE_FRONT_BIT;
			case CullMode::BACK: return VK_CULL_MODE_BACK_BIT;
			case CullMode::FRONTBACK: return VK_CULL_MODE_FRONT_AND_BACK;
			default: return VK_CULL_MODE_FLAG_BITS_MAX_ENUM;
			}
		}

		VkPipelineShaderStageCreateInfo VulkanPipeline::LoadShader(std::string fileName, VkShaderStageFlagBits stage)
		{
			VkPipelineShaderStageCreateInfo shaderStage = {};
			shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStage.stage = stage;
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
			shaderStage.module = engine::tools::loadShader(androidApp->activity->assetManager, fileName.c_str(), m_device);
#else
			shaderStage.module = engine::tools::loadShader(fileName.c_str(), _device);
#endif
			shaderStage.pName = "main"; // todo : make param
			assert(shaderStage.module != VK_NULL_HANDLE);
			m_shaderModules.push_back(shaderStage.module);
			return shaderStage;
		}

		VulkanPipeline::~VulkanPipeline()
		{
			vkDestroyPipeline(_device, m_vkPipeline, nullptr);
			vkDestroyPipelineLayout(_device, m_pipelineLayout, nullptr);
		}

		void VulkanPipeline::Create(VkDevice device, VkDescriptorSetLayout descriptorSetLayout,
			std::vector<VkVertexInputBindingDescription> vertexInputBindings,
			std::vector<VkVertexInputAttributeDescription> vertexInputAttributes,
			std::string vertexFile, std::string fragmentFile,
			VkRenderPass renderPass, VkPipelineCache cache,
			PipelineProperties properties
		)
		{
			if (m_vkPipeline != VK_NULL_HANDLE)
				return;

			_device = device;
			_renderPass = renderPass;
			_pipelineCache = cache;

			/*
				[POI] Create a pipeline layout used for our graphics pipeline
			*/
			VkPipelineLayoutCreateInfo pipelineLayoutCI{};
			pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

			if (properties.vertexConstantBlockSize > 0)
			{
				VkPushConstantRange pushConstantRange{};
				pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
				pushConstantRange.offset = 0;
				pushConstantRange.size = properties.vertexConstantBlockSize;						

				pipelineLayoutCI.pushConstantRangeCount = 1;
				pipelineLayoutCI.pPushConstantRanges = &pushConstantRange;
				std::cout << " " << properties.vertexConstantBlockSize;
			}

			// The pipeline layout is based on the descriptor set layout we created above
			pipelineLayoutCI.setLayoutCount = 1;
			pipelineLayoutCI.pSetLayouts = &descriptorSetLayout;
			VK_CHECK_RESULT(vkCreatePipelineLayout(_device, &pipelineLayoutCI, nullptr, &m_pipelineLayout));

			std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

			VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI{};
			inputAssemblyStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			inputAssemblyStateCI.topology = ToVkTopology(properties.topology);
			inputAssemblyStateCI.flags = 0;
			inputAssemblyStateCI.primitiveRestartEnable = properties.primitiveRestart;

			VkPipelineRasterizationStateCreateInfo rasterizationStateCI{};
			rasterizationStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			rasterizationStateCI.polygonMode = VK_POLYGON_MODE_FILL;
			rasterizationStateCI.cullMode = properties.cullMode;
			rasterizationStateCI.frontFace = VK_FRONT_FACE_CLOCKWISE;
			rasterizationStateCI.flags = 0;
			rasterizationStateCI.depthClampEnable = VK_FALSE;
			rasterizationStateCI.lineWidth = 1.0f;

			VkPipelineColorBlendAttachmentState blendAttachmentState{ VK_TRUE,
					VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
					VK_BLEND_OP_ADD,
					VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_FACTOR_ZERO,
					VK_BLEND_OP_ADD,
					0xf };
			blendAttachmentState.blendEnable = VkBool32(properties.blendEnable);

			std::vector<VkPipelineColorBlendAttachmentState> attachments{ blendAttachmentState };

			VkPipelineColorBlendStateCreateInfo colorBlendStateCI{};
			colorBlendStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;

			if (properties.pAttachments)
			{
				attachments.clear();
				for (int i = 0; i < properties.attachmentCount; i++)
				{
					VkPipelineColorBlendAttachmentState newblendAttachmentState = blendAttachmentState;
					newblendAttachmentState.blendEnable = VkBool32(properties.pAttachments[i].blend_enable);
					attachments.push_back(newblendAttachmentState);
				}
			}
			colorBlendStateCI.attachmentCount = attachments.size();
			colorBlendStateCI.pAttachments = attachments.data();

			VkBool32 enableDepthWrite = VkBool32(properties.depthWriteEnable && !properties.blendEnable);
			//TODO there will be times when we want to write to depth when there is blend enabled
			VkPipelineDepthStencilStateCreateInfo depthStencilStateCI{};
			depthStencilStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			depthStencilStateCI.depthTestEnable = properties.depthTestEnable;
			depthStencilStateCI.depthWriteEnable = enableDepthWrite;
			depthStencilStateCI.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
			depthStencilStateCI.front = depthStencilStateCI.back;
			depthStencilStateCI.back.compareOp = VK_COMPARE_OP_ALWAYS;

			VkPipelineViewportStateCreateInfo viewportStateCI{};
			viewportStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewportStateCI.viewportCount = 1;
			viewportStateCI.scissorCount = 1;
			viewportStateCI.flags = 0;

			VkPipelineMultisampleStateCreateInfo multisampleStateCI{};
			multisampleStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			multisampleStateCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
			multisampleStateCI.flags = 0;

			VkPipelineDynamicStateCreateInfo dynamicStateCI{};
			dynamicStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			dynamicStateCI.pDynamicStates = dynamicStateEnables.data();
			dynamicStateCI.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
			dynamicStateCI.flags = 0;

			VkPipelineVertexInputStateCreateInfo vertexInputState{};
			vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInputBindings.size());
			vertexInputState.pVertexBindingDescriptions = vertexInputBindings.data();
			vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
			vertexInputState.pVertexAttributeDescriptions = vertexInputAttributes.data();

			VkGraphicsPipelineCreateInfo pipelineCreateInfoCI{};
			pipelineCreateInfoCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			pipelineCreateInfoCI.layout = m_pipelineLayout;
			pipelineCreateInfoCI.renderPass = _renderPass;
			pipelineCreateInfoCI.flags = 0;
			pipelineCreateInfoCI.basePipelineIndex = -1;
			pipelineCreateInfoCI.basePipelineHandle = VK_NULL_HANDLE;
			pipelineCreateInfoCI.pVertexInputState = &vertexInputState;
			pipelineCreateInfoCI.pInputAssemblyState = &inputAssemblyStateCI;
			pipelineCreateInfoCI.pRasterizationState = &rasterizationStateCI;
			pipelineCreateInfoCI.pColorBlendState = &colorBlendStateCI;
			pipelineCreateInfoCI.pMultisampleState = &multisampleStateCI;
			pipelineCreateInfoCI.pViewportState = &viewportStateCI;
			pipelineCreateInfoCI.pDepthStencilState = &depthStencilStateCI;
			pipelineCreateInfoCI.pDynamicState = &dynamicStateCI;
			pipelineCreateInfoCI.subpass = properties.subpass;

			std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
			shaderStages.push_back(LoadShader(vertexFile, VK_SHADER_STAGE_VERTEX_BIT));

			if (properties.depthBias)
			{
				m_depthBias = true;
				// Cull front faces
				depthStencilStateCI.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
				// Enable depth bias
				rasterizationStateCI.depthBiasEnable = VK_TRUE;
				// Add depth bias to dynamic state, so we can change it at runtime
				dynamicStateEnables.push_back(VK_DYNAMIC_STATE_DEPTH_BIAS);
				dynamicStateCI.pDynamicStates = dynamicStateEnables.data();
				dynamicStateCI.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
			}

			//TODO remove hardcode
			if (fragmentFile.empty())
			{
				pipelineCreateInfoCI.stageCount = 1;
				// No blend attachment states (no color attachments used)
				colorBlendStateCI.attachmentCount = 0;

			}
			else
			{
				shaderStages.push_back(LoadShader(fragmentFile, VK_SHADER_STAGE_FRAGMENT_BIT));

				if (properties.fragmentConstants)
				{
					VkSpecializationMapEntry specializationMapEntry{};
					specializationMapEntry.constantID = 0;
					specializationMapEntry.offset = 0;
					specializationMapEntry.size = sizeof(uint32_t);

					VkSpecializationInfo specializationInfo{};
					specializationInfo.mapEntryCount = 1;
					specializationInfo.pMapEntries = &specializationMapEntry;
					specializationInfo.dataSize = sizeof(uint32_t);
					specializationInfo.pData = properties.fragmentConstants;
					shaderStages[1].pSpecializationInfo = &specializationInfo;
				}
			}

			pipelineCreateInfoCI.stageCount = static_cast<uint32_t>(shaderStages.size());
			pipelineCreateInfoCI.pStages = shaderStages.data();

			VK_CHECK_RESULT(vkCreateGraphicsPipelines(_device, _pipelineCache, 1, &pipelineCreateInfoCI, nullptr, &m_vkPipeline));

			for (auto& shaderModule : m_shaderModules)
			{
				vkDestroyShaderModule(_device, shaderModule, nullptr);
			}
			m_shaderModules.clear();
		}

		void VulkanPipeline::Draw(VkCommandBuffer command_buffer, VkPipelineBindPoint bindpoint)
		{
			vkCmdBindPipeline(command_buffer, bindpoint, m_vkPipeline);
		}

		void VulkanPipeline::Draw(CommandBuffer* commandBuffer)
		{
			VulkanCommandBuffer* cb = dynamic_cast<VulkanCommandBuffer*>(commandBuffer);

			float depthBiasConstant = 1.25f;
			float depthBiasSlope = 0.75f;
			if(m_depthBias)
			vkCmdSetDepthBias(
				cb->m_vkCommandBuffer,
				depthBiasConstant,
				0.0f,
				depthBiasSlope);
			
			Draw(cb->m_vkCommandBuffer);
		}

		void VulkanPipeline::CreateCompute(std::string file, VkDevice device, VkDescriptorSetLayout descriptorSetLayout, VkPipelineCache cache, PipelineProperties properties)
		{
			if (m_vkPipeline != VK_NULL_HANDLE)
				return;

			_device = device;
			_pipelineCache = cache;

			VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
			pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutCreateInfo.setLayoutCount = 1;
			pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;

			if (properties.vertexConstantBlockSize > 0)
			{
				VkPushConstantRange pushConstantRange{};
				pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
				pushConstantRange.offset = 0;
				pushConstantRange.size = properties.vertexConstantBlockSize;				
				
				pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
				pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;
			}

			VK_CHECK_RESULT(vkCreatePipelineLayout(_device, &pipelineLayoutCreateInfo, nullptr, &m_pipelineLayout));

			VkComputePipelineCreateInfo computePipelineCreateInfo{};
			computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
			computePipelineCreateInfo.layout = m_pipelineLayout;
			computePipelineCreateInfo.flags = 0;

			computePipelineCreateInfo.stage = LoadShader(file, VK_SHADER_STAGE_COMPUTE_BIT);

			VK_CHECK_RESULT(vkCreateComputePipelines(device, _pipelineCache, 1, &computePipelineCreateInfo, nullptr, &m_vkPipeline));

			for (auto& shaderModule : m_shaderModules)
			{
				vkDestroyShaderModule(_device, shaderModule, nullptr);
			}
			m_shaderModules.clear();
		}
	}
}