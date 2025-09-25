#include "D3D12Mesh.h"
#include "D3D12CommandBuffer.h"

using Microsoft::WRL::ComPtr;

namespace engine
{
    namespace render
    {
        //void Geometry::Load(ID3D12Device* device, std::string fileName, XMFLOAT3 atPosition, float scale, ID3D12GraphicsCommandList* commandList)
        //{
        //    Assimp::Importer Importer;
        //    const aiScene* pScene;
        //    static const int defaultFlags = aiProcess_FlipWindingOrder | aiProcess_Triangulate | aiProcess_PreTransformVertices | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals;
        //    pScene = Importer.ReadFile(fileName.c_str(), defaultFlags);
        //    if (!pScene)
        //    {
        //        std::string error = Importer.GetErrorString();
        //    }
        //    std::vector<Vertex> vertices;
        //    std::vector<UINT> modelindices;
        //    if (pScene)
        //    {
        //
        //        //  parts.clear();
        //        //  parts.resize(pScene->mNumMeshes);
        //        const aiVector3D Zero3D(0.0f, 0.0f, 0.0f);
        //
        //        // Load meshes
        //        for (unsigned int i = 0; i < pScene->mNumMeshes; i++)
        //        {
        //            const aiMesh* paiMesh = pScene->mMeshes[i];
        //            for (unsigned int j = 0; j < paiMesh->mNumVertices; j++)
        //            {
        //                const aiVector3D* pPos = &(paiMesh->mVertices[j]);
        //                const aiVector3D* pNormal = paiMesh->HasNormals() ? &(paiMesh->mNormals[j]) : &Zero3D;
        //                const aiVector3D* pTexCoord = (paiMesh->HasTextureCoords(0)) ? &(paiMesh->mTextureCoords[0][j]) : &Zero3D;
        //                Vertex v;
        //                v.position.x = pPos->x * scale + atPosition.x;
        //                v.position.y = pPos->y * scale + atPosition.y;
        //                v.position.z = pPos->z * scale + atPosition.z;
        //                v.uv.x = pTexCoord->x;
        //                v.uv.y = pTexCoord->y;
        //                vertices.push_back(v);
        //            }
        //
        //            int indexBase = 0;
        //            for (unsigned int j = 0; j < paiMesh->mNumFaces; j++)
        //            {
        //                const aiFace& Face = paiMesh->mFaces[j];
        //                if (Face.mNumIndices != 3)
        //                    continue;
        //                modelindices.push_back(indexBase + Face.mIndices[0]);
        //                modelindices.push_back(indexBase + Face.mIndices[1]);
        //                modelindices.push_back(indexBase + Face.mIndices[2]);
        //                // geometry->m_indices[index_index++] = indexBase + Face.mIndices[1];
        //               //  geometry->m_indices[index_index++] = indexBase + Face.mIndices[2];
        //                 //parts[i].indexCount += 3;
        //                 //geometry->m_indexCount += 3;
        //            }
        //        }
        //    }
        //    // Create the vertex buffer.
        //    {
        //
        //        const UINT vertexBufferSize = vertices.size() * sizeof(Vertex);//sizeof(triangleVertices);
        //        m_vertexBuffer.CreateGPUVisible(device,commandList, vertexBufferSize, vertices.data(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        //        m_vertexBuffer.CreateView(sizeof(Vertex));
        //        //const UINT vertexBufferSize = sizeof(triangleVertices);
        //
        //        // Note: using upload heaps to transfer static data like vert buffers is not 
        //        // recommended. Every time the GPU needs it, the upload heap will be marshalled 
        //        // over. Please read up on Default Heap usage. An upload heap is used here for 
        //        // code simplicity and because there are very few verts to actually transfer.
        //       // ThrowIfFailed(device->CreateCommittedResource(
        //       //     &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        //       //     D3D12_HEAP_FLAG_NONE,
        //       //     &CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
        //       //     D3D12_RESOURCE_STATE_COMMON,
        //       //     nullptr,
        //       //     IID_PPV_ARGS(&m_vertexBuffer)));
        //
        //       // CD3DX12_RESOURCE_BARRIER barrier =
        //       //     CD3DX12_RESOURCE_BARRIER::Transition(
        //       //         m_vertexBuffer.Get(),
        //       //         D3D12_RESOURCE_STATE_COMMON,
        //       //         D3D12_RESOURCE_STATE_COPY_DEST);
        //       // commandList->ResourceBarrier(1, &barrier);
        //
        //       // ThrowIfFailed(device->CreateCommittedResource(
        //       //     &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        //       //     D3D12_HEAP_FLAG_NONE,
        //       //     &CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
        //       //     D3D12_RESOURCE_STATE_GENERIC_READ,
        //       //     nullptr,
        //       //     IID_PPV_ARGS(&vertexBufferUploadHeap)));
        //
        //       // // Copy the triangle data to the vertex buffer.
        //       ///* UINT8* pVertexDataBegin;
        //       // CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
        //       // ThrowIfFailed(m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
        //       // //memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
        //       // memcpy(pVertexDataBegin, vertices.data(), vertexBufferSize);
        //       // m_vertexBuffer->Unmap(0, nullptr);*/
        //       // // Copy data to the intermediate upload heap and then schedule a copy 
        //       //// from the upload heap to the vertex buffer.
        //       // D3D12_SUBRESOURCE_DATA vertexData = {};
        //       // vertexData.pData = vertices.data();
        //       // vertexData.RowPitch = vertexBufferSize;
        //       // vertexData.SlicePitch = vertexData.RowPitch;
        //
        //       // UpdateSubresources<1>(commandList, m_vertexBuffer.Get(), vertexBufferUploadHeap.Get(), 0, 0, 1, &vertexData);
        //       // commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_vertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));
        //
        //       // // Initialize the vertex buffer view.
        //       // m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
        //       // m_vertexBufferView.StrideInBytes = sizeof(Vertex);
        //       // m_vertexBufferView.SizeInBytes = vertexBufferSize;
        //
        //        //UINT triangleIndices[] = { 3, 1, 0, 0,1,2 };
        //        //const UINT indexBufferSize = sizeof(triangleIndices);
        //        const UINT indexBufferSize = modelindices.size() * sizeof(UINT);
        //
        //        m_indexBuffer.CreateGPUVisible(device, commandList, indexBufferSize, modelindices.data(), D3D12_RESOURCE_STATE_INDEX_BUFFER);
        //        m_indexBuffer.CreateView();
        //
        //        //ThrowIfFailed(device->CreateCommittedResource(
        //        //    &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        //        //    D3D12_HEAP_FLAG_NONE,
        //        //    &CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize),
        //        //    D3D12_RESOURCE_STATE_COPY_DEST,
        //        //    nullptr,
        //        //    IID_PPV_ARGS(&m_indexBuffer)));
        //        //ThrowIfFailed(device->CreateCommittedResource(
        //        //    &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        //        //    D3D12_HEAP_FLAG_NONE,
        //        //    &CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize),
        //        //    D3D12_RESOURCE_STATE_GENERIC_READ,
        //        //    nullptr,
        //        //    IID_PPV_ARGS(&indexBufferUploadHeap)));
        //
        //        ///* UINT8* pindexDataBegin;
        //        // CD3DX12_RANGE ireadRange(0, 0);        // We do not intend to read from this resource on the CPU.
        //        // ThrowIfFailed(m_indexBuffer->Map(0, &ireadRange, reinterpret_cast<void**>(&pindexDataBegin)));
        //        // //memcpy(pindexDataBegin, triangleIndices, indexBufferSize);
        //        // memcpy(pindexDataBegin, modelindices.data(), indexBufferSize);
        //        // m_indexBuffer->Unmap(0, nullptr);*/
        //        // // Copy data to the intermediate upload heap and then schedule a copy 
        //        // // from the upload heap to the index buffer.
        //        //D3D12_SUBRESOURCE_DATA indexData = {};
        //        //indexData.pData = modelindices.data();
        //        //indexData.RowPitch = indexBufferSize;
        //        //indexData.SlicePitch = indexData.RowPitch;
        //
        //        //UpdateSubresources<1>(commandList, m_indexBuffer.Get(), indexBufferUploadHeap.Get(), 0, 0, 1, &indexData);
        //        //commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_indexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER));
        //
        //        //m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
        //        //m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
        //        //m_indexBufferView.SizeInBytes = indexBufferSize;
        //
        //        //m_numIndices = indexBufferSize / sizeof(UINT);
        //    }
        //}
        void D3D12Mesh::Create(ID3D12Device* device, MeshData* data, VertexLayout* vlayout, ID3D12GraphicsCommandList* commandList)
        {
            const UINT vertexBufferSize = data->m_vertexCount * vlayout->GetVertexSize(0);
            m_vertexBuffer.CreateGPUVisible(device, commandList, vertexBufferSize, data->m_vertices, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
            m_vertexBuffer.CreateView(vlayout->GetVertexSize(0));

            const UINT indexBufferSize = data->m_indexCount * sizeof(UINT);
            m_indexBuffer.CreateGPUVisible(device, commandList, indexBufferSize, data->m_indices, D3D12_RESOURCE_STATE_INDEX_BUFFER);
            m_indexBuffer.CreateView();
        }

        void D3D12Mesh::FreeRamData()
        {
            m_indexBuffer.FreeStagingBuffer();
            m_vertexBuffer.FreeStagingBuffer();
        }

        void D3D12Mesh::Draw(ID3D12GraphicsCommandList* commandList)
        {
            commandList->IASetIndexBuffer(&m_indexBuffer.m_view);
            commandList->IASetVertexBuffers(0, 1, &m_vertexBuffer.m_view);
            commandList->DrawIndexedInstanced(m_indexBuffer.m_numIndices, 1, 0, 0, 0);
        }

        void D3D12Mesh::Draw(CommandBuffer* commandBuffer)
        {
            D3D12CommandBuffer* d3dcommandBuffer = dynamic_cast<D3D12CommandBuffer*>(commandBuffer);
            Draw(d3dcommandBuffer->m_commandList.Get());
        }
    }
}