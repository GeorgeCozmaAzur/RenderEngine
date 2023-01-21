#include "DrawDebug.h"

namespace engine
{
	namespace scene
	{
		struct Vertex {
			float pos[3];
			float uv[2];
		};

		void DrawDebugTexture::SetTexture(render::VulkanTexture* texture)
		{
			m_texture = texture;
		}

		void DrawDebugTexture::Init(render::VulkanDevice* vulkanDevice, render::VulkanTexture* texture, VkQueue queue, VkRenderPass renderPass, VkPipelineCache pipelineCache)
		{
			m_hasDepth = texture->m_depth > 1;

			_vertexLayout = &m_vertexLayout;
			m_texture = texture;
			Geometry* geo = new Geometry;
			std::vector<Vertex> vertices =
			{
				{ {  1.0f,  1.0f, 0.0f }, { 1.0f, 1.0f } },
				{ { -1.0f,  1.0f, 0.0f }, { 0.0f, 1.0f } },
				{ { -1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f } },
				{ {  1.0f, -1.0f, 0.0f }, { 1.0f, 0.0f } }
			};

			// Setup indices
			std::vector<uint32_t> indices = { 0,1,2, 2,3,0 };
			geo->m_indexCount = static_cast<uint32_t>(indices.size());
			
			geo->SetIndexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geo->m_indexCount * sizeof(uint32_t), indices.data()));
			geo->SetVertexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, vertices.size() * sizeof(Vertex), vertices.data()));
			m_geometries.push_back(geo);

			std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> bindings
			{
				{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
				{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
			};

			_descriptorLayout = vulkanDevice->GetDescriptorSetLayout(bindings);

			VkDeviceSize bufferSize = m_hasDepth ? sizeof(uboVSdepth) : sizeof(uboVS);
			uniformBufferVS = vulkanDevice->GetBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, sizeof(uboVS));
			VK_CHECK_RESULT(uniformBufferVS->map());

			m_descriptorSets.push_back(vulkanDevice->GetDescriptorSet({ &uniformBufferVS->m_descriptor }, { &m_texture ->m_descriptor },
				_descriptorLayout->m_descriptorSetLayout, _descriptorLayout->m_setLayoutBindings));

			std::string shaderFileName = m_hasDepth ? "texture3d" : "texture";

			_pipeline = vulkanDevice->GetPipeline(_descriptorLayout->m_descriptorSetLayout, m_vertexLayout.m_vertexInputBindings, m_vertexLayout.m_vertexInputAttributes,
				engine::tools::getAssetPath() + "shaders/drawdebug/" + shaderFileName +".vert.spv", engine::tools::getAssetPath() + "shaders/drawdebug/" + shaderFileName + ".frag.spv", renderPass, pipelineCache);
		}

		void DrawDebugTexture::UpdateUniformBuffers(glm::mat4 projection, glm::mat4 view, float depth)
		{
			if (!uniformBufferVS)
				return;

			if (!m_hasDepth)
			{
				uboVS.projection = projection;
				uboVS.modelView = view;
				memcpy(uniformBufferVS->m_mapped, &uboVS, sizeof(uboVS));
			}
			else
			{
				uboVSdepth.projection = projection;
				uboVSdepth.modelView = view;
				uboVSdepth.depth = depth;
				memcpy(uniformBufferVS->m_mapped, &uboVSdepth, sizeof(uboVSdepth));
			}
		}
	}
}