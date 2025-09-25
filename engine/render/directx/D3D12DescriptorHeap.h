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
#include "render/DescriptorPool.h"

namespace engine
{
	namespace render
	{
		class D3D12DescriptorHeap : public DescriptorPool
		{
			D3D12_DESCRIPTOR_HEAP_TYPE m_type;
			UINT m_descriptorSize = 0;
			UINT m_maxDescriptors = 0;
			UINT m_usedDescriptors = 0;
		public:
			Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_heap;
			D3D12DescriptorHeap(std::vector<DescriptorPoolSize> poolSizes, uint32_t maxSets) : DescriptorPool(poolSizes, maxSets) {};

			void Create(Microsoft::WRL::ComPtr<ID3D12Device> device);
			void GetAvailableHandles(CD3DX12_CPU_DESCRIPTOR_HANDLE &cpuHandle, CD3DX12_GPU_DESCRIPTOR_HANDLE &gpuHandle);
		};
	}
}