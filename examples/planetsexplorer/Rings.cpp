#include "Rings.h"


namespace engine
{
	namespace scene
	{
		uint32_t* Rings::BuildIndices(int offsetX, int offsetY, int width, int length, int& size)
		{
			size = (width - 1) * (length) * 6;
			uint32_t* indices = new uint32_t[size];

			for (int y = 0; y < length; y++)
			{
				for (int x = 0; x < width - 1; x++)
				{
					int i = x + y * (width - 1);
					int vi = x + y * width;

					int x0y0 = vi;
					int x1y0 = vi + 1;
					int x0y1 = vi + width;
					if (y == length-1)
						x0y1 = 0;
					int x1y1 = vi + width + 1;
					if (y == length-1)
						x1y1 = 1;


					/*   0___
					 *   |  /|
					 *   | / |
					 *   |/__|
					 *
					 */
					 /*indices[i * 6 + 0] = x1y0;
					 indices[i * 6 + 1] = x0y0;
					 indices[i * 6 + 2] = x0y1;*/
					indices[i * 6 + 0] = x1y0;
					indices[i * 6 + 1] = x0y1;
					indices[i * 6 + 2] = x0y0;
					indices[i * 6 + 3] = x0y1;
					indices[i * 6 + 4] = x1y0;
					indices[i * 6 + 5] = x1y1;

				}
			}

			return indices;
		}

		void Rings::Init(float innerRadius, float outerRadius, int resolution, render::VulkanDevice* vulkanDevice, VkDescriptorPool descriptorPool, render::VulkanVertexLayout* vertex_layout, render::VulkanBuffer* globalUniformBufferVS, std::vector<VkDescriptorImageInfo*> texturesDescriptors, std::string vertexShaderFilename, std::string fragmentShaderFilename, VkRenderPass renderPass, VkPipelineCache pipelineCache, render::PipelineProperties pipelineProperties, VkQueue queue)
		{
			_vertexLayout = vertex_layout;

			int slices = resolution;
			int rings = 2;

			Geometry* geometry = new Geometry;
			geometry->m_vertexCount = rings * (slices);
			geometry->m_verticesSize = geometry->m_vertexCount * vertex_layout->GetVertexSize(0) / sizeof(float);
			geometry->m_vertices = new float[geometry->m_verticesSize];

			int vertex_index = 0;
			//uint32_t index_index = 0;

			for (int i = 0; i < resolution; i++)
			{
				/*auto phi = M_PI * double(i + 1) / double(rings);*/
				float radius = innerRadius;
				for (int j = 0; j < 2; j++)
				{
					auto theta = 2.0 * M_PI * double(i) / double(resolution);
					
					auto x = (float)(std::cos(theta)) * radius;
					auto z = (float)(std::sin(theta)) * radius;

					radius = outerRadius;

					for (auto& component : vertex_layout->m_components[0])
					{
						switch (component) {
						case render::VERTEX_COMPONENT_POSITION:
							geometry->m_vertices[vertex_index++] = x;
							geometry->m_vertices[vertex_index++] = 0;
							geometry->m_vertices[vertex_index++] = z;
							break;
						case render::VERTEX_COMPONENT_NORMAL:
							glm::vec3 normal(0.0, -1.0, 0.0);
							normal = glm::normalize(normal);
							geometry->m_vertices[vertex_index++] = normal.x;
							geometry->m_vertices[vertex_index++] = normal.y;
							geometry->m_vertices[vertex_index++] = normal.z;
							break;
						case render::VERTEX_COMPONENT_UV:
						{
							float uv = float(i) / ((float)resolution);

							geometry->m_vertices[vertex_index++] = (j==0 ? 1 : 0.0f);
							geometry->m_vertices[vertex_index++] = uv;//(j == 1 ? 1 : 0.0f);

							break;
						}
						case render::VERTEX_COMPONENT_COLOR:
							geometry->m_vertices[vertex_index++] = 0;
							geometry->m_vertices[vertex_index++] = 0;
							geometry->m_vertices[vertex_index++] = 0;
							break;
						case render::VERTEX_COMPONENT_TANGENT:
							geometry->m_vertices[vertex_index++] = 0;
							geometry->m_vertices[vertex_index++] = 0;
							geometry->m_vertices[vertex_index++] = 0;
							break;
						case render::VERTEX_COMPONENT_BITANGENT:
							geometry->m_vertices[vertex_index++] = 0;
							geometry->m_vertices[vertex_index++] = 0;
							geometry->m_vertices[vertex_index++] = 0;						
							break;
						};
					}
				}
			}

			int sizeofindices = 0;
			geometry->m_indices = BuildIndices(0, 0, 2, resolution, sizeofindices);
			geometry->m_indexCount = sizeofindices;

			/*geometry->m_indices = new uint32_t[6];
			geometry->m_indexCount = 6;

			geometry->m_indices[0] = 5;
			geometry->m_indices[1] = 0;
			geometry->m_indices[2] = 4;
			geometry->m_indices[3] = 0;
			geometry->m_indices[4] = 5;
			geometry->m_indices[5] = 1;*/
			/*for (int i = 0;i < sizeofindices;i++)
			{
				std::cout << geometry->m_indices[i] << "\n";
			}*/
			
			geometry->SetIndexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geometry->m_indexCount * sizeof(uint32_t), geometry->m_indices));
			geometry->SetVertexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geometry->m_verticesSize * sizeof(float), geometry->m_vertices), true);
			m_geometries.push_back(geometry);

			uniformBufferVS = vulkanDevice->GetUniformBuffer(sizeof(uboVS), true, queue);
			uniformBufferVS->Map();

			std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> bindings
			{
				{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
				{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT}
			};
			for (auto desc : texturesDescriptors)
			{
				bindings.push_back({ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT });
			}

			_descriptorLayout = vulkanDevice->GetDescriptorSetLayout(bindings);

			m_descriptorSets.push_back(vulkanDevice->GetDescriptorSet(descriptorPool, { &globalUniformBufferVS->m_descriptor, &uniformBufferVS->m_descriptor }, texturesDescriptors,
				_descriptorLayout->m_descriptorSetLayout, _descriptorLayout->m_setLayoutBindings));

			_pipeline = vulkanDevice->GetPipeline(_descriptorLayout->m_descriptorSetLayout, _vertexLayout->m_vertexInputBindings, _vertexLayout->m_vertexInputAttributes,
				engine::tools::getAssetPath() + "shaders/" + vertexShaderFilename + ".vert.spv", engine::tools::getAssetPath() + "shaders/" + fragmentShaderFilename + ".frag.spv", renderPass, pipelineCache, pipelineProperties);

		}

		void Rings::UpdateUniforms(glm::mat4& model)
		{
			if(uniformBufferVS)
			uniformBufferVS->MemCopy(&model, sizeof(model));
		}
	}
}