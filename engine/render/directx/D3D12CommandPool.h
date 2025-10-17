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
#include "render/CommandPool.h"

namespace engine
{
	namespace render
	{
		class D3D12CommandPool : public CommandPool
		{
		public:
			virtual CommandBuffer* GetCommandBuffer();
			virtual void Reset();
		};
	}
}