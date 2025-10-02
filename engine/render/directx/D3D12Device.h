#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <string>
#include <wrl.h>
#include <shellapi.h>
#include <vector>
#include "render/directx/d3dx12.h"
#include "render/GraphicsDevice.h"

namespace engine
{
	namespace render
	{
		class D3D12Device : public GraphicsDevice
		{
			Microsoft::WRL::ComPtr<ID3D12Device> m_device;

		public:
			D3D12Device(Microsoft::WRL::ComPtr<ID3D12Device> device) : m_device(device) {};

			~D3D12Device();

			virtual Buffer* GetUniformBuffer(size_t size, void* data, DescriptorPool* descriptorPool);

			virtual Texture* GetTexture(TextureData* data, DescriptorPool* descriptorPool, CommandBuffer* commandBuffer);

			virtual Texture* GetRenderTarget(uint32_t width, uint32_t height, GfxFormat format, DescriptorPool* srvDescriptorPool, DescriptorPool* rtvDescriptorPool, CommandBuffer* commandBuffer);
			
			virtual Texture* GetDepthRenderTarget(uint32_t width, uint32_t height, GfxFormat format, DescriptorPool* srvDescriptorPool, DescriptorPool* rtvDescriptorPool, CommandBuffer* commandBuffer, bool useInShaders, bool withStencil);

			virtual DescriptorPool* GetDescriptorPool(std::vector<DescriptorPoolSize> poolSizes, uint32_t maxSets);

			virtual VertexLayout* GetVertexLayout(std::initializer_list<Component> vComponents, std::initializer_list<Component> iComponents);
			
			virtual DescriptorSetLayout* GetDescriptorSetLayout(std::vector<LayoutBinding> bindings);

			virtual DescriptorSet* GetDescriptorSet(DescriptorSetLayout* layout, DescriptorPool* pool, std::vector<Buffer*> buffers, std::vector <Texture*> textures);

			virtual RenderPass* GetRenderPass(uint32_t width, uint32_t height, Texture* colorTexture, Texture* depthTexture);

			virtual Pipeline* GetPipeLine(std::string vertexFileName, std::string vertexEntry, std::string fragmentFilename, std::string fragmentEntry, VertexLayout* vertexLayout, DescriptorSetLayout* descriptorSetlayout, PipelineProperties properties, RenderPass* renderPass);

			virtual CommandBuffer* GetCommandBuffer();

			virtual Mesh* GetMesh(MeshData* data, VertexLayout* vlayout, CommandBuffer* commanBuffer);
		};
	}
}