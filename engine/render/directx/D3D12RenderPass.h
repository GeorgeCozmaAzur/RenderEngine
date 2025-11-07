#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <string>
#include <wrl.h>
#include <shellapi.h>
#include <vector>
#include "render/directx/d3dx12.h"
#include "render/RenderPass.h"

namespace engine
{
	namespace render
	{
		struct FrameBufferObject
		{
			std::vector <CD3DX12_CPU_DESCRIPTOR_HANDLE> m_rtvHandles;
			std::vector <ID3D12Resource*> m_colorTextures;
			CD3DX12_CPU_DESCRIPTOR_HANDLE m_dsvHandle;
			ID3D12Resource* m_depthTexture;
		};

		class D3D12RenderPass : public RenderPass
		{
			uint32_t m_width;
			uint32_t m_height;
			CD3DX12_VIEWPORT m_viewport;
			CD3DX12_RECT   m_scissorRect;
			std::vector<FrameBufferObject> m_frameBuffers;
			D3D12_RESOURCE_STATES m_stateBeforeRendering;
			D3D12_RESOURCE_STATES m_stateAfterRendering;

			uint32_t currentSubpass = 0;
			
		public:
			std::vector<RenderSubpass> m_subPasses;
			std::vector<DXGI_FORMAT> m_RTVFormats;//TODO make them private
			std::vector<DirectX::XMFLOAT4> m_clearColors;
			DXGI_FORMAT m_DSVFormat;
			void Create(uint32_t width, uint32_t height, std::vector<DXGI_FORMAT> rtvFormats, DXGI_FORMAT dvsFormat, std::vector<FrameBufferObject> frameBuffers,
				D3D12_RESOURCE_STATES stateBeforeRendering = D3D12_RESOURCE_STATE_RENDER_TARGET, 
				D3D12_RESOURCE_STATES stateAfterRendering = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			void SetSubPasses(std::vector<RenderSubpass> subpasses) { m_subPasses = subpasses; }
			void Begin(ID3D12GraphicsCommandList* commandList, uint32_t frameBufferIndex = 0, uint32_t subpass = 0);
			void End(ID3D12GraphicsCommandList* commandList, uint32_t frameBufferIndex = 0, uint32_t subpass = 0);
			virtual void Begin(CommandBuffer* commandBuffer, uint32_t frameBufferIndex = 0);
			virtual void End(CommandBuffer* commandBuffer, uint32_t frameBufferIndex = 0);
			virtual void NextSubPass(CommandBuffer* commandBuffer);
		};
	}
}