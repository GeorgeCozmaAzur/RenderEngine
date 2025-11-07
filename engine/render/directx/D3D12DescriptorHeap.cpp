#include "D3D12DescriptorHeap.h"
#include "D3D12CommandBuffer.h"
#include "DXSampleHelper.h"
#include <numeric>
#include <algorithm>

namespace engine
{
	namespace render
	{
		void D3D12DescriptorHeap::Create(Microsoft::WRL::ComPtr<ID3D12Device> device)
		{
			D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};

			int rtv_count = std::count_if(m_poolSizes.begin(), m_poolSizes.end(), [&](const DescriptorPoolSize& ps) {return ps.type == DescriptorType::RTV; });
			int dsv_count = std::count_if(m_poolSizes.begin(), m_poolSizes.end(), [&](const DescriptorPoolSize& ps) {return ps.type == DescriptorType::DSV; });
			if (rtv_count > 0)
			{
				heapDesc.NumDescriptors = std::accumulate(m_poolSizes.begin(), m_poolSizes.end(), 0,
					[](int acc, const auto& thepair) {return thepair.type == DescriptorType::RTV ? acc + thepair.size : 0; });
				heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			}
			else
			if(dsv_count > 0)
			{
				heapDesc.NumDescriptors = dsv_count;
				heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
			}
			else
			{
				heapDesc.NumDescriptors = std::accumulate(m_poolSizes.begin(), m_poolSizes.end(), 0 ,
					[](int acc, const auto& thepair) {return acc + thepair.size; });
				heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			}

			heapDesc.Flags = heapDesc.Type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			ThrowIfFailed(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_heap)));

			m_descriptorSize = device->GetDescriptorHandleIncrementSize(heapDesc.Type);
			m_usedDescriptors = 0;
		}

		void D3D12DescriptorHeap::GetAvailableCPUHandle(CD3DX12_CPU_DESCRIPTOR_HANDLE& cpuHandle)
		{
			cpuHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_heap->GetCPUDescriptorHandleForHeapStart(), m_usedDescriptors, m_descriptorSize);
			m_usedDescriptors++;
		}

		void D3D12DescriptorHeap::GetAvailableHandles(CD3DX12_CPU_DESCRIPTOR_HANDLE& cpuHandle, CD3DX12_GPU_DESCRIPTOR_HANDLE& gpuHandle)
		{
			cpuHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_heap->GetCPUDescriptorHandleForHeapStart(), m_usedDescriptors, m_descriptorSize);
			gpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_heap->GetGPUDescriptorHandleForHeapStart(), m_usedDescriptors, m_descriptorSize);
			m_usedDescriptors++;
		}

		void D3D12DescriptorHeap::Draw(render::CommandBuffer* commandBuffer)
		{
			D3D12CommandBuffer* d3dcommandBuffer = static_cast<D3D12CommandBuffer*>(commandBuffer);
			ID3D12DescriptorHeap* ppHeaps[] = { m_heap.Get() };
			d3dcommandBuffer->m_commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
		}
	}
}
