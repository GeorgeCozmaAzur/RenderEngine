#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <string>
#include <wrl.h>
#include <shellapi.h>
#include <vector>
#include "render/DescriptorSet.h"
#include "render/directx/d3dx12.h"

namespace engine
{
	namespace render
	{
		struct TableEntry
		{
			CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle;
			UINT parameterIndex;
		};

		class D3D12DescriptorTable : public DescriptorSet
		{
			Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_Heap;
			std::vector<TableEntry> m_entries;
		public:

			void Create(std::vector<TableEntry> entries);
			void Draw(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList);
		};
	}
}