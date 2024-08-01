#pragma once
#include <map>
#include "Terrain.h"
#include "render/VulkanDevice.h"

namespace engine
{
	namespace scene
	{	
		class TerrainUVSphere : public Terrain
		{
			struct {
				glm::mat4 modelView;
			} uboVS;
			render::VulkanBuffer* uniformBufferVS = nullptr;
			render::VulkanBuffer* uniformBufferFS = nullptr;

			float m_radius = 0.0f;

		public:
			virtual uint32_t* BuildPatchIndices(int offsetX, int offsetY, int width, int heights, int& size);

			void Init(const std::string& filename, float radius, render::VulkanDevice* vulkanDevice
				, render::VertexLayout* vertex_layout, render::VulkanBuffer* globalUniformBufferVS, VkDeviceSize vertexUniformBufferSize, VkDeviceSize fragmentUniformBufferSize, std::vector<VkDescriptorImageInfo*> texturesDescriptors
				, std::string vertexShaderFilename
				, std::string fragmentShaderFilename
				, VkRenderPass renderPass
				, VkPipelineCache pipelineCache
				, render::PipelineProperties pipelineProperties
				, VkQueue queue
				, int fallbackRings = 0
				, int fallbackSlices = 0);
			void UpdateUniforms(glm::mat4 &model);
			render::VulkanBuffer* GetVSUniformBuffer() {
				return uniformBufferVS;
			};
			render::VulkanBuffer* GetFSUniformBuffer() {
				return uniformBufferFS;
			};
			float GetRadius() { return m_radius; }
		};
	}
}