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

namespace engine
{
	namespace render
	{
		class D3D12DescriptorHeap
		{
			
			D3D12_DESCRIPTOR_HEAP_TYPE m_type;
			UINT m_descriptorSize;
			UINT m_maxDescriptors;
			UINT m_usedDescriptors;
		public:
			Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_heap;
			void Create(Microsoft::WRL::ComPtr<ID3D12Device> device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT maxDescriptors);
			void GetAvailableHandles(CD3DX12_CPU_DESCRIPTOR_HANDLE &cpuHandle, CD3DX12_GPU_DESCRIPTOR_HANDLE &gpuHandle);
		};
	}
}