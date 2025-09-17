#include "D3D12DescriptorTable.h"

namespace engine
{
	namespace render
	{

		void D3D12DescriptorTable::Create(Microsoft::WRL::ComPtr<ID3D12Device> device, Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap, D3D12_DESCRIPTOR_HEAP_TYPE heapType)
		{
			m_Heap = heap;
			m_HeapType = heapType;
			m_DescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}
		void D3D12DescriptorTable::AddEntry(CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle, UINT parameterIndex)
		{
			//CD3DX12_GPU_DESCRIPTOR_HANDLE cbvSrvHandle1(m_Heap->GetGPUDescriptorHandleForHeapStart(), heapOffset, m_DescriptorSize);
			m_entries.push_back({ gpuHandle, parameterIndex });
			//m_GPUHandles.push_back(cbvSrvHandle1);
		}

		void D3D12DescriptorTable::Draw(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList)
		{
			for (int i = 0; i < m_entries.size(); i++)
			{
				//CD3DX12_GPU_DESCRIPTOR_HANDLE cbvSrvHandle1(m_Heap->GetGPUDescriptorHandleForHeapStart(), entry.heapOffset, m_DescriptorSize);
				//commandList->SetGraphicsRootDescriptorTable(entry.parameterIndex, cbvSrvHandle1);

				commandList->SetGraphicsRootDescriptorTable(m_entries[i].parameterIndex, m_entries[i].gpuHandle);
			}
		}
	}
}
