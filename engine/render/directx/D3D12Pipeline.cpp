#include "render/directx/D3D12Pipeline.h"
#include "render/directx/DXSampleHelper.h"
#include "render/directx/d3dx12.h"

using Microsoft::WRL::ComPtr;

namespace engine
{
    namespace render
    {
        void D3D12Pipeline::Load(ID3D12Device* device, std::wstring fileName, std::string vertexEntry, std::string fragmentEntry, DescriptorSetLayout* dlayout, PipelineProperties properties)
        {
            // Create the root signature.
            {
                D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

                // This is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
                featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

                if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
                {
                    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
                }

                std::vector<CD3DX12_DESCRIPTOR_RANGE1> ranges(dlayout->m_bindings.size());
                std::vector<CD3DX12_ROOT_PARAMETER1> rootParameters(dlayout->m_bindings.size());
                UINT bsrsrv = 0;
                UINT bsrcvb = 0;
                for (int i = 0; i < dlayout->m_bindings.size(); i++)
                {
                    D3D12_DESCRIPTOR_RANGE_TYPE rangeType;
                    if(dlayout->m_bindings[i].descriptorType == DescriptorType::UNIFORM_BUFFER)
                    {
                        ranges[i].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, bsrcvb++, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
                    }
                    else
                    {
                        ranges[i].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, bsrsrv++, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
                    }
                    
                    D3D12_SHADER_VISIBILITY sv = dlayout->m_bindings[i].stage == ShaderStage::VERTEX ? D3D12_SHADER_VISIBILITY_ALL : D3D12_SHADER_VISIBILITY_PIXEL;
                    rootParameters[i].InitAsDescriptorTable(1, &ranges[i], sv);
                }

                //CD3DX12_DESCRIPTOR_RANGE1 ranges[3];
                //ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
                //ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
                //ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

                ////CD3DX12_ROOT_PARAMETER1 rootParameters[3];
                //rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);
                //rootParameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_ALL);
                //rootParameters[2].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_PIXEL);

                D3D12_STATIC_SAMPLER_DESC sampler = {};
                sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
                sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
                sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
                sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
                sampler.MipLODBias = 0;
                sampler.MaxAnisotropy = 0;
                sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
                sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
                sampler.MinLOD = 0.0f;
                sampler.MaxLOD = D3D12_FLOAT32_MAX;
                sampler.ShaderRegister = 0;
                sampler.RegisterSpace = 0;
                sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

                CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
                rootSignatureDesc.Init_1_1(rootParameters.size(), rootParameters.data(), 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

                ComPtr<ID3DBlob> signature;
                ComPtr<ID3DBlob> error;
                ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error));
                ThrowIfFailed(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
            }

            // Create the pipeline state, which includes compiling and loading shaders.
            {
                ComPtr<ID3DBlob> vertexShader;
                ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
                // Enable better shader debugging with the graphics debugging tools.
                UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
                UINT compileFlags = 0;
#endif
                Microsoft::WRL::ComPtr<ID3DBlob> errors;

                HRESULT hr;

                hr = D3DCompileFromFile(fileName.c_str(), nullptr, nullptr, vertexEntry.c_str(), "vs_5_0", compileFlags, 0, &vertexShader, &errors);
                if (errors != nullptr)
                {
                    OutputDebugStringA((char*)errors->GetBufferPointer());
                }
                ThrowIfFailed(hr);

                hr = D3DCompileFromFile(fileName.c_str(), nullptr, nullptr, fragmentEntry.c_str(), "ps_5_0", compileFlags, 0, &pixelShader, &errors);
                if (errors != nullptr)
                {
                    OutputDebugStringA((char*)errors->GetBufferPointer());
                }
                ThrowIfFailed(hr);

                // Define the vertex input layout.
                D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
                {
                    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
                };

                // Describe and create the graphics pipeline state object (PSO).
                D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
                psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
                psoDesc.pRootSignature = m_rootSignature.Get();
                psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
                psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
                psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
                //if (texturesNo == 0)
                if(properties.depthBias == true)
                {
                    psoDesc.RasterizerState.DepthBias = 0.0f;
                    psoDesc.RasterizerState.DepthBiasClamp = 0.1f;
                    psoDesc.RasterizerState.SlopeScaledDepthBias = 0.01f;
                }
                psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
                psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
                //psoDesc.DepthStencilState.DepthEnable = TRUE;
                //psoDesc.DepthStencilState.StencilEnable = FALSE;
                psoDesc.SampleMask = UINT_MAX;
                psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
                psoDesc.NumRenderTargets = 1;
                psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
                psoDesc.SampleDesc.Count = 1;
                psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
                ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
            }
        }
    }
}