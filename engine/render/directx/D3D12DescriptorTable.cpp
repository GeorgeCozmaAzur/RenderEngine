#include "D3D12DescriptorTable.h"
#include "D3D12CommandBuffer.h"

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

		void D3D12DescriptorTable::Draw(ID3D12GraphicsCommandList* commandList)
		{
			for (int i = 0; i < m_entries.size(); i++)
			{
				commandList->SetGraphicsRootDescriptorTable(m_entries[i].parameterIndex, m_entries[i].gpuHandle);
			}
		}

		void D3D12DescriptorTable::Draw(class CommandBuffer* commandBuffer, Pipeline* pipeline, uint32_t indexInDynamicUniformBuffer)
		{
			D3D12CommandBuffer* d3dcommandBuffer = static_cast<D3D12CommandBuffer*>(commandBuffer);
			Draw(d3dcommandBuffer->m_commandList.Get());
		}
	}
}
