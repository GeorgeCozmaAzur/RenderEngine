#pragma once
#include <map>
#include "RenderObject.h"
#include "render/vulkan/VulkanDevice.h"

namespace engine
{
	namespace scene
	{	
		class DrawDebugTexture : public RenderObject
		{
			render::VulkanVertexLayout m_vertexLayout = render::VulkanVertexLayout({
		render::VERTEX_COMPONENT_POSITION,
		render::VERTEX_COMPONENT_UV
				}, {});
			struct {
				glm::mat4 projection;
				glm::mat4 modelView;
			} uboVS;

			struct {
				glm::mat4 projection;
				glm::mat4 modelView;
				float depth;
			} uboVSdepth;

			bool m_hasDepth = false;

			render::Texture* m_texture = nullptr;
			render::Buffer* uniformBufferVS = nullptr;

		public:
			void SetTexture(render::Texture *texture);
			void Init(render::VulkanDevice* vulkanDevice, render::DescriptorPool* descriptorPool, render::Texture* texture, VkQueue queue, render::RenderPass* renderPass, VkPipelineCache pipelineCache);
			void UpdateUniformBuffers(glm::mat4 projection, glm::mat4 view, float depth = -1);
		};

		class DrawDebugVectors : public RenderObject
		{
			render::VulkanVertexLayout debugVertexLayout = render::VulkanVertexLayout({
			render::VERTEX_COMPONENT_POSITION,
			render::VERTEX_COMPONENT_COLOR
				}, {});

		public:
			render::MeshData* CreateDebugVectorsGeometry(glm::vec3 position, std::vector<glm::vec3> directions, std::vector<glm::vec3> colors);
			void Init(render::VulkanDevice* vulkanDevice, render::DescriptorPool* descriptorPool, render::VulkanBuffer* globalUniformBufferVS, VkQueue queue, render::RenderPass* renderPass, VkPipelineCache pipelineCache);
		};

		class DrawDebugBBs : public RenderObject
		{
			render::VulkanVertexLayout debugVertexLayout = render::VulkanVertexLayout({
			render::VERTEX_COMPONENT_POSITION,
			render::VERTEX_COMPONENT_COLOR
				}, {});

		public:
			void Init(std::vector<std::vector<glm::vec3>> boundries, render::VulkanDevice* vulkanDevice, render::DescriptorPool* descriptorPool, render::Buffer* globalUniformBufferVS, VkQueue queue, render::RenderPass* renderPass, VkPipelineCache pipelineCache, uint32_t constantSize = 0);
		};
	}
}