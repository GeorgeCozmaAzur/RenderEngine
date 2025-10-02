#include "D3D12CommandBuffer.h"
#include "DXSampleHelper.h"

namespace engine
{
	namespace render
	{
		D3D12CommandBuffer::~D3D12CommandBuffer()
		{

		}
		void D3D12CommandBuffer::Create(ID3D12Device* device)
		{
			ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));
			ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList)));
			ThrowIfFailed(m_commandList->Close());
		}

		void D3D12CommandBuffer::Begin()
		{
			ThrowIfFailed(m_commandAllocator->Reset());
			ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), nullptr));
		}

		void D3D12CommandBuffer::End()
		{
			ThrowIfFailed(m_commandList->Close());
		}
	}
}
