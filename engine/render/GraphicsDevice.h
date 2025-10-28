#pragma once
#include "VertexLayout.h"
#include "Buffer.h"
#include "Texture.h"
#include "DescriptorPool.h"
#include "DescriptorSetLayout.h"
#include "DescriptorSet.h"
#include "RenderPass.h"
#include "Pipeline.h"
#include "CommandPool.h"
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
			std::vector<Buffer*> m_loadStaggingBuffers;
			std::vector<Texture*> m_textures;
			std::vector<Mesh*> m_meshes;
			std::vector<VertexLayout*> m_vertexLayouts;
			std::vector<DescriptorSetLayout*> m_descriptorSetLayouts;
			std::vector<DescriptorPool*> m_descriptorPools;
			std::vector<DescriptorSet*> m_descriptorSets;
			std::vector<RenderPass*> m_renderPasses;
			std::vector<Pipeline*> m_pipelines;
			std::vector<CommandPool*> m_primaryCommandPools;
			std::vector<CommandPool*> m_secondaryCommandPools;
			
		public:

			virtual char* GetDeviceName()
			{
				return nullptr;
			}

			virtual Buffer* GetUniformBuffer(size_t size, void* data, DescriptorPool* descriptorPool) = 0;

			virtual Texture* GetTexture(TextureData* data, DescriptorPool *descriptorPool, CommandBuffer* commandBuffer) = 0;

			virtual Texture* GetRenderTarget(uint32_t width, uint32_t height, GfxFormat format, DescriptorPool* srvDescriptorPool, DescriptorPool* rtvDescriptorPool, CommandBuffer* commandBuffer) = 0;
			
			virtual Texture* GetDepthRenderTarget(uint32_t width, uint32_t height, GfxFormat format, DescriptorPool* srvDescriptorPool, DescriptorPool* rtvDescriptorPool, CommandBuffer* commandBuffer, bool useInShaders, bool withStencil) = 0;

			virtual DescriptorPool* GetDescriptorPool(std::vector<DescriptorPoolSize> poolSizes, uint32_t maxSets) = 0;

			virtual VertexLayout* GetVertexLayout(std::initializer_list<Component> vComponents, std::initializer_list<Component> iComponents) = 0;

			virtual DescriptorSetLayout* GetDescriptorSetLayout(std::vector<LayoutBinding> bindings) = 0;

			virtual DescriptorSet* GetDescriptorSet(DescriptorSetLayout* layout, DescriptorPool* pool, std::vector<Buffer*> buffers, std::vector <Texture*> textures) = 0;

			virtual RenderPass* GetRenderPass(uint32_t width, uint32_t height, Texture *colorTexture, Texture *depthTexture) = 0;

			virtual Pipeline* GetPipeline(std::string vertexFileName, std::string vertexEntry, std::string fragmentFilename, std::string fragmentEntry, VertexLayout* vertexLayout, DescriptorSetLayout* descriptorSetlayout, PipelineProperties properties, RenderPass* renderPass) = 0;

			virtual CommandPool* GetCommandPool(uint32_t queueIndex, bool primary = true) = 0;
			
			virtual CommandBuffer* GetCommandBuffer(CommandPool *pool, bool primary = true) = 0;

			virtual Mesh* GetMesh(MeshData* data, VertexLayout* vlayout, CommandBuffer* commanBuffer) = 0;

			virtual void UpdateHostVisibleMesh(MeshData* data, Mesh* mesh) = 0;

			void DestroyBuffer(Buffer* buffer);

			void FreeLoadStaggingBuffers();

			virtual ~GraphicsDevice();
		};
	}
}