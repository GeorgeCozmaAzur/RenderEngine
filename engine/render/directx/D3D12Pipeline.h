#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <string>
#include <wrl.h>
#include <shellapi.h>
#include "PipeLine.h"
#include "DescriptorSetLayout.h"
#include "VertexLayout.h"

namespace engine
{
	namespace render
	{
		class D3D12Pipeline : public Pipeline
		{
		public:
			//~D3D12Pipeline() { m_rootSignature.Reset(); m_pipelineState.Reset(); }
			Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
			Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;

			UINT m_constantsShaderRegister;

			D3D_PRIMITIVE_TOPOLOGY m_topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

			void Load(ID3D12Device* device, std::wstring fileName, std::string vertexEntry, std::string fragmentEntry, VertexLayout* vlayout, DescriptorSetLayout* dlayout, PipelineProperties properties, class D3D12RenderPass* renderPass);
			void LoadCompute(ID3D12Device* device, std::wstring fileName, DescriptorSetLayout* dlayout, size_t constantsSize = 0);
			virtual void Draw(class CommandBuffer* commandBuffer);
			virtual void PushConstants(class CommandBuffer* commandBuffer, void* constantsData = nullptr);
		};
	}
}