#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <string>
#include <wrl.h>
#include <shellapi.h>
#include <vector>
#include "Buffer.h"
#include "render/directx/d3dx12.h"

namespace engine
{
	namespace render
	{
		class D3D12Buffer : public Buffer
		{
		protected:
			Microsoft::WRL::ComPtr<ID3D12Resource> m_buffer;
			Microsoft::WRL::ComPtr<ID3D12Resource> m_stagingBuffer;
		public:
			void CreateGPUVisible(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, size_t size, void* data, D3D12_RESOURCE_STATES finalState);
			void FreeStagingBuffer();
		};

		class D3D12UniformBuffer : public D3D12Buffer
		{
			CD3DX12_CPU_DESCRIPTOR_HANDLE m_CPUHandle;

		public:
			CD3DX12_GPU_DESCRIPTOR_HANDLE m_GPUHandle;
		
			void Create(ID3D12Device* device, size_t size, void* data, CD3DX12_CPU_DESCRIPTOR_HANDLE cupHandle, CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle);
		};

		class D3D12VertexBuffer : public D3D12Buffer
		{
		public:
			D3D12_VERTEX_BUFFER_VIEW m_view;
		
			void CreateView(UINT vertexSize);
		};

		class D3D12IndexBuffer : public D3D12Buffer
		{
		public:
			D3D12_INDEX_BUFFER_VIEW m_view;
			UINT m_numIndices;
		
			void CreateView();
		};
	}
}