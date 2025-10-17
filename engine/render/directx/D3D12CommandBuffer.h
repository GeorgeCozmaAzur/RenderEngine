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
#include "render/CommandBuffer.h"

namespace engine
{
	namespace render
	{
		class D3D12CommandBuffer : public CommandBuffer
		{
		public:
			D3D12CommandBuffer(CommandPool* pool) : CommandBuffer(pool) {}
			~D3D12CommandBuffer();
			Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
			Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandAllocator;
			void Create(ID3D12Device *device);
			virtual void Begin();
			virtual void End();
		};
	}
}