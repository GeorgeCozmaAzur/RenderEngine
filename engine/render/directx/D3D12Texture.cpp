#include "D3D12Texture.h"
//#if !defined(__ANDROID__)
//#define STB_IMAGE_IMPLEMENTATION
//#include <stb_image.h>
//#endif

namespace engine
{
    namespace render
    {

        using Microsoft::WRL::ComPtr;

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

        void D3D12Texture::Load(ID3D12Device* device, std::string fileName, ID3D12GraphicsCommandList* commandList, D3D12_CPU_DESCRIPTOR_HANDLE descriptorHeapAdress)
        {
            // Note: ComPtr's are CPU objects but this resource needs to stay in scope until
           // the command list that references it has finished executing on the GPU.
           // We will flush the GPU at the end of this method to ensure the resource is not
           // prematurely destroyed.

            tdata.LoadFromFile(fileName, GfxFormat::R8G8B8A8_UNORM);

            /*int texChannels;
            stbi_uc* pixels = stbi_load(fileName.c_str(), &m_Width, &m_Height, &texChannels, STBI_rgb_alpha);

            assert(pixels);

            int m_imageSize = m_Width * m_Height * 4;

            m_ram_data = new char[m_imageSize];
            memcpy(m_ram_data, pixels, m_imageSize);

            stbi_image_free(pixels);*/

            //m_width = static_cast<uint32_t>(texWidth);
           // m_height = static_cast<uint32_t>(texHeight);
           // Microsoft::WRL::ComPtr<ID3D12Resource> m_textureUploadHeap;
            // Create the texture.
            {
                // Describe and create a Texture2D.
                D3D12_RESOURCE_DESC textureDesc = {};
                textureDesc.MipLevels = 1;
                textureDesc.Format = ToDxgiFormat(GfxFormat::R8G8B8A8_UNORM);
                textureDesc.Width = tdata.m_width;
                textureDesc.Height = tdata.m_height;
                textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
                textureDesc.DepthOrArraySize = 1;
                textureDesc.SampleDesc.Count = 1;
                textureDesc.SampleDesc.Quality = 0;
                textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

                device->CreateCommittedResource(
                    &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
                    D3D12_HEAP_FLAG_NONE,
                    &textureDesc,
                    D3D12_RESOURCE_STATE_COPY_DEST,
                    nullptr,
                    IID_PPV_ARGS(&m_texture));

                const UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_texture.Get(), 0, 1);

                // Create the GPU upload buffer.
                device->CreateCommittedResource(
                    &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                    D3D12_HEAP_FLAG_NONE,
                    &CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
                    D3D12_RESOURCE_STATE_GENERIC_READ,
                    nullptr,
                    IID_PPV_ARGS(&m_textureUploadHeap));

                // Copy data to the intermediate upload heap and then schedule a copy 
                // from the upload heap to the Texture2D.
               // std::vector<UINT8> texture = GenerateTextureData();

                D3D12_SUBRESOURCE_DATA textureData = {};
                textureData.pData = tdata.m_ram_data;//&texture[0];
                textureData.RowPitch = tdata.m_width * m_TexturePixelSize;
                textureData.SlicePitch = textureData.RowPitch * tdata.m_height;

                UpdateSubresources(commandList, m_texture.Get(), m_textureUploadHeap.Get(), 0, 0, 1, &textureData);
                commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

                //// Describe and create a SRV for the texture.
                D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
                srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                srvDesc.Format = textureDesc.Format;
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                srvDesc.Texture2D.MipLevels = 1;
                //device->CreateShaderResourceView(m_texture.Get(), &srvDesc, m_srvHeap->GetCPUDescriptorHandleForHeapStart());
                device->CreateShaderResourceView(m_texture.Get(), &srvDesc, descriptorHeapAdress);
            }
        }

        void D3D12Texture::FreeRamData()
        {
            m_textureUploadHeap.Reset();
            tdata.Destroy();
            //delete[] m_ram_data;
        }
    }
}