#include "DeferredLights.h"

namespace engine
{
	namespace scene
	{
		float randomFloat() {
			return (float)rand() / ((float)RAND_MAX + 1);
		}

		void DeferredLights::Init(render::VulkanBuffer* ub, render::VulkanDevice* device, VkQueue queue, VkRenderPass renderPass, VkPipelineCache pipelineCache, int lightsNumber, render::VulkanTexture* positions, render::VulkanTexture* normals, render::VulkanTexture* roughnessMetallic, render::VulkanTexture* albedo)
		{
			vulkanDevice = device;
			_vertexLayout = new render::VertexLayout(
				{
					render::VERTEX_COMPONENT_POSITION
				},
				{ 
					render::VERTEX_COMPONENT_DUMMY_VEC4,
					render::VERTEX_COMPONENT_DUMMY_VEC4 
				});

			LoadGeometry(engine::tools::getAssetPath() + "models/sphere.obj", _vertexLayout, 0.05f, lightsNumber, glm::vec3(0.0, 0.0, 0.0));

			std::string shaderVertexName = "shaders/basicdeferred/deferredlights.vert.spv";
			std::string shaderFragmentName = "shaders/basicdeferred/deferredlights.frag.spv";

			
			m_pointLights.resize(lightsNumber * 2);
			for (int i=0;i< m_pointLights.size();i++)
			{
				m_pointLights[i] = glm::vec4(0.0f, 0.5f, 0.0f, 3.0f);;
				m_pointLights[++i] = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			}

			for (auto geo : m_geometries)
			{
				geo->SetIndexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geo->m_indexCount * sizeof(uint32_t), geo->m_indices));
				geo->SetVertexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geo->m_verticesSize * sizeof(float), geo->m_vertices));
				geo->SetInstanceBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
					queue, m_pointLights.size() * sizeof(glm::vec4), m_pointLights.data(),
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));
			}

			std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> modelbindings
			{
				{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
				{VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT},
				{VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT}
			};
			std::vector<VkDescriptorImageInfo*> texturesDescriptors = { &positions->m_descriptor, &normals->m_descriptor };

			//we have a pbr lighting system
			if (roughnessMetallic && albedo)
			{
				modelbindings.push_back({ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT });
				modelbindings.push_back({ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT });
				texturesDescriptors.insert(texturesDescriptors.begin(), &albedo->m_descriptor);
				texturesDescriptors.push_back(&roughnessMetallic->m_descriptor);	
				shaderVertexName = "shaders/basicdeferred/deferredlightspbr.vert.spv";
				shaderFragmentName = "shaders/basicdeferred/deferredlightspbr.frag.spv";
			}

			_descriptorLayout = vulkanDevice->GetDescriptorSetLayout(modelbindings);
			m_descriptorSets.push_back( vulkanDevice->GetDescriptorSet({ &ub->m_descriptor }, texturesDescriptors,
				_descriptorLayout->m_descriptorSetLayout, _descriptorLayout->m_setLayoutBindings));

			std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStates{ {VK_TRUE, 
				VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_DST_ALPHA,
				VK_BLEND_OP_ADD,
				VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_DST_ALPHA,
				VK_BLEND_OP_ADD,
				0xf
				} };

			render::PipelineProperties props;
			props.blendEnable = true;
			props.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			props.attachmentCount = static_cast<uint32_t>(blendAttachmentStates.size());
			props.pAttachments = blendAttachmentStates.data();
			props.depthTestEnable = true;
			props.depthWriteEnable = false;
			props.subpass = 1U;
			props.cullMode = VK_CULL_MODE_BACK_BIT;
			_pipeline = vulkanDevice->GetPipeline(_descriptorLayout->m_descriptorSetLayout, _vertexLayout->m_vertexInputBindings, _vertexLayout->m_vertexInputAttributes,
				engine::tools::getAssetPath() + shaderVertexName, engine::tools::getAssetPath() + shaderFragmentName,
				renderPass, pipelineCache, props);
				//true, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, nullptr, static_cast<uint32_t>(blendAttachmentStates.size()), blendAttachmentStates.data(),false,true,false,1U);
		}

		void DeferredLights::Update()
		{
			for (auto geo : m_geometries)
			{
				geo->UpdateInstanceBuffer(m_pointLights.data(),0, m_pointLights.size() * sizeof(glm::vec4));
			}
		}

		DeferredLights::~DeferredLights()
		{
			delete _vertexLayout;
		}
	}
}