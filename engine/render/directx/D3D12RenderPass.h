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
			CD3DX12_CPU_DESCRIPTOR_HANDLE m_rtvHandle;
			CD3DX12_CPU_DESCRIPTOR_HANDLE m_dsvHandle;
			ID3D12Resource* m_colorTexture;
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

		public:
			void Create(uint32_t width, uint32_t height, std::vector<FrameBufferObject> frameBuffers, 
				D3D12_RESOURCE_STATES stateBeforeRendering = D3D12_RESOURCE_STATE_RENDER_TARGET, 
				D3D12_RESOURCE_STATES stateAfterRendering = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			void Begin(ID3D12GraphicsCommandList* commandList, uint32_t frameBufferIndex = 0);
			void End(ID3D12GraphicsCommandList* commandList, uint32_t frameBufferIndex = 0);
			virtual void Begin(CommandBuffer* commandBuffer, uint32_t frameBufferIndex = 0);
			virtual void End(CommandBuffer* commandBuffer, uint32_t frameBufferIndex = 0);
			virtual void NextSubPass(CommandBuffer* commandBuffer);
		};
	}
}