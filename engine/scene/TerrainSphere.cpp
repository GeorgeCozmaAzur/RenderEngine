#include "TerrainSphere.h"


namespace engine
{
	namespace scene
	{
		void UVSphere::Init(const std::string& filename, float radius, render::VulkanDevice* vulkanDevice, render::VertexLayout* vertex_layout, render::VulkanBuffer* globalUniformBufferVS, std::vector<VkDescriptorImageInfo*> texturesDescriptors, std::string vertexShaderFilename, std::string fragmentShaderFilename, VkRenderPass renderPass, VkPipelineCache pipelineCache, VkQueue queue)
		{
			_vertexLayout = vertex_layout;
	
			LoadHeightmap(filename, 2.0);

			int slices = m_width;
			int rings = m_length;

			Geometry* geometry = new Geometry;
			geometry->m_vertexCount = (rings - 1) * slices;
			geometry->m_verticesSize = geometry->m_vertexCount * vertex_layout->GetVertexSize(0) / sizeof(float);
			geometry->m_vertices = new float[geometry->m_verticesSize];

			std::vector<glm::vec3> positions;
			std::vector<glm::vec3> normals;
			std::vector<glm::vec3> heightmapnormals;
			std::vector<glm::vec3> tangents;
			std::vector<glm::vec3> bitangents;
			std::vector<glm::vec2> uvs;
			bool hastangents = false;

			int vertex_index = 0;
			uint32_t index_index = 0;
			float finalradius = radius;
			for (int i = 0; i < rings - 1; i++)
			{
				auto phi = M_PI * double(i + 1) / double(rings);
				for (int j = 0; j < slices; j++)
				{
					auto theta = 2.0 * M_PI * double(j) / double(slices);

					finalradius = radius + GetHeight(j, i) * 0.1f;
					auto x = std::sin(phi) * std::cos(theta) * finalradius;
					auto y = std::cos(phi) * finalradius;
					auto z = std::sin(phi) * std::sin(theta) * finalradius;

					for (auto& component : vertex_layout->m_components[0])
					{
						switch (component) {
						case render::VERTEX_COMPONENT_POSITION:
							geometry->m_vertices[vertex_index++] = x;
							geometry->m_vertices[vertex_index++] = y;
							geometry->m_vertices[vertex_index++] = z;
							positions.push_back(glm::vec3(x,y,z));
							break;
						case render::VERTEX_COMPONENT_NORMAL:
							glm::vec3 normal(x,y,z);
							normal = glm::normalize(normal);
							geometry->m_vertices[vertex_index++] = normal.x;
							geometry->m_vertices[vertex_index++] = normal.y;
							geometry->m_vertices[vertex_index++] = normal.z;
							normals.push_back(normal);
							glm::vec3 heightmapnormal = GetNormal(j, i);
							heightmapnormals.push_back(heightmapnormal);
							tangents.push_back(glm::vec3(0.0f));
							bitangents.push_back(glm::vec3(0.0f));
							break;
						case render::VERTEX_COMPONENT_UV:
						{
							float u = float(j) / ((float)slices);
							float v = float(i) / ((float)rings - 1);
							geometry->m_vertices[vertex_index++] = u;
							geometry->m_vertices[vertex_index++] = v;
							uvs.push_back(glm::vec2(u, v));
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
							
							hastangents = true;
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
			geometry->m_indices = BuildPatchIndices(0, 0, slices, rings-1, sizeofindices);
			geometry->m_indexCount = sizeofindices;

			hastangents = true;

			if (hastangents)
			{
				//tangents
				for (int i = 0; i < sizeofindices; i += 3)//screw the last two vertices//george era 2 in loc de 20
				{

					int first = geometry->m_indices[i];
					int second = geometry->m_indices[i + 1];
					int third = geometry->m_indices[i + 2];

					if (tangents[first] == glm::vec3(0.0f))
					{
						glm::vec3 tangent, bitangent;
						ComputeTangents(positions[first], positions[second], positions[third],
							uvs[geometry->m_indices[i]], uvs[geometry->m_indices[i + 1]], uvs[geometry->m_indices[i + 2]],
							tangent, bitangent);

						tangents[first] = tangent;
						bitangents[first] = bitangent;
					}

					if (tangents[second] == glm::vec3(0.0f))
					{
						glm::vec3 tangent, bitangent;
						ComputeTangents(positions[second], positions[first], positions[third],
							uvs[geometry->m_indices[i + 1]], uvs[geometry->m_indices[i]], uvs[geometry->m_indices[i + 2]],
							tangent, bitangent);


						tangents[second] = tangent;
						bitangents[second] = bitangent;
					}

					if (tangents[third] == glm::vec3(0.0f))
					{
						glm::vec3 tangent, bitangent;
						ComputeTangents(positions[geometry->m_indices[i + 2]], positions[geometry->m_indices[i]], positions[geometry->m_indices[i + 1]],
							uvs[geometry->m_indices[i + 2]], uvs[geometry->m_indices[i]], uvs[geometry->m_indices[i + 1]],
							tangent, bitangent);

						tangents[geometry->m_indices[i + 2]] = tangent;
						bitangents[geometry->m_indices[i + 2]] = bitangent;
					}
				}

				uint32_t tangent_offset = 0;
				for (auto& component : vertex_layout->m_components[0])
				{
					if (component == render::VERTEX_COMPONENT_NORMAL)
						break;
					tangent_offset += vertex_layout->GetComponentSize(component);
				}
				tangent_offset /= sizeof(float);
				int j = 0;
				for (int i = tangent_offset; i < geometry->m_verticesSize; i += tangent_offset)
				{
					glm::mat3 TBN = glm::mat3(tangents[j], normals[j], bitangents[j]);
					glm::vec3 finalnormal = TBN * heightmapnormals[j];

					geometry->m_vertices[i] = finalnormal.x;
					geometry->m_vertices[++i] = finalnormal.y;
					geometry->m_vertices[++i] = finalnormal.z;
					i+=3;
					j++;
				}
			}
			
			geometry->SetIndexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geometry->m_indexCount * sizeof(uint32_t), geometry->m_indices));
			geometry->SetVertexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geometry->m_verticesSize * sizeof(float), geometry->m_vertices), true);
			m_geometries.push_back(geometry);

			uniformBufferVS = vulkanDevice->GetUniformBuffer(sizeof(uboVS), true, queue);
			uniformBufferVS->map();

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

			m_descriptorSets.push_back(vulkanDevice->GetDescriptorSet({ &globalUniformBufferVS->m_descriptor, &uniformBufferVS->m_descriptor }, texturesDescriptors,
				_descriptorLayout->m_descriptorSetLayout, _descriptorLayout->m_setLayoutBindings));

			_pipeline = vulkanDevice->GetPipeline(_descriptorLayout->m_descriptorSetLayout, _vertexLayout->m_vertexInputBindings, _vertexLayout->m_vertexInputAttributes,
				engine::tools::getAssetPath() + "shaders/" + vertexShaderFilename +".vert.spv", engine::tools::getAssetPath() + "shaders/" + fragmentShaderFilename + ".frag.spv", renderPass, pipelineCache, false, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
		}

		void UVSphere::UpdateUniforms(glm::mat4& model)
		{
			if(uniformBufferVS)
			uniformBufferVS->copyTo(&model, sizeof(model));
		}
	}
}