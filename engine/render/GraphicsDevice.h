#pragma once
#include "VertexLayout.h"
#include "Buffer.h"
#include "Texture.h"
#include "DescriptorPool.h"
#include "DescriptorSetLayout.h"
#include "DescriptorSet.h"
#include "RenderPass.h"
#include "Pipeline.h"
#include "CommandBuffer.h"
#include "Mesh.h"
#include <vector>

namespace engine
{
	namespace render
	{
		class GraphicsDevice
		{
		protected:
			std::vector<Buffer*> m_buffers;
			std::vector<Texture*> m_textures;
			std::vector<Mesh*> m_meshes;
			std::vector<DescriptorSetLayout*> m_descriptorSetLayouts;
			std::vector<DescriptorPool*> m_descriptorPools;
			std::vector<DescriptorSet*> m_descriptorSets;
			std::vector<RenderPass*> m_renderPasses;
			std::vector<Pipeline*> m_pipelines;
			std::vector<CommandBuffer*> m_commandBuffers;
			
		public:
			virtual ~GraphicsDevice();

			virtual Buffer* GetUniformBuffer(size_t size, void* data, DescriptorPool* descriptorPool) = 0;

			virtual Texture* GetTexture(TextureData* data, DescriptorPool *descriptorPool, CommandBuffer* commandBuffer) = 0;

			virtual Texture* GetRenderTarget(uint32_t width, uint32_t height, GfxFormat format, DescriptorPool* srvDescriptorPool, DescriptorPool* rtvDescriptorPool, CommandBuffer* commandBuffer, bool depthBuffer) = 0;

			virtual DescriptorPool* GetDescriptorPool(std::vector<DescriptorPoolSize> poolSizes, uint32_t maxSets) = 0;

			virtual DescriptorSetLayout* GetDescriptorSetLayout(std::vector<LayoutBinding> bindings) = 0;

			virtual DescriptorSet* GetDescriptorSet(DescriptorSetLayout* layout, std::vector<Buffer*> buffers, std::vector <Texture*> textures) = 0;

			virtual RenderPass* GetRenderPass(uint32_t width, uint32_t height, Texture *colorTexture, Texture *depthTexture) = 0;

			virtual Pipeline* GetPipeLine(std::string vertexFileName, std::string vertexEntry, std::string fragmentFilename, std::string fragmentEntry, VertexLayout* vertexLayout, DescriptorSetLayout* descriptorSetlayout, PipelineProperties properties, RenderPass* renderPass) = 0;

			virtual CommandBuffer* GetCommandBuffer() = 0;

			virtual Mesh* GetMesh(MeshData* data, VertexLayout* vlayout, CommandBuffer* commanBuffer) = 0;
		};
	}
}