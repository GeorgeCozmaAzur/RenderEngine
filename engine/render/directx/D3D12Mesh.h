#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <string>
#include <wrl.h>
#include <shellapi.h>
#include "render/directx/D3D12Buffer.h"
#include "render/VertexLayout.h"
#include "render/Mesh.h"

namespace engine
{
    namespace render
    {
        class D3D12Mesh : public Mesh
        {
        public:
            render::D3D12VertexBuffer* _vertexBuffer;
            render::D3D12VertexBuffer* _instanceBuffer;
            render::D3D12IndexBuffer* _indexBuffer;

	        //void Load(ID3D12Device* device, std::string fileName, XMFLOAT3 atPosition, float scale, ID3D12GraphicsCommandList* commandList);
            void Create(ID3D12Device* device, MeshData* data, VertexLayout* vlayout, ID3D12GraphicsCommandList* commandList);
            void FreeRamData();
            virtual void UpdateIndexBuffer(void* data, size_t size, size_t offset);
            virtual void UpdateVertexBuffer(void* data, size_t size, size_t offset);
            virtual void UpdateInstanceBuffer(void* data, size_t size, size_t offset);
            void Draw(ID3D12GraphicsCommandList* commandList, const std::vector<MeshPart>& parts = std::vector<MeshPart>());
            virtual void Draw(CommandBuffer* commandBuffer, const std::vector<MeshPart>& parts = std::vector<MeshPart>());
        };
    }
}