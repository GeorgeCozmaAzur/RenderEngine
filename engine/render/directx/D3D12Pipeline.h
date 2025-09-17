#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <string>
#include <wrl.h>
#include <shellapi.h>
#include "PipeLine.h"

namespace engine
{
	namespace render
	{
		class D3D12Pipeline : public Pipeline
		{
		public:
			Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
			Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;

			void Load(ID3D12Device* device, std::wstring fileName, std::string vertexEntry, std::string fragmentEntry, int texturesNo, int cbNo, PipelineProperties properties);
		};
	}
}