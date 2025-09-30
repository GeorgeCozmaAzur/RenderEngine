#include "D3D12RenderPass.h"
#include "D3D12CommandBuffer.h"
#include "DXSampleHelper.h"

namespace engine
{
	namespace render
	{
		void D3D12RenderPass::Create(uint32_t width, uint32_t height, std::vector<FrameBufferObject> frameBuffers, D3D12_RESOURCE_STATES stateBeforeRendering, D3D12_RESOURCE_STATES stateAfterRendering)
		{
			m_width = width;
			m_height = height;
			m_frameBuffers = frameBuffers;

			m_viewport = CD3DX12_VIEWPORT{ 0.0f, 0.0f, static_cast<float>(m_width), static_cast<float>(m_height) };
			m_scissorRect = CD3DX12_RECT{ 0, 0, static_cast<LONG>(m_width), static_cast<LONG>(m_height) };
			m_stateBeforeRendering = stateBeforeRendering;
			m_stateAfterRendering = stateAfterRendering;
		}

		void D3D12RenderPass::Begin(ID3D12GraphicsCommandList* commandList, uint32_t frameBufferIndex)
		{
			commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_frameBuffers[frameBufferIndex].m_colorTexture, m_stateAfterRendering, m_stateBeforeRendering));
			commandList->RSSetViewports(1, &m_viewport);
			commandList->RSSetScissorRects(1, &m_scissorRect);
			commandList->OMSetRenderTargets(1, &m_frameBuffers[frameBufferIndex].m_rtvHandle, FALSE, &m_frameBuffers[frameBufferIndex].m_dsvHandle);
			const float clearColoro[] = { 1.0f, 1.0f, 1.0f, 1.0f };
			commandList->ClearRenderTargetView(m_frameBuffers[frameBufferIndex].m_rtvHandle, clearColoro, 0, nullptr);
			commandList->ClearDepthStencilView(m_frameBuffers[frameBufferIndex].m_dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
		}

		void D3D12RenderPass::End(ID3D12GraphicsCommandList* commandList, uint32_t frameBufferIndex)
		{
			commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_frameBuffers[frameBufferIndex].m_colorTexture, m_stateBeforeRendering, m_stateAfterRendering));
		}

		void D3D12RenderPass::Begin(CommandBuffer* commandBuffer, uint32_t frameBufferIndex)
		{
			D3D12CommandBuffer* d3dcommandBuffer = dynamic_cast<D3D12CommandBuffer*>(commandBuffer);
			Begin(d3dcommandBuffer->m_commandList.Get(), frameBufferIndex);
		}

		void D3D12RenderPass::End(CommandBuffer* commandBuffer, uint32_t frameBufferIndex)
		{
			D3D12CommandBuffer* d3dcommandBuffer = dynamic_cast<D3D12CommandBuffer*>(commandBuffer);
			End(d3dcommandBuffer->m_commandList.Get(), frameBufferIndex);
		}
	}
}
