#include "RenderObject.h"

namespace engine
{
	namespace scene
	{
		RenderObject& RenderObject::operator = (const RenderObject& t)
		{
			//m_geometries = std::move(t.m_geometries);
			AdoptGeometriesFrom(t);
			_vertexLayout = t._vertexLayout;
			return *this;
		}

		void RenderObject::AddPipeline(render::VulkanPipeline* pipeline)
		{
			_pipeline = pipeline;
			//TODO verify compatibility		
		}

		void RenderObject::AddDescriptor(render::VulkanDescriptorSet* descriptorset)
		{
			m_descriptorSets.push_back(descriptorset);
			//TODO verify compatibility		
		}
		void RenderObject::AddGeometry(Geometry* geometry)
		{
			m_geometries.push_back(geometry);//TODO verify compatibility
		}

		void RenderObject::PopulateDynamicUniformBufferIndices()
		{
			for (auto geo : m_geometries)
			{
				m_dynamicUniformBufferIndices.push_back(static_cast<uint32_t>(m_dynamicUniformBufferIndices.size()));
			}
		}
		void RenderObject::InitGeometriesPushConstants(uint32_t constantSize, uint32_t constantsNumber, void* data)
		{
			m_sizeofConstant = constantSize;
			m_constantsNumber = constantsNumber;
			_geometriesPushConstants = new char[constantSize * constantsNumber];
			memcpy(_geometriesPushConstants, data, constantSize * constantsNumber);
		}

		void RenderObject::Draw(VkCommandBuffer commandBuffer, uint32_t swapchainImageIndex, render::VulkanPipeline* currentPipeline, render::VulkanDescriptorSet* currentDescriptorSet)
		{
			if (m_geometries.empty())
				return;
			bool is_visible = false;
			if (m_boundingBoxes.empty()) is_visible = true;
			for (int i = 0; i < m_boundingBoxes.size(); i++)
			{
				is_visible |= m_boundingBoxes[i]->IsVisible();
			}
			if (!is_visible)
				return;


			uint32_t vip = _vertexLayout->GetVertexInputBinding(VK_VERTEX_INPUT_RATE_VERTEX);//TODO can we add this info for each geometry?
			uint32_t iip = _vertexLayout->GetVertexInputBinding(VK_VERTEX_INPUT_RATE_INSTANCE);

			if (_pipeline != currentPipeline)
				_pipeline->Draw(commandBuffer);

			if (m_dynamicUniformBufferIndices.empty() && m_descriptorSets[swapchainImageIndex] != currentDescriptorSet)
				m_descriptorSets[swapchainImageIndex]->Draw(commandBuffer, _pipeline->getPipelineLayout(), 0);

			for (uint32_t j = 0; j < m_geometries.size();j++)
			{
				if(_geometriesPushConstants != nullptr)
					vkCmdPushConstants(commandBuffer, _pipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, m_sizeofConstant, _geometriesPushConstants + (m_sizeofConstant * j));
				if (!m_boundingBoxes.empty() && (m_boundingBoxes.size() - 1) >= j && !m_boundingBoxes[j]->IsVisible()) continue;//TODO remove hardcode
				if (!m_dynamicUniformBufferIndices.empty()) //TODO see how can we add the current descriptor comparison
					m_descriptorSets[swapchainImageIndex]->Draw(commandBuffer, _pipeline->getPipelineLayout(), m_dynamicUniformBufferIndices[j]);

				m_geometries[j]->Draw(&commandBuffer, vip, iip);
			}
		}

		bool RenderObject::LoadGeometry(const std::string& filename, render::VertexLayout* vertexLayout, float scale, int instanceNo, glm::vec3 atPos)
		{
			return false;
		}

		void RenderObject::AdoptGeometriesFrom(const RenderObject& t)
		{
			for (auto geo : t.m_geometries)
			{
				Geometry* mygeo = new Geometry;
				*mygeo = *geo;
				m_geometries.push_back(mygeo);
			}
		}

		void RenderObject::ComputeTangents(glm::vec3 pos1, glm::vec3 pos2, glm::vec3 pos3, glm::vec2 uv1, glm::vec2 uv2, glm::vec2 uv3, glm::vec3& tangent1, glm::vec3& bitangent1)
		{
			glm::vec3 edge1 = pos2 - pos1;
			glm::vec3 edge2 = pos3 - pos1;
			glm::vec2 deltaUV1 = uv2 - uv1;
			glm::vec2 deltaUV2 = uv3 - uv1;

			float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

			tangent1.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
			tangent1.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
			tangent1.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
			tangent1 = glm::normalize(tangent1);

			bitangent1.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
			bitangent1.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
			bitangent1.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
			bitangent1 = glm::normalize(bitangent1);
		}
	}
}