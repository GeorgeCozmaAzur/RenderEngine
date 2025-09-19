#pragma once

#include "Texture.h"
#include <d3d12.h>
#include <dxgi1_6.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include "d3dx12.h"

namespace engine
{
    namespace render
    {
        class D3D12Texture : public Texture
        {
            inline DXGI_FORMAT ToDxgiFormat(GfxFormat format) {
                switch (format) {
                case GfxFormat::R8_UNORM: return DXGI_FORMAT_R8_UNORM;
                case GfxFormat::R8G8B8A8_UNORM: return DXGI_FORMAT_R8G8B8A8_UNORM;
                case GfxFormat::B8G8R8A8_UNORM: return DXGI_FORMAT_B8G8R8A8_UNORM;
                case GfxFormat::R16G16B16A16_UNORM: return DXGI_FORMAT_R16G16B16A16_UNORM;
                case GfxFormat::R16G16B16A16_SFLOAT: return DXGI_FORMAT_R16G16B16A16_FLOAT;
                case GfxFormat::R32G32B32A32_SFLOAT: return DXGI_FORMAT_R32G32B32A32_FLOAT;//TODO see what's the deal with signed or unsigned float
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

            CD3DX12_CPU_DESCRIPTOR_HANDLE m_CPUHandle;
            CD3DX12_GPU_DESCRIPTOR_HANDLE m_GPUHandle;

            void Load(ID3D12Device* device, std::string fileName, ID3D12GraphicsCommandList* commandList, D3D12_CPU_DESCRIPTOR_HANDLE descriptorHeapAdress, D3D12_GPU_DESCRIPTOR_HANDLE descriptorGPUHeapAdress);
            void FreeRamData();
        };
    }
}