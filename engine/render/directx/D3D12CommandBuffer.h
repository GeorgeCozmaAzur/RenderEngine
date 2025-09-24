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
		class D3D12CommandBuffer
		{
		public:
			Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
			Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandAllocator;
			void Create(ID3D12Device *device);
			void Begin();
			void End();
		};
	}
}