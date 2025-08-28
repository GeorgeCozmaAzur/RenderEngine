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
			render::VertexLayout m_vertexLayout = render::VertexLayout({
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

			render::VulkanTexture* m_texture = nullptr;
			render::VulkanBuffer* uniformBufferVS = nullptr;

		public:
			void SetTexture(render::VulkanTexture *texture);
			void Init(render::VulkanDevice * vulkanDevice, VkDescriptorPool descriptorPool, render::VulkanTexture* texture, VkQueue queue, VkRenderPass renderPass, VkPipelineCache pipelineCache);
			void UpdateUniformBuffers(glm::mat4 projection, glm::mat4 view, float depth = -1);
		};

		class DrawDebugVectors : public RenderObject
		{
			render::VertexLayout debugVertexLayout = render::VertexLayout({
			render::VERTEX_COMPONENT_POSITION,
			render::VERTEX_COMPONENT_COLOR
				}, {});

		public:
			void CreateDebugVectorsGeometry(glm::vec3 position, std::vector<glm::vec3> directions, std::vector<glm::vec3> colors);
			void Init(render::VulkanDevice* vulkanDevice, VkDescriptorPool descriptorPool, render::VulkanBuffer* globalUniformBufferVS, VkQueue queue, VkRenderPass renderPass, VkPipelineCache pipelineCache);
		};

		class DrawDebugBBs : public RenderObject
		{
			render::VertexLayout debugVertexLayout = render::VertexLayout({
			render::VERTEX_COMPONENT_POSITION,
			render::VERTEX_COMPONENT_COLOR
				}, {});

		public:
			void Init(std::vector<std::vector<glm::vec3>> boundries, render::VulkanDevice* vulkanDevice, VkDescriptorPool descriptorPool, render::VulkanBuffer* globalUniformBufferVS, VkQueue queue, VkRenderPass renderPass, VkPipelineCache pipelineCache, uint32_t constantSize = 0);
		};
	}
}