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

        void D3D12Texture::Create(ID3D12Device* device, uint32_t width, uint32_t height, GfxFormat format, D3D12_RESOURCE_FLAGS resourceFlags, D3D12_RESOURCE_STATES resourceState)
        {
            m_width = width;
            m_height = height;
            m_resourceFlags = resourceFlags;
            m_dxgiFormat = ToDxgiFormat(format);
            const D3D12_RESOURCE_DESC textureDesc = CD3DX12_RESOURCE_DESC::Tex2D(m_dxgiFormat,
                static_cast<UINT64>(width),
                static_cast<UINT>(height),
                1, 1, 1, 0, resourceFlags);

            D3D12_CLEAR_VALUE* clearValuep = nullptr;
            D3D12_CLEAR_VALUE clearValue = {};

            if ((resourceFlags & D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) > 0)
            {
                clearValue.Format = DXGI_FORMAT_D32_FLOAT;
                clearValue.DepthStencil.Depth = 1.0f;
                clearValue.DepthStencil.Stencil = 0;
                clearValuep = &clearValue;
            }
            else
            {
                if ((resourceFlags & D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)>0)
                {
                    clearValue = { m_dxgiFormat, { m_ClearColor[0], m_ClearColor[1], m_ClearColor[2], m_ClearColor[3]}};
                    clearValuep = &clearValue;
                }
            }

            device->CreateCommittedResource(
                &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
                D3D12_HEAP_FLAG_NONE,
                &textureDesc,
                resourceState,
                clearValuep,
                IID_PPV_ARGS(&m_texture));
        }

        UINT64 D3D12Texture::GetSize()
        {
            return GetRequiredIntermediateSize(m_texture.Get(), 0, 1);
        }

        void D3D12Texture::Upload(ID3D12Resource* staggingBuffer, ID3D12GraphicsCommandList* commandList, TextureExtent** extents, void *data)
        {
           // const UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_texture.Get(), 0, 1);

            // Create the GPU upload buffer.
           /* device->CreateCommittedResource(
                &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                D3D12_HEAP_FLAG_NONE,
                &CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&m_textureUploadHeap));*/

            // Copy data to the intermediate upload heap and then schedule a copy 
            // from the upload heap to the Texture2D.
           // std::vector<UINT8> texture = GenerateTextureData();

            D3D12_SUBRESOURCE_DATA textureData = {};
            textureData.pData = data;//&texture[0];
            textureData.RowPitch = extents[0][0].width * m_TexturePixelSize;
            textureData.SlicePitch = textureData.RowPitch * extents[0][0].height;

            UpdateSubresources(commandList, m_texture.Get(), staggingBuffer, 0, 0, 1, &textureData);
            commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
        }

        void D3D12Texture::CreateDescriptor(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE descriptorHeapAdress, D3D12_GPU_DESCRIPTOR_HANDLE descriptorGPUHeapAdress)
        {
            //// Describe and create a SRV for the texture.
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.Format = m_dxgiFormat;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = 1;
            //device->CreateShaderResourceView(m_texture.Get(), &srvDesc, m_srvHeap->GetCPUDescriptorHandleForHeapStart());
            device->CreateShaderResourceView(m_texture.Get(), &srvDesc, descriptorHeapAdress);

            m_CPUHandle = descriptorHeapAdress;
            m_GPUHandle = descriptorGPUHeapAdress;
        }

        void D3D12Texture::Load(ID3D12Device* device, std::string fileName, ID3D12GraphicsCommandList* commandList, D3D12_CPU_DESCRIPTOR_HANDLE descriptorHeapAdress, D3D12_GPU_DESCRIPTOR_HANDLE descriptorGPUHeapAdress)
        {
            // Note: ComPtr's are CPU objects but this resource needs to stay in scope until
           // the command list that references it has finished executing on the GPU.
           // We will flush the GPU at the end of this method to ensure the resource is not
           // prematurely destroyed.

           /* tdata = new Texture2DData();
            tdata->LoadFromFile(fileName, GfxFormat::R8G8B8A8_UNORM);

            Create(device,tdata->m_width,tdata->m_height,tdata->m_format, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST);
            Upload(device,commandList,tdata->m_extents,tdata->m_ram_data);//TODO george
            CreateDescriptor(device, descriptorHeapAdress, descriptorGPUHeapAdress);*/

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
              /*  D3D12_RESOURCE_DESC textureDesc = {};
                textureDesc.MipLevels = 1;
                textureDesc.Format = ToDxgiFormat(GfxFormat::R8G8B8A8_UNORM);
                textureDesc.Width = tdata.m_width;
                textureDesc.Height = tdata.m_height;
                textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
                textureDesc.DepthOrArraySize = 1;
                textureDesc.SampleDesc.Count = 1;
                textureDesc.SampleDesc.Quality = 0;
                textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;*/

               // const D3D12_RESOURCE_DESC textureDesc = CD3DX12_RESOURCE_DESC::Tex2D(ToDxgiFormat(GfxFormat::R8G8B8A8_UNORM),
               //     static_cast<UINT64>(tdata.m_width),
               //     static_cast<UINT>(tdata.m_height),
               //     1, 1, 1, 0, D3D12_RESOURCE_FLAG_NONE);

               // device->CreateCommittedResource(
               //     &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
               //     D3D12_HEAP_FLAG_NONE,
               //     &textureDesc,
               //     D3D12_RESOURCE_STATE_COPY_DEST,
               //     nullptr,
               //     IID_PPV_ARGS(&m_texture));

               // const UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_texture.Get(), 0, 1);

               // // Create the GPU upload buffer.
               // device->CreateCommittedResource(
               //     &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
               //     D3D12_HEAP_FLAG_NONE,
               //     &CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
               //     D3D12_RESOURCE_STATE_GENERIC_READ,
               //     nullptr,
               //     IID_PPV_ARGS(&m_textureUploadHeap));

               // // Copy data to the intermediate upload heap and then schedule a copy 
               // // from the upload heap to the Texture2D.
               //// std::vector<UINT8> texture = GenerateTextureData();

               // D3D12_SUBRESOURCE_DATA textureData = {};
               // textureData.pData = tdata.m_ram_data;//&texture[0];
               // textureData.RowPitch = tdata.m_width * m_TexturePixelSize;
               // textureData.SlicePitch = textureData.RowPitch * tdata.m_height;

               // UpdateSubresources(commandList, m_texture.Get(), m_textureUploadHeap.Get(), 0, 0, 1, &textureData);
               // commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

               // //// Describe and create a SRV for the texture.
               // D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
               // srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
               // srvDesc.Format = textureDesc.Format;
               // srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
               // srvDesc.Texture2D.MipLevels = 1;
               // //device->CreateShaderResourceView(m_texture.Get(), &srvDesc, m_srvHeap->GetCPUDescriptorHandleForHeapStart());
               // device->CreateShaderResourceView(m_texture.Get(), &srvDesc, descriptorHeapAdress);

               // m_CPUHandle = descriptorHeapAdress;
               // m_GPUHandle = descriptorGPUHeapAdress;
            }
        }

        void D3D12RenderTarget::CreateDescriptor(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE descriptorHeapAdress, D3D12_GPU_DESCRIPTOR_HANDLE descriptorGPUHeapAdress, D3D12_CPU_DESCRIPTOR_HANDLE descriptorRTVHeapAdress)
        {
            m_CPUHandle = descriptorHeapAdress;
            m_GPUHandle = descriptorGPUHeapAdress;
            m_CPURTVHandle = descriptorRTVHeapAdress;
            if ((m_resourceFlags & D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) > 0)
            {
                device->CreateRenderTargetView(m_texture.Get(), nullptr, descriptorRTVHeapAdress);
                device->CreateShaderResourceView(m_texture.Get(), nullptr, m_CPUHandle);
            }
            else
            if ((m_resourceFlags & D3D12_RESOURCE_FLAGS::D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) > 0)
            {
                D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
                depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
                depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
                depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

                device->CreateDepthStencilView(m_texture.Get(), &depthStencilDesc, descriptorRTVHeapAdress);

                if (m_CPUHandle.ptr != 0)
                {
                    D3D12_SHADER_RESOURCE_VIEW_DESC srv = {};
                    srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                    srv.Format = DXGI_FORMAT_R32_FLOAT;
                    srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                    srv.Texture2D.MipLevels = 1;

                    device->CreateShaderResourceView(m_texture.Get(), &srv , m_CPUHandle);
                }
            }  
        }

        void D3D12Texture::FreeRamData()
        {
            //m_textureUploadHeap.Reset();
            //delete tdata;
            //delete[] m_ram_data;
        }
    }
}