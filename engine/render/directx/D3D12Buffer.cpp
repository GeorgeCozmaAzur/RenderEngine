#include "render/directx/D3D12Buffer.h"
#include "render/directx/DXSampleHelper.h"

namespace engine
{
	namespace render
	{
        void D3D12Buffer::Create(ID3D12Device* device, size_t size, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_STATES finalState)
        {
            m_size = size;
            ThrowIfFailed(device->CreateCommittedResource(
                &CD3DX12_HEAP_PROPERTIES(heapType),
                D3D12_HEAP_FLAG_NONE,
                &CD3DX12_RESOURCE_DESC::Buffer(m_size),
                finalState,
                nullptr,
                IID_PPV_ARGS(&m_buffer)));
        }

        void D3D12Buffer::CreateCPUVisible(ID3D12Device* device, size_t size, void* data)
        {
            m_size = size;
            ThrowIfFailed(device->CreateCommittedResource(
                &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                D3D12_HEAP_FLAG_NONE,
                &CD3DX12_RESOURCE_DESC::Buffer(m_size),
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&m_buffer)));

            // Map and initialize the constant buffer. We don't unmap this until the
            // app closes. Keeping things mapped for the lifetime of the resource is okay.
            CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
            ThrowIfFailed(m_buffer->Map(0, &readRange, &m_mapped));
            if(data)
            memcpy(m_mapped, data, m_size);
        }

		void D3D12Buffer::CreateGPUVisible(ID3D12Device* device, ID3D12Resource* staggingBuffer, ID3D12GraphicsCommandList* commandList, size_t size, void* data, D3D12_RESOURCE_STATES finalState)
		{
            m_size = size;
            ThrowIfFailed(device->CreateCommittedResource(
                &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
                D3D12_HEAP_FLAG_NONE,
                &CD3DX12_RESOURCE_DESC::Buffer(m_size),
                D3D12_RESOURCE_STATE_COMMON,
                nullptr,
                IID_PPV_ARGS(&m_buffer)));

            CD3DX12_RESOURCE_BARRIER barrier =
                CD3DX12_RESOURCE_BARRIER::Transition(
                    m_buffer.Get(),
                    D3D12_RESOURCE_STATE_COMMON,
                    D3D12_RESOURCE_STATE_COPY_DEST);
            commandList->ResourceBarrier(1, &barrier);

            //ThrowIfFailed(device->CreateCommittedResource(
            //    &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            //    D3D12_HEAP_FLAG_NONE,
            //    &CD3DX12_RESOURCE_DESC::Buffer(m_size),
            //    D3D12_RESOURCE_STATE_GENERIC_READ,
            //    nullptr,
            //    IID_PPV_ARGS(&m_stagingBuffer)));

            D3D12_SUBRESOURCE_DATA bufferData = {};
            bufferData.pData = data;
            bufferData.RowPitch = m_size;
            bufferData.SlicePitch = bufferData.RowPitch;

            UpdateSubresources<1>(commandList, m_buffer.Get(), staggingBuffer, 0, 0, 1, &bufferData);
            commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, finalState));
		}

        void D3D12Buffer::FreeStagingBuffer()
        {
            //m_stagingBuffer.Reset();
        }

		void D3D12UniformBuffer::Create(ID3D12Device* device, size_t size, void* data, CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle, CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle)
		{
			//m_size = size;
			//ThrowIfFailed(device->CreateCommittedResource(
			//	&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			//	D3D12_HEAP_FLAG_NONE,
			//	&CD3DX12_RESOURCE_DESC::Buffer(m_size),
			//	D3D12_RESOURCE_STATE_GENERIC_READ,
			//	nullptr,
			//	IID_PPV_ARGS(&m_buffer)));

			//// Map and initialize the constant buffer. We don't unmap this until the
			//// app closes. Keeping things mapped for the lifetime of the resource is okay.
			//CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
			//ThrowIfFailed(m_buffer->Map(0, &readRange, &m_mapped));
			//memcpy(m_mapped, data, m_size);
            CreateCPUVisible(device, size, data);

            // Describe and create a constant buffer view.
            D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
            cbvDesc.BufferLocation = m_buffer->GetGPUVirtualAddress();
            cbvDesc.SizeInBytes = (UINT)m_size;
            // m_device->CreateConstantBufferView(&cbvDesc, m_cbvHeap->GetCPUDescriptorHandleForHeapStart());
            device->CreateConstantBufferView(&cbvDesc, cpuHandle);

			m_CPUHandle = cpuHandle;
			m_GPUHandle = gpuHandle;
		}

        void D3D12UniformBuffer::CreateView(ID3D12Device* device, CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle, CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle)
        {
            // Describe and create a constant buffer view.
            D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
            cbvDesc.BufferLocation = m_buffer->GetGPUVirtualAddress();
            cbvDesc.SizeInBytes = (UINT)m_size;
            device->CreateConstantBufferView(&cbvDesc, cpuHandle);

            m_CPUHandle = cpuHandle;
            m_GPUHandle = gpuHandle;
        }

        void D3D12VertexBuffer::CreateView(UINT vertexSize)
        {
            m_view.BufferLocation = m_buffer->GetGPUVirtualAddress();
            m_view.StrideInBytes = vertexSize;
            m_view.SizeInBytes = m_size;
        }

        void D3D12IndexBuffer::CreateView(uint16_t indexSize)
        {
            m_view.BufferLocation = m_buffer->GetGPUVirtualAddress();
            m_view.Format = indexSize > 2 ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
            m_view.SizeInBytes = m_size;

            m_numIndices = m_size / indexSize;
            
        }
	}
}
