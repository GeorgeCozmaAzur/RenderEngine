#include "D3D12DescriptorHeap.h"
#include "DXSampleHelper.h"

namespace engine
{
	namespace render
	{
		void D3D12DescriptorHeap::Create(Microsoft::WRL::ComPtr<ID3D12Device> device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT maxDescriptors)
		{
			D3D12_DESCRIPTOR_HEAP_DESC rtvoHeapDesc = {};
			rtvoHeapDesc.NumDescriptors = maxDescriptors;
			rtvoHeapDesc.Type = heapType;
			rtvoHeapDesc.Flags = heapType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			ThrowIfFailed(device->CreateDescriptorHeap(&rtvoHeapDesc, IID_PPV_ARGS(&m_heap)));
			m_descriptorSize = device->GetDescriptorHandleIncrementSize(heapType);
			m_usedDescriptors = 0;
		}

		void D3D12DescriptorHeap::GetAvailableHandles(CD3DX12_CPU_DESCRIPTOR_HANDLE& cpuHandle, CD3DX12_GPU_DESCRIPTOR_HANDLE& gpuHandle)
		{
			cpuHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_heap->GetCPUDescriptorHandleForHeapStart(), m_usedDescriptors, m_descriptorSize);
			gpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_heap->GetGPUDescriptorHandleForHeapStart(), m_usedDescriptors, m_descriptorSize);
			m_usedDescriptors++;
		}
	}
}
