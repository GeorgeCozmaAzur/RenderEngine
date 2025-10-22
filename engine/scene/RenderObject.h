#pragma once
#include <map>
#include "render/vulkan/VulkanDescriptorSet.h"
#include "render/vulkan/VulkanPipeline.h"
#include "render/vulkan/VulkanDescriptorSetLayout.h"
#include "render/vulkan/VulkanVertexLayout.h"
#include "BoundingObject.h"
#include "render/vulkan/VulkanBuffer.h"
#include "CommandBuffer.h"
#include "scene/Geometry.h"
#include "render/Mesh.h"

namespace engine
{
	namespace scene
	{
		class RenderObject
		{
		public:

			render::VertexLayout* _vertexLayout = nullptr;
			render::DescriptorSetLayout* _descriptorLayout = nullptr;
			render::Pipeline* _pipeline = nullptr;
			std::vector <render::DescriptorSet*> m_descriptorSets;
			std::vector<render::MeshData*> m_meshesData;
			std::vector<render::Mesh*> m_geometries;
			std::vector<uint32_t> m_dynamicUniformBufferIndices;

			std::vector<BoundingBox*> m_boundingBoxes;//TODO should be in the scene manager
			std::vector<Geometry*> m_boundingBoxesGeometries;

			char* _geometriesPushConstants = nullptr;
			uint32_t m_sizeofConstant;
			uint32_t m_constantsNumber;

			virtual ~RenderObject()
			{
				//for (auto geo : m_geometries)delete geo;
				for (auto box : m_boundingBoxes)delete box;
				if (_geometriesPushConstants)
				{
					delete []_geometriesPushConstants;
					_geometriesPushConstants = nullptr;
				}
			};

			RenderObject& operator = (const RenderObject& t);

			virtual std::vector<render::MeshData*> LoadGeometry(const std::string& filename, render::VertexLayout* vertexLayout, float scale = 1.0f, int instanceNo = 1, glm::vec3 atPos = glm::vec3(0.0f));
			void AdoptGeometriesFrom(const RenderObject& t);
			void ComputeTangents(glm::vec3 pos1, glm::vec3 pos2, glm::vec3 pos3, glm::vec2 uv1, glm::vec2 uv2, glm::vec2 uv3, glm::vec3& tangent1, glm::vec3& bitangent1);
			void SetVertexLayout(render::VertexLayout* vlayout) { _vertexLayout = vlayout; };
			void SetDescriptorSetLayout(render::DescriptorSetLayout* val) { _descriptorLayout = val; }
			void AddPipeline(render::Pipeline* pipeline);
			void AddDescriptor(render::DescriptorSet*);
			void AddGeometry(render::Mesh*);

			void PopulateDynamicUniformBufferIndices();
			void InitGeometriesPushConstants(uint32_t constantSize, uint32_t constantsNumber, void *data);

			bool IsSimilar(RenderObject* another) { return _pipeline == another->_pipeline /*|| m_descriptorSet == another->m_descriptorSet*/; };

			void Draw(render::CommandBuffer* commandBuffer, uint32_t swapchainImageIndex = 0, render::Pipeline* currentPipeline = nullptr, render::DescriptorSet* currentDescriptorSet = nullptr);
		};
	}
}