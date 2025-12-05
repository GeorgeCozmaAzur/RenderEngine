#include "D3D12Texture.h"

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

        void D3D12Texture::Create(ID3D12Device* device, uint32_t width, uint32_t height, uint16_t layersCount, uint32_t mipCount, GfxFormat format, D3D12_RESOURCE_FLAGS resourceFlags, D3D12_RESOURCE_STATES resourceState)
        {
            m_width = width;
            m_height = height;
            m_layerCount = layersCount;
            m_mipLevelsCount = mipCount;
            m_resourceFlags = resourceFlags;
            m_dxgiFormat = ToDxgiFormat(format);
            const D3D12_RESOURCE_DESC textureDesc = CD3DX12_RESOURCE_DESC::Tex2D(m_dxgiFormat,
                static_cast<UINT64>(width),
                static_cast<UINT>(height),
                m_layerCount, m_mipLevelsCount, 1, 0, resourceFlags);

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
            return GetRequiredIntermediateSize(m_texture.Get(), 0, m_layerCount*m_mipLevelsCount);
        }

        void D3D12Texture::Upload(ID3D12Resource* staggingBuffer, ID3D12GraphicsCommandList* commandList, TextureExtent** extents, void *data)
        {
           m_TexturePixelSize = extents[0][0].size / (extents[0][0].width * extents[0][0].height);

            std::vector<D3D12_SUBRESOURCE_DATA> subresources;
            subresources.resize(m_layerCount * m_mipLevelsCount);

            UINT index = 0;

            for (UINT face = 0; face < m_layerCount; ++face)
            {
                for (UINT mip = 0; mip < m_mipLevelsCount; ++mip)
                {
                    D3D12_SUBRESOURCE_DATA& sr = subresources[index++];

                    sr.pData = extents[face][mip].data ? extents[face][mip].data : data;
                    sr.RowPitch = extents[face][mip].width * m_TexturePixelSize;   // UNCOMPRESSED RULE
                    sr.SlicePitch = sr.RowPitch * extents[face][mip].height;
                }
            }
            UpdateSubresources(
                commandList,
                m_texture.Get(),
                staggingBuffer,
                0, 0,
                (UINT)subresources.size(),
                subresources.data()
            );

            commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
        }

        void D3D12Texture::CreateDescriptor(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE descriptorHeapAdress, D3D12_GPU_DESCRIPTOR_HANDLE descriptorGPUHeapAdress)
        {
            //// Describe and create a SRV for the texture.
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.Format = m_dxgiFormat;
            if (m_layerCount == 1)
            {
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                srvDesc.Texture2D.MipLevels = m_mipLevelsCount;
                srvDesc.Texture2D.MostDetailedMip = 0;
            }
            else
                if (m_layerCount > 1)
                {
                    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
                    srvDesc.TextureCube.MipLevels = m_mipLevelsCount;
                    srvDesc.TextureCube.MostDetailedMip = 0;
                }
            //device->CreateShaderResourceView(m_texture.Get(), &srvDesc, m_srvHeap->GetCPUDescriptorHandleForHeapStart());
            device->CreateShaderResourceView(m_texture.Get(), &srvDesc, descriptorHeapAdress);

            m_CPUHandle = descriptorHeapAdress;
            m_GPUHandle = descriptorGPUHeapAdress;
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