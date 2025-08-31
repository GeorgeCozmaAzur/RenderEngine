#pragma once

#include "Texture.h"
#include <d3d12.h>
#include <dxgi1_6.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include "d3dx12.h"

using namespace engine::render;
class D3D12Texture : public Texture
{
    inline DXGI_FORMAT ToDxgiFormat(GfxFormat format) {
        switch (format) {
        case GfxFormat::R8_UNORM: return DXGI_FORMAT_R8_UNORM;
        case GfxFormat::R8G8B8A8_UNORM: return DXGI_FORMAT_R8G8B8A8_UNORM;
        case GfxFormat::B8G8R8A8_UNORM: return DXGI_FORMAT_B8G8R8A8_UNORM;
        case GfxFormat::D24_UNORM_S8_UINT: return DXGI_FORMAT_D24_UNORM_S8_UINT;
        case GfxFormat::D32_FLOAT: return DXGI_FORMAT_D32_FLOAT;
        default: return DXGI_FORMAT_UNKNOWN;
        }
    }

public:
    //int m_Width = 256;
    //int m_Height = 256;
    UINT m_TexturePixelSize = 4;
    Texture2DData tdata;

    //char* m_ram_data = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> m_texture;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_textureUploadHeap;

    void Load(ID3D12Device* device, std::string fileName, ID3D12GraphicsCommandList* commandList, D3D12_CPU_DESCRIPTOR_HANDLE descriptorHeapAdress);
    void FreeRamData();
};