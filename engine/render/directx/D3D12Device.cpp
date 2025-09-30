#include "D3D12Device.h"
#include "D3D12Buffer.h"
#include "D3D12Texture.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12DescriptorTable.h"
#include "D3D12CommandBuffer.h"
#include "D3D12RenderPass.h"
#include "D3D12Pipeline.h"
#include "D3D12Mesh.h"
#include "DXSampleHelper.h"

namespace engine
{
	namespace render
	{
		D3D12Device::~D3D12Device()
		{

		}
		Buffer* D3D12Device::GetUniformBuffer(size_t size, void* data, DescriptorPool* descriptorPool)
		{
			D3D12UniformBuffer* buffer = new D3D12UniformBuffer();
			D3D12DescriptorHeap* descHeap = dynamic_cast<D3D12DescriptorHeap*>(descriptorPool);
			CD3DX12_CPU_DESCRIPTOR_HANDLE cbvSrvHandle;
			CD3DX12_GPU_DESCRIPTOR_HANDLE cbvSrvHandleGPU;
			descHeap->GetAvailableHandles(cbvSrvHandle, cbvSrvHandleGPU);
			buffer->Create(m_device.Get(), size, data, cbvSrvHandle, cbvSrvHandleGPU);

			m_buffers.push_back(buffer);
			return buffer;
		}

