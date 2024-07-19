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

		void DrawDebugVectors::CreateDebugVectorsGeometry(glm::vec3 position, std::vector<glm::vec3> directions, std::vector<glm::vec3> colors)
		{
			Geometry* geo = new Geometry;
			geo->m_indexCount = static_cast<uint32_t>(directions.size()) * 2;
			geo->m_indices = new uint32_t[geo->m_indexCount];
			int ii = 0;
			for (int i = 0; i < directions.size(); i++)
			{
				geo->m_indices[ii++] = 0;
				geo->m_indices[ii++] = i + 1;
			}

			int vindex = 0;
			geo->m_vertexCount = directions.size() + 1;
			geo->m_verticesSize = geo->m_vertexCount * 6;
			geo->m_vertices = new float[geo->m_verticesSize];

			geo->m_vertices[vindex++] = position.x;
			geo->m_vertices[vindex++] = position.y;
			geo->m_vertices[vindex++] = position.z;
			geo->m_vertices[vindex++] = 1.0;
			geo->m_vertices[vindex++] = 1.0;
			geo->m_vertices[vindex++] = 1.0;

			for (int i = 0; i < directions.size(); i++)
			{
				geo->m_vertices[vindex++] = position.x + directions[i].x;
				geo->m_vertices[vindex++] = position.y + directions[i].y;
				geo->m_vertices[vindex++] = position.z + directions[i].z;
				geo->m_vertices[vindex++] = colors[i].x;//abs(directions[i].x);
				geo->m_vertices[vindex++] = colors[i].y;//abs(directions[i].y);
				geo->m_vertices[vindex++] = colors[i].z;//abs(directions[i].z);
			}

			geo->m_instanceNo = 1;
			m_geometries.push_back(geo);
		}

		void DrawDebugVectors::Init(render::VulkanDevice* vulkanDevice, render::VulkanBuffer* globalUniformBufferVS, VkQueue queue, VkRenderPass renderPass, VkPipelineCache pipelineCache)
		{
			std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> bindings
			{
				{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT}
			};
			_descriptorLayout = vulkanDevice->GetDescriptorSetLayout(bindings);

			_vertexLayout = &debugVertexLayout;

			m_descriptorSets.push_back(vulkanDevice->GetDescriptorSet({ &globalUniformBufferVS->m_descriptor }, {},
				_descriptorLayout->m_descriptorSetLayout, _descriptorLayout->m_setLayoutBindings));

			_pipeline = vulkanDevice->GetPipeline(_descriptorLayout->m_descriptorSetLayout, _vertexLayout->m_vertexInputBindings, _vertexLayout->m_vertexInputAttributes,
				engine::tools::getAssetPath() + "shaders/drawdebug/vertexcolored.vert.spv", engine::tools::getAssetPath() + "shaders/drawdebug/colored.frag.spv", renderPass, pipelineCache, false, VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
		}

		void DrawDebugBBs::Init(std::vector<std::vector<glm::vec3>> boundries, render::VulkanDevice* vulkanDevice, render::VulkanBuffer* globalUniformBufferVS, VkQueue queue, VkRenderPass renderPass, VkPipelineCache pipelineCache, uint32_t constantSize)
		{
			_vertexLayout = &debugVertexLayout;

			for (auto bb : boundries)
			{
				glm::vec3 first = bb[0];
				glm::vec3 second = bb[1];
				scene::Geometry* geometry = new scene::Geometry();
				geometry->_device = vulkanDevice->logicalDevice;
				geometry->m_instanceNo = 1;
				std::vector<uint32_t> wire_indices = { 0,1,1,2,2,3,3,0, 0,4,3,7,2,6,1,5,4,5,5,6,6,7,7,4 };
				geometry->m_indexCount = static_cast<uint32_t>(wire_indices.size());
				geometry->m_indices = new uint32_t[wire_indices.size()];
				for (int i = 0; i < wire_indices.size(); i++)
				{
					geometry->m_indices[i] = wire_indices[i];
				}
				geometry->m_vertexCount = 8;
				geometry->m_verticesSize = geometry->m_vertexCount * _vertexLayout->GetVertexSize(0);
				geometry->m_vertices = new float[geometry->m_verticesSize];

				std::vector<glm::vec3> vertices;
				vertices.reserve(geometry->m_vertexCount * 2);
				vertices.push_back(first);
				vertices.push_back(glm::vec3(0.0, 1.0, 0.0));
				vertices.push_back(glm::vec3(second.x, first.y, first.z));
				vertices.push_back(glm::vec3(0.0, 1.0, 0.0));
				vertices.push_back(glm::vec3(second.x, first.y, second.z));
				vertices.push_back(glm::vec3(0.0, 1.0, 0.0));
				vertices.push_back(glm::vec3(first.x, first.y, second.z));
				vertices.push_back(glm::vec3(0.0, 1.0, 0.0));

				vertices.push_back(glm::vec3(first.x, second.y, first.z));
				vertices.push_back(glm::vec3(0.0, 0.0, 1.0));
				vertices.push_back(glm::vec3(second.x, second.y, first.z));
				vertices.push_back(glm::vec3(0.0, 0.0, 1.0));
				vertices.push_back(second);
				vertices.push_back(glm::vec3(0.0, 0.0, 1.0));
				vertices.push_back(glm::vec3(first.x, second.y, second.z));
				vertices.push_back(glm::vec3(0.0, 0.0, 1.0));

				memcpy(geometry->m_vertices, vertices.data(), geometry->m_verticesSize);

				geometry->SetIndexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geometry->m_indexCount * sizeof(uint32_t), geometry->m_indices), false);
				geometry->SetVertexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geometry->m_verticesSize * sizeof(float), geometry->m_vertices), false);
				m_geometries.push_back(geometry);
			}
			std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> bindings
			{
				{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT}
			};
			_descriptorLayout = vulkanDevice->GetDescriptorSetLayout(bindings);

			m_descriptorSets.push_back(vulkanDevice->GetDescriptorSet({ &globalUniformBufferVS->m_descriptor }, {},
				_descriptorLayout->m_descriptorSetLayout, _descriptorLayout->m_setLayoutBindings));

			_pipeline = vulkanDevice->GetPipeline(_descriptorLayout->m_descriptorSetLayout, _vertexLayout->m_vertexInputBindings, _vertexLayout->m_vertexInputAttributes,
				engine::tools::getAssetPath() + "shaders/drawdebug/colored.vert.spv", engine::tools::getAssetPath() + "shaders/drawdebug/colored.frag.spv", renderPass, pipelineCache, false, VK_PRIMITIVE_TOPOLOGY_LINE_LIST, constantSize);

		}

	}
}