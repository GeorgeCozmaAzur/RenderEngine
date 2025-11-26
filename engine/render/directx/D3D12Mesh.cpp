#include "D3D12Mesh.h"
#include "D3D12CommandBuffer.h"

using Microsoft::WRL::ComPtr;

namespace engine
{
    namespace render
    {
       
        void D3D12Mesh::Create(ID3D12Device* device, MeshData* data, VertexLayout* vlayout, ID3D12GraphicsCommandList* commandList)
        {
           /* const UINT vertexBufferSize = data->m_vertexCount * vlayout->GetVertexSize(0);
            m_vertexBuffer.CreateGPUVisible(device, commandList, vertexBufferSize, data->m_vertices, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
            m_vertexBuffer.CreateView(vlayout->GetVertexSize(0));

            const UINT indexBufferSize = data->m_indexCount * sizeof(UINT);
            m_indexBuffer.CreateGPUVisible(device, commandList, indexBufferSize, data->m_indices, D3D12_RESOURCE_STATE_INDEX_BUFFER);
            m_indexBuffer.CreateView();*/
        }

        void D3D12Mesh::FreeRamData()
        {
            _indexBuffer->FreeStagingBuffer();
            _vertexBuffer->FreeStagingBuffer();
        }

        void D3D12Mesh::UpdateIndexBuffer(void* data, size_t size, size_t offset)
        {
            _indexBuffer->MemCopy(data, size, offset);
        }

        void D3D12Mesh::UpdateVertexBuffer(void* data, size_t size, size_t offset)
        {
            _vertexBuffer->MemCopy(data, size, offset);
        }

        void D3D12Mesh::UpdateInstanceBuffer(void* data, size_t size, size_t offset)
        {
           _instanceBuffer->MemCopy(data, size, offset);
        }

        void D3D12Mesh::SetVertexBuffer(class render::Buffer* buffer)
        {
            _vertexBuffer = dynamic_cast<D3D12VertexBuffer*>(buffer);
        }

        void D3D12Mesh::Draw(ID3D12GraphicsCommandList* commandList, const std::vector<MeshPart>& parts)
        {
            if (_vertexBuffer == nullptr && _indexBuffer == nullptr)
            {
                commandList->DrawInstanced(m_indexCount, m_instanceNo, 0, 0);
                return;
            }

            commandList->IASetIndexBuffer(&_indexBuffer->m_view);
            commandList->IASetVertexBuffers(0, 1, &_vertexBuffer->m_view);
            if(_instanceBuffer)
                commandList->IASetVertexBuffers(1, 1, &_instanceBuffer->m_view);

            if(parts.size()==0)
            commandList->DrawIndexedInstanced(_indexBuffer->m_numIndices, m_instanceNo, 0, 0, 0);
            else
                for (const MeshPart& part : parts)
                {
                    commandList->DrawIndexedInstanced(part.indexCount, part.instanceCount, part.firstIndex, part.vertexOffset, part.firstInstance);
                }
        }

        void D3D12Mesh::Draw(CommandBuffer* commandBuffer, const std::vector<MeshPart>& parts)
        {
            D3D12CommandBuffer* d3dcommandBuffer = static_cast<D3D12CommandBuffer*>(commandBuffer);
            Draw(d3dcommandBuffer->m_commandList.Get(), parts);
        }
    }
}