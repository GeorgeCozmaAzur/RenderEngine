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

namespace engine
{
	namespace render
	{
		class D3D12RenderPass
		{
			uint32_t m_width;
			uint32_t m_height;
			CD3DX12_VIEWPORT m_viewport;
			CD3DX12_RECT   m_scissorRect;
			CD3DX12_CPU_DESCRIPTOR_HANDLE m_rtvHandle;
			CD3DX12_CPU_DESCRIPTOR_HANDLE m_dsvHandle;
			ID3D12Resource* m_colorTexture;
		public:
			void Create(uint32_t width, uint32_t height, CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle, CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle, ID3D12Resource* colorTexture);
			void Begin(ID3D12GraphicsCommandList* commandList);
			void End(ID3D12GraphicsCommandList* commandList);
		};
	}
}