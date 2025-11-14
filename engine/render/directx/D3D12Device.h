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

			std::vector<CommandBuffer*> m_commandBuffers;//TODO remove this

			char deviceName[9] = "dxdevice";

		public:
			D3D12Device(Microsoft::WRL::ComPtr<ID3D12Device> device) : m_device(device) {};

			~D3D12Device();

			virtual char* GetDeviceName()
			{
				return deviceName;
			}

			virtual Buffer* GetUniformBuffer(size_t size, void* data, DescriptorPool* descriptorPool, bool onCPU = true, CommandBuffer* commandBuffer = nullptr);

			virtual Buffer* GetStorageVertexBuffer(size_t size, void* data, size_t vertexSize, DescriptorPool* descriptorPool, bool onCPU, CommandBuffer* commandBuffer);

			virtual Texture* GetTexture(TextureData* data, DescriptorPool* descriptorPool, CommandBuffer* commandBuffer, bool generateMipmaps = false);

			virtual Texture* GetRenderTarget(uint32_t width, uint32_t height, GfxFormat format, DescriptorPool* srvDescriptorPool, DescriptorPool* rtvDescriptorPool, CommandBuffer* commandBuffer, float* clearValues = nullptr);
			
			virtual Texture* GetDepthRenderTarget(uint32_t width, uint32_t height, GfxFormat format, DescriptorPool* srvDescriptorPool, DescriptorPool* rtvDescriptorPool, CommandBuffer* commandBuffer, bool useInShaders, bool withStencil);

			virtual DescriptorPool* GetDescriptorPool(std::vector<DescriptorPoolSize> poolSizes, uint32_t maxSets);

			virtual VertexLayout* GetVertexLayout(std::initializer_list<Component> vComponents, std::initializer_list<Component> iComponents);
			
			virtual DescriptorSetLayout* GetDescriptorSetLayout(std::vector<LayoutBinding> bindings);

			virtual DescriptorSet* GetDescriptorSet(DescriptorSetLayout* layout, DescriptorPool* pool, std::vector<Buffer*> buffers, std::vector <Texture*> textures, size_t dynamicAlignment = 0);

			virtual RenderPass* GetRenderPass(uint32_t width, uint32_t height, std::vector<Texture*> colorTexture, Texture* depthTexture, std::vector<RenderSubpass> subpasses = {});

			virtual Pipeline* GetPipeline(std::string vertexFileName, std::string vertexEntry, std::string fragmentFilename, std::string fragmentEntry, VertexLayout* vertexLayout, DescriptorSetLayout* descriptorSetlayout, PipelineProperties properties, RenderPass* renderPass);

			virtual Pipeline* GetComputePipeline(std::string computeFileName, std::string computeEntry, DescriptorSetLayout* descriptorSetlayout, uint32_t constanBlockSize = 0);

			virtual CommandPool* GetCommandPool(uint32_t queueIndex, bool primary = true);

			virtual CommandBuffer* GetCommandBuffer(CommandPool* pool, bool primary = true);

			virtual Mesh* GetMesh(MeshData* data, VertexLayout* vlayout, CommandBuffer* commanBuffer);

			virtual void UpdateHostVisibleMesh(MeshData* data, Mesh* mesh);
		};
	}
}