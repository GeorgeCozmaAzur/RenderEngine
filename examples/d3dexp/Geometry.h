#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <string>
#include <wrl.h>
#include <shellapi.h>
#include "render/directx/D3D12Buffer.h"

using namespace DirectX;
using namespace engine;

struct Vertex
{
    XMFLOAT3 position;
    XMFLOAT2 uv;
};

class Geometry
{
	//Microsoft::WRL::ComPtr<ID3D12Resource> vertexBufferUploadHeap;
	//Microsoft::WRL::ComPtr<ID3D12Resource> indexBufferUploadHeap;
public:
    render::D3D12VertexBuffer m_vertexBuffer;
    render::D3D12IndexBuffer m_indexBuffer;
    //Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
   // Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;
    //D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
   // D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
   // UINT m_numIndices;

	void Load(ID3D12Device* device, std::string fileName, XMFLOAT3 atPosition, float scale, ID3D12GraphicsCommandList* commandList);
    void FreeRamData();
    void Draw(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList);
};