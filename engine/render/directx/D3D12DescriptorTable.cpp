#include "D3D12DescriptorTable.h"

namespace engine
{
	namespace render
	{
		void D3D12DescriptorTable::Create(std::vector<TableEntry> entries)
		{
			m_entries = entries;
		}

		void D3D12DescriptorTable::Draw(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList)
		{
			for (int i = 0; i < m_entries.size(); i++)
			{
				commandList->SetGraphicsRootDescriptorTable(m_entries[i].parameterIndex, m_entries[i].gpuHandle);
			}
		}
	}
}
