#include "VulkanPipeline.h"
#include <array>
namespace engine
{
	namespace render
	{
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


		void VulkanPipeline::Create(VkDevice device,
			VkDescriptorSetLayout descriptorSetLayout,
			std::vector<VkVertexInputBindingDescription> vertexInputBindings, std::vector<VkVertexInputAttributeDescription> vertexInputAttributes,
			std::string vertexFile, std::string fragmentFile,
			VkRenderPass renderPass, VkPipelineCache cache,
			VkPrimitiveTopology topology,
			uint32_t vertexConstantBlockSize,
			uint32_t* fragmentConstants,
			uint32_t attachmentCount,
			const VkPipelineColorBlendAttachmentState* pAttachments,
			bool depthBias,
			bool depthTestEnable,
			bool depthWriteEnable,
			uint32_t subpass)
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

			if (vertexConstantBlockSize > 0)
			{
				VkPushConstantRange pushConstantRange = engine::initializers::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, vertexConstantBlockSize, 0);
				pipelineLayoutCI.pushConstantRangeCount = 1;
				pipelineLayoutCI.pPushConstantRanges = &pushConstantRange;
			}

			// The pipeline layout is based on the descriptor set layout we created above
			pipelineLayoutCI.setLayoutCount = 1;
			pipelineLayoutCI.pSetLayouts = &descriptorSetLayout;
			VK_CHECK_RESULT(vkCreatePipelineLayout(_device, &pipelineLayoutCI, nullptr, &m_pipelineLayout));

			std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

			VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI = engine::initializers::pipelineInputAssemblyStateCreateInfo(topology, 0, VK_FALSE);
			VkPipelineRasterizationStateCreateInfo rasterizationStateCI = m_blendEnable ? engine::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE, 0) :
				engine::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE, 0);
			VkPipelineColorBlendAttachmentState blendAttachmentState = engine::initializers::pipelineColorBlendAttachmentState(0xf, VkBool32(m_blendEnable));

			VkPipelineColorBlendStateCreateInfo colorBlendStateCI = (attachmentCount && pAttachments) ?
				engine::initializers::pipelineColorBlendStateCreateInfo(attachmentCount, pAttachments) :
				engine::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
				
			VkBool32 enableDepthWrite = VkBool32(depthWriteEnable && !m_blendEnable);
			//TODO there will be times when we want to write to depth when there is blend enabled
			VkPipelineDepthStencilStateCreateInfo depthStencilStateCI = engine::initializers::pipelineDepthStencilStateCreateInfo(depthTestEnable, enableDepthWrite, VK_COMPARE_OP_LESS_OR_EQUAL);
			VkPipelineViewportStateCreateInfo viewportStateCI = engine::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
			VkPipelineMultisampleStateCreateInfo multisampleStateCI = engine::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
			VkPipelineDynamicStateCreateInfo dynamicStateCI = engine::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables.data(), static_cast<uint32_t>(dynamicStateEnables.size()), 0);

			VkPipelineVertexInputStateCreateInfo vertexInputState = engine::initializers::pipelineVertexInputStateCreateInfo();
			vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInputBindings.size());
			vertexInputState.pVertexBindingDescriptions = vertexInputBindings.data();
			vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
			vertexInputState.pVertexAttributeDescriptions = vertexInputAttributes.data();

			VkGraphicsPipelineCreateInfo pipelineCreateInfoCI = engine::initializers::pipelineCreateInfo(m_pipelineLayout, _renderPass, 0);
			pipelineCreateInfoCI.pVertexInputState = &vertexInputState;
			pipelineCreateInfoCI.pInputAssemblyState = &inputAssemblyStateCI;
			pipelineCreateInfoCI.pRasterizationState = &rasterizationStateCI;
			pipelineCreateInfoCI.pColorBlendState = &colorBlendStateCI;
			pipelineCreateInfoCI.pMultisampleState = &multisampleStateCI;
			pipelineCreateInfoCI.pViewportState = &viewportStateCI;
			pipelineCreateInfoCI.pDepthStencilState = &depthStencilStateCI;
			pipelineCreateInfoCI.pDynamicState = &dynamicStateCI;
			pipelineCreateInfoCI.subpass = subpass;

			std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
			shaderStages.push_back(LoadShader(vertexFile, VK_SHADER_STAGE_VERTEX_BIT));

			/*const std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = {
				LoadShader(vertex_file, VK_SHADER_STAGE_VERTEX_BIT),
				LoadShader(fragment_file, VK_SHADER_STAGE_FRAGMENT_BIT)
			};*/

			if (depthBias)
			{
				// Cull front faces
				depthStencilStateCI.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
				// Enable depth bias
				rasterizationStateCI.depthBiasEnable = VK_TRUE;
				// Add depth bias to dynamic state, so we can change it at runtime
				dynamicStateEnables.push_back(VK_DYNAMIC_STATE_DEPTH_BIAS);
				dynamicStateCI =
					engine::initializers::pipelineDynamicStateCreateInfo(
						dynamicStateEnables.data(),
						dynamicStateEnables.size(),
						0);
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

				if (fragmentConstants)
				{
					VkSpecializationMapEntry specializationMapEntry = engine::initializers::specializationMapEntry(0, 0, sizeof(uint32_t));
					VkSpecializationInfo specializationInfo = engine::initializers::specializationInfo(1, &specializationMapEntry, sizeof(uint32_t), fragmentConstants);
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
			m_blendEnable = properties.blendEnable;

			/*
				[POI] Create a pipeline layout used for our graphics pipeline
			*/
			VkPipelineLayoutCreateInfo pipelineLayoutCI{};
			pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

			if (properties.vertexConstantBlockSize > 0)
			{
				VkPushConstantRange pushConstantRange = engine::initializers::pushConstantRange(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, properties.vertexConstantBlockSize, 0);
				pipelineLayoutCI.pushConstantRangeCount = 1;
				pipelineLayoutCI.pPushConstantRanges = &pushConstantRange;
			}

			// The pipeline layout is based on the descriptor set layout we created above
			pipelineLayoutCI.setLayoutCount = 1;
			pipelineLayoutCI.pSetLayouts = &descriptorSetLayout;
			VK_CHECK_RESULT(vkCreatePipelineLayout(_device, &pipelineLayoutCI, nullptr, &m_pipelineLayout));

			std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

			VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI = engine::initializers::pipelineInputAssemblyStateCreateInfo(properties.topology, 0, VK_FALSE);
			VkPipelineRasterizationStateCreateInfo rasterizationStateCI = m_blendEnable ? engine::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE, 0) :
				engine::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, properties.cullMode, VK_FRONT_FACE_CLOCKWISE, 0);
			VkPipelineColorBlendAttachmentState blendAttachmentState = engine::initializers::pipelineColorBlendAttachmentState(0xf, VkBool32(m_blendEnable));

			VkPipelineColorBlendStateCreateInfo colorBlendStateCI = (properties.attachmentCount && properties.pAttachments) ?
				engine::initializers::pipelineColorBlendStateCreateInfo(properties.attachmentCount, properties.pAttachments) :
				engine::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);

			VkBool32 enableDepthWrite = VkBool32(properties.depthWriteEnable);
			//TODO there will be times when we want to write to depth when there is blend enabled
			VkPipelineDepthStencilStateCreateInfo depthStencilStateCI = engine::initializers::pipelineDepthStencilStateCreateInfo(properties.depthTestEnable, enableDepthWrite, VK_COMPARE_OP_LESS_OR_EQUAL);
			VkPipelineViewportStateCreateInfo viewportStateCI = engine::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
			VkPipelineMultisampleStateCreateInfo multisampleStateCI = engine::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
			VkPipelineDynamicStateCreateInfo dynamicStateCI = engine::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables.data(), static_cast<uint32_t>(dynamicStateEnables.size()), 0);

			VkPipelineVertexInputStateCreateInfo vertexInputState = engine::initializers::pipelineVertexInputStateCreateInfo();
			vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInputBindings.size());
			vertexInputState.pVertexBindingDescriptions = vertexInputBindings.data();
			vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
			vertexInputState.pVertexAttributeDescriptions = vertexInputAttributes.data();

			VkGraphicsPipelineCreateInfo pipelineCreateInfoCI = engine::initializers::pipelineCreateInfo(m_pipelineLayout, _renderPass, 0);
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

			/*const std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = {
				LoadShader(vertex_file, VK_SHADER_STAGE_VERTEX_BIT),
				LoadShader(fragment_file, VK_SHADER_STAGE_FRAGMENT_BIT)
			};*/

			if (properties.depthBias)
			{
				// Cull front faces
				depthStencilStateCI.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
				// Enable depth bias
				rasterizationStateCI.depthBiasEnable = VK_TRUE;
				// Add depth bias to dynamic state, so we can change it at runtime
				dynamicStateEnables.push_back(VK_DYNAMIC_STATE_DEPTH_BIAS);
				dynamicStateCI =
					engine::initializers::pipelineDynamicStateCreateInfo(
						dynamicStateEnables.data(),
						dynamicStateEnables.size(),
						0);
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
					VkSpecializationMapEntry specializationMapEntry = engine::initializers::specializationMapEntry(0, 0, sizeof(uint32_t));
					VkSpecializationInfo specializationInfo = engine::initializers::specializationInfo(1, &specializationMapEntry, sizeof(uint32_t), properties.fragmentConstants);
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

		void VulkanPipeline::CreateCompute(std::string file, VkDevice device, VkDescriptorSetLayout descriptorSetLayout, VkPipelineCache cache)
		{
			if (m_vkPipeline != VK_NULL_HANDLE)
				return;

			_device = device;
			_pipelineCache = cache;

			VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo =
				initializers::pipelineLayoutCreateInfo(&descriptorSetLayout, 1);

			VK_CHECK_RESULT(vkCreatePipelineLayout(_device, &pPipelineLayoutCreateInfo, nullptr, &m_pipelineLayout));

			VkComputePipelineCreateInfo computePipelineCreateInfo =
				initializers::computePipelineCreateInfo(m_pipelineLayout, 0);

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