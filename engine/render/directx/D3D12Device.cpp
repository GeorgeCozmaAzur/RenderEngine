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
			for (auto cmd : m_commandBuffers)
				delete cmd;
		}

		Buffer* D3D12Device::GetUniformBuffer(size_t size, void* data, DescriptorPool* descriptorPool, bool onCPU, CommandBuffer* commandBuffer)
		{
			D3D12UniformBuffer* buffer = new D3D12UniformBuffer();
			D3D12DescriptorHeap* descHeap = dynamic_cast<D3D12DescriptorHeap*>(descriptorPool);
			D3D12CommandBuffer* d3dcmd = dynamic_cast<D3D12CommandBuffer*>(commandBuffer);

			//assert(size <= 256);
			size = (size / 256) * 256 + 256;
			
			if (onCPU)
			{
				buffer->CreateCPUVisible(m_device.Get(), size, data);//TODO make sure we read just what we need
			}
			else
			{
				D3D12Buffer* staggingBuffer = new D3D12Buffer();
				staggingBuffer->Create(m_device.Get(), size, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
				m_loadStaggingBuffers.push_back(staggingBuffer);
				buffer->CreateGPUVisible(m_device.Get(), staggingBuffer->GetD3DBuffer(), d3dcmd->m_commandList.Get(), size, data, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
			}
			//buffer->Create(m_device.Get(), size, data, cbvSrvHandle, cbvSrvHandleGPU);
			CD3DX12_CPU_DESCRIPTOR_HANDLE cbvSrvHandle;
			CD3DX12_GPU_DESCRIPTOR_HANDLE cbvSrvHandleGPU;
			descHeap->GetAvailableHandles(cbvSrvHandle, cbvSrvHandleGPU);			
			buffer->CreateView(m_device.Get(), cbvSrvHandle, cbvSrvHandleGPU);

			m_buffers.push_back(buffer);
			return buffer;
		}

		Buffer* D3D12Device::GetStorageVertexBuffer(size_t size, void* data, size_t vertexSize, DescriptorPool* descriptorPool, bool onCPU, CommandBuffer* commandBuffer)
		{
			D3D12StorageVertexBuffer* buffer = new D3D12StorageVertexBuffer();
			D3D12DescriptorHeap* descHeap = dynamic_cast<D3D12DescriptorHeap*>(descriptorPool);
			D3D12CommandBuffer* d3dcmd = dynamic_cast<D3D12CommandBuffer*>(commandBuffer);

			D3D12Buffer* staggingBuffer = new D3D12Buffer();
			staggingBuffer->Create(m_device.Get(), size, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
			m_loadStaggingBuffers.push_back(staggingBuffer);
			buffer->CreateGPUVisible(m_device.Get(), staggingBuffer->GetD3DBuffer(), d3dcmd->m_commandList.Get(), size, data, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

			CD3DX12_CPU_DESCRIPTOR_HANDLE SrvHandle;
			CD3DX12_GPU_DESCRIPTOR_HANDLE SrvHandleGPU;
			descHeap->GetAvailableHandles(SrvHandle, SrvHandleGPU);
			CD3DX12_CPU_DESCRIPTOR_HANDLE UAVHandle;
			CD3DX12_GPU_DESCRIPTOR_HANDLE UAVHandleGPU;
			descHeap->GetAvailableHandles(UAVHandle, UAVHandleGPU);
			buffer->CreateView(vertexSize, m_device.Get(), SrvHandle, SrvHandleGPU, UAVHandle, UAVHandleGPU);

			m_buffers.push_back(buffer);
			return buffer;
		}

		Texture* D3D12Device::GetTexture(TextureData* data, DescriptorPool* descriptorPool, CommandBuffer* commandBuffer, bool generateMipmaps)
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

		Texture* D3D12Device::GetRenderTarget(uint32_t width, uint32_t height, GfxFormat format, DescriptorPool* srvDescriptorPool, DescriptorPool* rtvDescriptorPool, CommandBuffer* commandBuffer, float* clearValues)
		{
			D3D12RenderTarget* texture = new D3D12RenderTarget();

			if (clearValues)
			{
				memcpy(texture->m_ClearColor, clearValues, sizeof(float)*4);//TODO a little unsafe
			}

			texture->Create(m_device.Get(), width, height, format, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
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

		Texture* D3D12Device::GetDepthRenderTarget(uint32_t width, uint32_t height, GfxFormat format, DescriptorPool* srvDescriptorPool, DescriptorPool* rtvDescriptorPool, CommandBuffer* commandBuffer, bool useInShaders, bool withStencil)
		{
			D3D12RenderTarget* texture = new D3D12RenderTarget();

			texture->Create(m_device.Get(), width, height, format, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, useInShaders ? D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE : D3D12_RESOURCE_STATE_DEPTH_WRITE);
			D3D12DescriptorHeap* descHeap = dynamic_cast<D3D12DescriptorHeap*>(srvDescriptorPool);
			CD3DX12_CPU_DESCRIPTOR_HANDLE cbvSrvHandle{};
			CD3DX12_GPU_DESCRIPTOR_HANDLE cbvSrvHandleGPU{};
			CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle{};
			if (descHeap && useInShaders)
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

		VertexLayout* D3D12Device::GetVertexLayout(std::initializer_list<Component> vComponents, std::initializer_list<Component> iComponents)
		{
			VertexLayout* vlayout = new VertexLayout(vComponents, iComponents);
			m_vertexLayouts.push_back(vlayout);
			return vlayout;
		}

		DescriptorSetLayout* D3D12Device::GetDescriptorSetLayout(std::vector<LayoutBinding> bindings)
		{
			DescriptorSetLayout* layout = new DescriptorSetLayout(bindings);
			m_descriptorSetLayouts.push_back(layout);
			return layout;
		}

		DescriptorSet* D3D12Device::GetDescriptorSet(DescriptorSetLayout* layout, DescriptorPool* pool, std::vector<Buffer*> buffers, std::vector <Texture*> textures, size_t dynamicAlignment)
		{
			D3D12DescriptorTable* table = new D3D12DescriptorTable();
			std::vector <CD3DX12_GPU_DESCRIPTOR_HANDLE> bufferHandles(buffers.size());
			std::vector <CD3DX12_GPU_DESCRIPTOR_HANDLE> textureHandles(textures.size());
			UINT buffer_index = 0;
			for (UINT i = 0; i < layout->m_bindings.size(); i++)
			{
				if (layout->m_bindings[i].descriptorType == UNIFORM_BUFFER)
				{
					bufferHandles[buffer_index] = dynamic_cast<D3D12UniformBuffer*>(buffers[buffer_index])->m_GPUHandle; buffer_index++;
				}else
				if (layout->m_bindings[i].descriptorType == INPUT_STORAGE_BUFFER)
				{
					bufferHandles[buffer_index] = dynamic_cast<D3D12StorageVertexBuffer*>(buffers[buffer_index])->m_srvGPUHandle; buffer_index++;
				}else
				if (layout->m_bindings[i].descriptorType == OUTPUT_STORAGE_BUFFER)
				{
					bufferHandles[buffer_index] = dynamic_cast<D3D12StorageVertexBuffer*>(buffers[buffer_index])->m_uavGPUHandle; buffer_index++;
				}
				
			}
			/*for (int i = 0; i < buffers.size(); i++)
			{
				bufferHandles[i] = dynamic_cast<D3D12UniformBuffer*>(buffers[i])->m_GPUHandle;
			}*/
			for (int i = 0; i < textures.size(); i++)
			{
				textureHandles[i] = dynamic_cast<D3D12Texture*>(textures[i])->m_GPUHandle;
			}
			table->Create(layout, bufferHandles, textureHandles);
			m_descriptorSets.push_back(table);
			return table;
		}

		RenderPass* D3D12Device::GetRenderPass(uint32_t width, uint32_t height, std::vector<Texture*> colorTextures, Texture* depthTexture, std::vector<RenderSubpass> subpasses)
		{
			D3D12RenderPass* pass = new D3D12RenderPass();
			std::vector<DXGI_FORMAT> rtvFormats(colorTextures.size());
			std::vector <CD3DX12_CPU_DESCRIPTOR_HANDLE> rtvHandles(colorTextures.size());
			std::vector <ID3D12Resource*> rtvColorTextures(colorTextures.size());
			//pass->m_clearColors.resize(colorTextures.size());
			
			D3D12RenderTarget* depthRT = dynamic_cast<D3D12RenderTarget*>(depthTexture);
			for (int i = 0; i < colorTextures.size(); i++)
			{
				D3D12RenderTarget* colorRT = dynamic_cast<D3D12RenderTarget*>(colorTextures[i]);
				rtvFormats[i] = colorRT->m_dxgiFormat;
				rtvHandles[i] = colorRT->m_CPURTVHandle;
				rtvColorTextures[i] = colorRT->m_texture.Get();
				pass->m_clearColors.push_back({ colorRT->m_ClearColor[0], colorRT->m_ClearColor[1], colorRT->m_ClearColor[2], colorRT->m_ClearColor[3] });
			}
			pass->Create(width, height, rtvFormats, depthRT->m_dxgiFormat, 
				{ { rtvHandles, rtvColorTextures, depthRT->m_CPURTVHandle, depthRT->m_texture.Get() } });
			if (depthRT->m_CPUHandle.ptr != 0)
			{
				pass->depthInShaders = true;
			}
			if(subpasses.size() > 0)
			pass->SetSubPasses(subpasses);
			m_renderPasses.push_back(pass);
			return pass;
		}

		Pipeline* D3D12Device::GetPipeline(std::string vertexFileName, std::string vertexEntry, std::string fragmentFilename, std::string fragmentEntry, VertexLayout* vertexLayout, DescriptorSetLayout* descriptorSetlayout, PipelineProperties properties, RenderPass* renderPass)
		{
			D3D12RenderPass* d3dRenderPass = dynamic_cast<D3D12RenderPass*>(renderPass);
			D3D12Pipeline* pipeline = new D3D12Pipeline();
			std::wstring ws(vertexFileName.begin(), vertexFileName.end());
			pipeline->Load(m_device.Get(), ws, vertexEntry, fragmentEntry, vertexLayout, descriptorSetlayout, properties, d3dRenderPass);
			m_pipelines.push_back(pipeline);
			return pipeline;
		}

		Pipeline* D3D12Device::GetComputePipeline(std::string computeFileName, std::string computeEntry, DescriptorSetLayout* descriptorSetlayout, uint32_t constanBlockSize)
		{
			D3D12Pipeline* pipeline = new D3D12Pipeline();
			std::wstring ws(computeFileName.begin(), computeFileName.end());
			pipeline->LoadCompute(m_device.Get(), ws, descriptorSetlayout, constanBlockSize);
			m_pipelines.push_back(pipeline);
			return pipeline;
		}

		CommandPool* D3D12Device::GetCommandPool(uint32_t queueIndex, bool primary)
		{
			return nullptr;
		}

		CommandBuffer* D3D12Device::GetCommandBuffer(CommandPool* pool, bool primary)
		{
			D3D12CommandBuffer* commandBuffer = new D3D12CommandBuffer(pool);
			commandBuffer->Create(m_device.Get());
			m_commandBuffers.push_back(commandBuffer);
			return commandBuffer;
			//return pool->GetCommandBuffer();
		}

		Mesh* D3D12Device::GetMesh(MeshData* data, VertexLayout* vlayout, CommandBuffer* commandBuffer)
		{
			D3D12Mesh* mesh = new D3D12Mesh();
			D3D12CommandBuffer* d3dcommandBuffer = dynamic_cast<D3D12CommandBuffer*>(commandBuffer);

			//mesh->Create(m_device.Get(),data,vlayout, d3dcommandBuffer->m_commandList.Get());
			const UINT vertexBufferSize = data->m_vertexCount * vlayout->GetVertexSize(0);
			if (vertexBufferSize)
			{
				D3D12Buffer* staggingBuffer = new D3D12Buffer();
				staggingBuffer->Create(m_device.Get(), vertexBufferSize, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
				m_loadStaggingBuffers.push_back(staggingBuffer);
				mesh->_vertexBuffer = new D3D12VertexBuffer();
				mesh->_vertexBuffer->CreateGPUVisible(m_device.Get(), staggingBuffer->GetD3DBuffer(), d3dcommandBuffer->m_commandList.Get(), vertexBufferSize, data->m_vertices, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
				//mesh->_vertexBuffer->CreateCPUVisible(m_device.Get(), vertexBufferSize, data->m_vertices);// , D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
				mesh->_vertexBuffer->CreateView(vlayout->GetVertexSize(0));
				m_buffers.push_back(mesh->_vertexBuffer);
			}

			const UINT indexBufferSize = data->m_indexCount * sizeof(UINT);
			if (indexBufferSize)
			{
				D3D12Buffer* staggingBuffer = new D3D12Buffer();
				staggingBuffer->Create(m_device.Get(), indexBufferSize, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
				m_loadStaggingBuffers.push_back(staggingBuffer);
				mesh->m_indexSize = data->m_indexSize;
				mesh->_indexBuffer = new D3D12IndexBuffer();
				mesh->_indexBuffer->CreateGPUVisible(m_device.Get(), staggingBuffer->GetD3DBuffer(), d3dcommandBuffer->m_commandList.Get(), indexBufferSize, data->m_indices, D3D12_RESOURCE_STATE_INDEX_BUFFER);
				mesh->_indexBuffer->CreateView(mesh->m_indexSize);
				m_buffers.push_back(mesh->_indexBuffer);
			}

			if (data->m_instanceBufferSize > 0)
			{
				mesh->_instanceBuffer = new D3D12VertexBuffer();
				mesh->_instanceBuffer->CreateCPUVisible(m_device.Get(), data->m_instanceBufferSize, data->_instanceExternalData);
				mesh->_instanceBuffer->CreateView(vlayout->GetVertexSize(1));
				mesh->m_instanceNo = data->m_instanceNo;
				m_buffers.push_back(mesh->_instanceBuffer);
			}

			m_meshes.push_back(mesh);

			return mesh;
		}

		void D3D12Device::UpdateHostVisibleMesh(MeshData* data, Mesh* mesh)
		{
			render::D3D12Mesh* vkmesh = dynamic_cast<render::D3D12Mesh*>(mesh);

			if (vkmesh->m_vertexCount != data->m_vertexCount)
			{
				if (vkmesh->_vertexBuffer && vkmesh->_vertexBuffer->GetSize() < data->m_verticesSize)
				{
					DestroyBuffer(vkmesh->_vertexBuffer);
					vkmesh->_vertexBuffer = nullptr;
				}
				vkmesh->m_vertexCount = data->m_vertexCount;
				if (!vkmesh->_vertexBuffer)
				{
					vkmesh->_vertexBuffer = new D3D12VertexBuffer();
					vkmesh->_vertexBuffer->CreateCPUVisible(m_device.Get(), data->m_verticesSize, data->m_vertices);
					vkmesh->_vertexBuffer->CreateView(data->m_verticesSize / vkmesh->m_vertexCount);
					m_buffers.push_back(vkmesh->_vertexBuffer);
				}
			}
			if (vkmesh->m_indexCount < data->m_indexCount)
			{
				if (vkmesh->_indexBuffer)
				{
					DestroyBuffer(vkmesh->_indexBuffer);
					vkmesh->_indexBuffer = nullptr;
				}

				vkmesh->m_indexCount = data->m_indexCount;
				vkmesh->m_indexSize = data->m_indexSize;
				if (!vkmesh->_indexBuffer)
				{
					vkmesh->_indexBuffer = new D3D12IndexBuffer();
					vkmesh->_indexBuffer->CreateCPUVisible(m_device.Get(), data->m_indexCount * data->m_indexSize, data->m_indices);
					vkmesh->_indexBuffer->CreateView(vkmesh->m_indexSize);
					m_buffers.push_back(vkmesh->_indexBuffer);
				}
			}
		}
	}
}