		Texture* D3D12Device::GetTexture(TextureData* data, DescriptorPool* descriptorPool, CommandBuffer* commandBuffer)
		{
			D3D12Texture* texture = new D3D12Texture();

			texture->Create(m_device.Get(), data->m_width, data->m_height, data->m_format, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST);

			D3D12CommandBuffer* d3dcommandBuffer = dynamic_cast<D3D12CommandBuffer*>(commandBuffer);
			D3D12Buffer* staggingBuffer = new D3D12Buffer();
			staggingBuffer->Create(m_device.Get(), texture->GetSize(), D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
			m_loadStaggingBuffers.push_back(staggingBuffer);
			texture->Upload(staggingBuffer->GetD3DBuffer(), d3dcommandBuffer->m_commandList.Get(), data->m_extents, data->m_ram_data);
			D3D12DescriptorHeap* descHeap = dynamic_cast<D3D12DescriptorHeap*>(descriptorPool);
			CD3DX12_CPU_DESCRIPTOR_HANDLE cbvSrvHandle;
			CD3DX12_GPU_DESCRIPTOR_HANDLE cbvSrvHandleGPU;
			descHeap->GetAvailableHandles(cbvSrvHandle, cbvSrvHandleGPU);
			texture->CreateDescriptor(m_device.Get(), cbvSrvHandle, cbvSrvHandleGPU);

			m_textures.push_back(texture);
			return texture;
		}

		Texture* D3D12Device::GetRenderTarget(uint32_t width, uint32_t height, GfxFormat format, DescriptorPool* srvDescriptorPool, DescriptorPool* rtvDescriptorPool, CommandBuffer* commandBuffer, bool depthBuffer)
		{
			D3D12RenderTarget* texture = new D3D12RenderTarget();

			texture->Create(m_device.Get(), width, height, format, depthBuffer ? D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL : D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, depthBuffer? D3D12_RESOURCE_STATE_DEPTH_WRITE : D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			D3D12DescriptorHeap* descHeap = dynamic_cast<D3D12DescriptorHeap*>(srvDescriptorPool);
			CD3DX12_CPU_DESCRIPTOR_HANDLE cbvSrvHandle{};
			CD3DX12_GPU_DESCRIPTOR_HANDLE cbvSrvHandleGPU{};
			CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle{};
			if(descHeap)
			descHeap->GetAvailableHandles(cbvSrvHandle, cbvSrvHandleGPU);
			D3D12DescriptorHeap* rtvHeap = dynamic_cast<D3D12DescriptorHeap*>(rtvDescriptorPool);
			rtvHeap->GetAvailableCPUHandle(rtvHandle);
			texture->CreateDescriptor(m_device.Get(), cbvSrvHandle, cbvSrvHandleGPU, rtvHandle);

			m_textures.push_back(texture);
			return texture;
		}

		DescriptorPool* D3D12Device::GetDescriptorPool(std::vector<DescriptorPoolSize> poolSizes, uint32_t maxSets)
		{
			D3D12DescriptorHeap* pool = new D3D12DescriptorHeap(poolSizes, maxSets);
			pool->Create(m_device.Get());
			m_descriptorPools.push_back(pool);
			return pool;
		}

		DescriptorSetLayout* D3D12Device::GetDescriptorSetLayout(std::vector<LayoutBinding> bindings)
		{
			DescriptorSetLayout* layout = new DescriptorSetLayout(bindings);
			m_descriptorSetLayouts.push_back(layout);
			return layout;
		}

		DescriptorSet* D3D12Device::GetDescriptorSet(DescriptorSetLayout* layout, std::vector<Buffer*> buffers, std::vector <Texture*> textures)
		{
			D3D12DescriptorTable* table = new D3D12DescriptorTable();
			std::vector <CD3DX12_GPU_DESCRIPTOR_HANDLE> bufferHandles(buffers.size());
			std::vector <CD3DX12_GPU_DESCRIPTOR_HANDLE> textureHandles(textures.size());
			for (int i = 0; i < buffers.size(); i++)
			{
				bufferHandles[i] = dynamic_cast<D3D12UniformBuffer*>(buffers[i])->m_GPUHandle;
			}
			for (int i = 0; i < textures.size(); i++)
			{
				textureHandles[i] = dynamic_cast<D3D12Texture*>(textures[i])->m_GPUHandle;
			}
			table->Create(layout, bufferHandles, textureHandles);
			m_descriptorSets.push_back(table);
			return table;
		}

		RenderPass* D3D12Device::GetRenderPass(uint32_t width, uint32_t height, Texture* colorTexture, Texture* depthTexture)
		{
			D3D12RenderPass* pass = new D3D12RenderPass();
			D3D12RenderTarget* colorRT = dynamic_cast<D3D12RenderTarget*>(colorTexture);
			pass->Create(width, height, { { colorRT->m_CPURTVHandle, dynamic_cast<D3D12RenderTarget*>(depthTexture)->m_CPURTVHandle, colorRT->m_texture.Get() } });
			m_renderPasses.push_back(pass);
			return pass;
		}

		Pipeline* D3D12Device::GetPipeLine(std::string vertexFileName, std::string vertexEntry, std::string fragmentFilename, std::string fragmentEntry, VertexLayout* vertexLayout, DescriptorSetLayout* descriptorSetlayout, PipelineProperties properties, RenderPass* renderPass)
		{
			D3D12Pipeline* pipeline = new D3D12Pipeline();
			std::wstring ws(vertexFileName.begin(), vertexFileName.end());
			pipeline->Load(m_device.Get(), ws, vertexEntry,fragmentEntry, descriptorSetlayout, properties);
			m_pipelines.push_back(pipeline);
			return pipeline;
		}

		CommandBuffer* D3D12Device::GetCommandBuffer()
		{
			D3D12CommandBuffer* commandBuffer = new D3D12CommandBuffer();
			commandBuffer->Create(m_device.Get());
			m_commandBuffers.push_back(commandBuffer);
			return commandBuffer;
		}

		Mesh* D3D12Device::GetMesh(MeshData* data, VertexLayout* vlayout, CommandBuffer* commandBuffer)
		{
			D3D12Mesh* mesh = new D3D12Mesh();
			D3D12CommandBuffer* d3dcommandBuffer = dynamic_cast<D3D12CommandBuffer*>(commandBuffer);

			//mesh->Create(m_device.Get(),data,vlayout, d3dcommandBuffer->m_commandList.Get());
			const UINT vertexBufferSize = data->m_vertexCount * vlayout->GetVertexSize(0);
			D3D12Buffer* staggingBuffer = new D3D12Buffer();
			staggingBuffer->Create(m_device.Get(), vertexBufferSize, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
			m_loadStaggingBuffers.push_back(staggingBuffer);
			mesh->m_vertexBuffer.CreateGPUVisible(m_device.Get(), staggingBuffer->GetD3DBuffer(), d3dcommandBuffer->m_commandList.Get(), vertexBufferSize, data->m_vertices, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
			mesh->m_vertexBuffer.CreateView(vlayout->GetVertexSize(0));

			const UINT indexBufferSize = data->m_indexCount * sizeof(UINT);
			staggingBuffer = new D3D12Buffer();
			staggingBuffer->Create(m_device.Get(), indexBufferSize, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
			m_loadStaggingBuffers.push_back(staggingBuffer);
			mesh->m_indexBuffer.CreateGPUVisible(m_device.Get(), staggingBuffer->GetD3DBuffer(), d3dcommandBuffer->m_commandList.Get(), indexBufferSize, data->m_indices, D3D12_RESOURCE_STATE_INDEX_BUFFER);
			mesh->m_indexBuffer.CreateView();

			m_meshes.push_back(mesh);

			return mesh;
		}
	}
}
