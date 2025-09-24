#include "D3D12DescriptorTable.h"

namespace engine
{
	namespace render
	{
		void D3D12DescriptorTable::Create(DescriptorSetLayout* layout, std::vector<CD3DX12_GPU_DESCRIPTOR_HANDLE> buffers, std::vector<CD3DX12_GPU_DESCRIPTOR_HANDLE> textures)
		{
			auto buffersit = buffers.begin();
			auto texturesit = textures.begin();
			//for (auto binding : layout->m_bindings)
			for (UINT i = 0; i < layout->m_bindings.size(); i++)
			{
				if (layout->m_bindings[i].descriptorType == UNIFORM_BUFFER)
				{
					m_entries.push_back({i,*(buffersit++)});
				}
				else
				{
					m_entries.push_back({ i,*(texturesit++) });
				}
			}
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
