#include "D3D12RenderPass.h"
#include "D3D12CommandBuffer.h"
#include "DXSampleHelper.h"

namespace engine
{
	namespace render
	{
		void D3D12RenderPass::Create(uint32_t width, uint32_t height, std::vector<DXGI_FORMAT> rtvFormats, DXGI_FORMAT dvsFormat, std::vector<FrameBufferObject> frameBuffers, D3D12_RESOURCE_STATES stateBeforeRendering, D3D12_RESOURCE_STATES stateAfterRendering)
		{
			m_width = width;
			m_height = height;
			m_frameBuffers = frameBuffers;
			m_RTVFormats = rtvFormats;
			m_DSVFormat = dvsFormat;

			m_viewport = CD3DX12_VIEWPORT{ 0.0f, 0.0f, static_cast<float>(m_width), static_cast<float>(m_height) };
			m_scissorRect = CD3DX12_RECT{ 0, 0, static_cast<LONG>(m_width), static_cast<LONG>(m_height) };
			m_stateBeforeRendering = stateBeforeRendering;
			m_stateAfterRendering = stateAfterRendering;
			m_subPasses.resize(1);
			m_subPasses[0].outputAttachmanets.resize(rtvFormats.size() + 1);
			for (int i = 0; i < rtvFormats.size() + 1; i++)
				m_subPasses[0].outputAttachmanets[i] = i;
		}

		void D3D12RenderPass::Begin(ID3D12GraphicsCommandList* commandList, uint32_t frameBufferIndex, uint32_t subpass)
		{
			commandList->RSSetViewports(1, &m_viewport);
			commandList->RSSetScissorRects(1, &m_scissorRect);
			//for(int i=0;i< m_frameBuffers[frameBufferIndex].m_colorTextures.size();i++)
			std::vector <CD3DX12_CPU_DESCRIPTOR_HANDLE> color_rtvHandles;
			for (int i = 0; i < m_subPasses[subpass].outputAttachmanets.size(); i++)//TODO make the transition for depth too
			{
				if (m_subPasses[subpass].outputAttachmanets[i] < m_frameBuffers[frameBufferIndex].m_colorTextures.size())
				{
					commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_frameBuffers[frameBufferIndex].m_colorTextures[m_subPasses[subpass].outputAttachmanets[i]], m_stateAfterRendering, m_stateBeforeRendering));
					color_rtvHandles.push_back(m_frameBuffers[frameBufferIndex].m_rtvHandles[m_subPasses[subpass].outputAttachmanets[i]]);
				}
			}
			commandList->OMSetRenderTargets(color_rtvHandles.size(), color_rtvHandles.data(), FALSE, &m_frameBuffers[frameBufferIndex].m_dsvHandle);
			//const float clearColoro[] = { 1.0f, 1.0f, 1.0f, 1.0f };
			//for (int i = 0; i < m_frameBuffers[frameBufferIndex].m_colorTextures.size(); i++)
			for (int i = 0; i < m_subPasses[subpass].outputAttachmanets.size(); i++)
			{
				if (m_subPasses[subpass].outputAttachmanets[i] < m_frameBuffers[frameBufferIndex].m_rtvHandles.size())
				commandList->ClearRenderTargetView(m_frameBuffers[frameBufferIndex].m_rtvHandles[m_subPasses[subpass].outputAttachmanets[i]], reinterpret_cast<const float*>(&m_clearColors[m_subPasses[subpass].outputAttachmanets[i]]), 0, nullptr);
			}
			commandList->ClearDepthStencilView(m_frameBuffers[frameBufferIndex].m_dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
		}

		void D3D12RenderPass::End(ID3D12GraphicsCommandList* commandList, uint32_t frameBufferIndex, uint32_t subpass)
		{
			//for (int i = 0; i < m_frameBuffers[frameBufferIndex].m_colorTextures.size(); i++)
			//commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_frameBuffers[frameBufferIndex].m_colorTextures[i], m_stateBeforeRendering, m_stateAfterRendering));
			for (int i = 0; i < m_subPasses[subpass].outputAttachmanets.size(); i++)
			{
				if (m_subPasses[subpass].outputAttachmanets[i] < m_frameBuffers[frameBufferIndex].m_colorTextures.size())
					commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_frameBuffers[frameBufferIndex].m_colorTextures[m_subPasses[subpass].outputAttachmanets[i]], m_stateBeforeRendering, m_stateAfterRendering));
			}
		}

		void D3D12RenderPass::Begin(CommandBuffer* commandBuffer, uint32_t frameBufferIndex)
		{
			currentSubpass = 0;
			D3D12CommandBuffer* d3dcommandBuffer = static_cast<D3D12CommandBuffer*>(commandBuffer);
			Begin(d3dcommandBuffer->m_commandList.Get(), frameBufferIndex);
		}

		void D3D12RenderPass::End(CommandBuffer* commandBuffer, uint32_t frameBufferIndex)
		{
			D3D12CommandBuffer* d3dcommandBuffer = static_cast<D3D12CommandBuffer*>(commandBuffer);
			End(d3dcommandBuffer->m_commandList.Get(), frameBufferIndex, currentSubpass);
		}

		void D3D12RenderPass::NextSubPass(CommandBuffer* commandBuffer)
		{
			//uint32_t oldsubpass = 0;
			//uint32_t nextsubpass = 1;
			D3D12CommandBuffer* d3dcommandBuffer = static_cast<D3D12CommandBuffer*>(commandBuffer);
			/*for (int i = 0; i < m_subPasses[oldsubpass].outputAttachmanets.size(); i++)
			{
				if (m_subPasses[oldsubpass].outputAttachmanets[i] < m_frameBuffers[0].m_colorTextures.size())
				d3dcommandBuffer->m_commandList.Get()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_frameBuffers[0].m_colorTextures[m_subPasses[oldsubpass].outputAttachmanets[i]], m_stateBeforeRendering, m_stateAfterRendering));
			}*/
			End(d3dcommandBuffer->m_commandList.Get(), 0, currentSubpass++);
			Begin(d3dcommandBuffer->m_commandList.Get(), 0, currentSubpass);
		}
	}
}
