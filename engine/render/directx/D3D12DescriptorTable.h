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
#include "render/DescriptorSetLayout.h"
#include "render/directx/d3dx12.h"

namespace engine
{
	namespace render
	{
		struct TableEntry
		{
			UINT parameterIndex;
			CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle;	
		};

		class D3D12DescriptorTable : public DescriptorSet
		{
			Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_Heap;
			std::vector<TableEntry> m_entries;
		public:

			void Create(DescriptorSetLayout* layout, std::vector<CD3DX12_GPU_DESCRIPTOR_HANDLE> buffers, std::vector<CD3DX12_GPU_DESCRIPTOR_HANDLE> textures);
			void Draw(ID3D12GraphicsCommandList* commandList);
			virtual void Draw(class CommandBuffer* commandBuffer, Pipeline* pipeline = nullptr);
		};
	}
}