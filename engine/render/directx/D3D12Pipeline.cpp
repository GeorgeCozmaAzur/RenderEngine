#include "render/directx/D3D12Pipeline.h"
#include "render/directx/D3D12CommandBuffer.h"
#include "render/directx/D3D12RenderPass.h"
#include "render/directx/DXSampleHelper.h"
#include "render/directx/d3dx12.h"

using Microsoft::WRL::ComPtr;

namespace engine
{
    namespace render
    {
        /*inline DXGI_FORMAT GetVertexFormat(uint32_t size)
        {
            switch (size)
            {
            case 4:     return DXGI_FORMAT_R32_FLOAT;

            case 8:     return DXGI_FORMAT_R32G32_FLOAT;

            case 12:    return DXGI_FORMAT_R32G32B32_FLOAT;

            case 16:	
            default:    return DXGI_FORMAT_R32G32B32A32_FLOAT;
            }
        }*/

        DXGI_FORMAT GetDXFormat(Component component)
        {
            switch (component)
            {
            case VERTEX_COMPONENT_UV:
            case VERTEX_COMPONENT_POSITION2D:	return DXGI_FORMAT_R32G32_FLOAT;

            case VERTEX_COMPONENT_DUMMY_FLOAT:	return DXGI_FORMAT_R32_FLOAT;

            case VERTEX_COMPONENT_DUMMY_INT:	return DXGI_FORMAT_R32_SINT;

            case VERTEX_COMPONENT_COLOR_UINT:	return DXGI_FORMAT_R8G8B8A8_UNORM;

            case VERTEX_COMPONENT_POSITION4D:
            case VERTEX_COMPONENT_COLOR4:
            case VERTEX_COMPONENT_TANGENT4:
            case VERTEX_COMPONENT_DUMMY_VEC4:	return DXGI_FORMAT_R32G32B32A32_FLOAT;

            default:							return DXGI_FORMAT_R32G32B32_FLOAT;
            }
        }

        inline D3D12_CULL_MODE ToD3DCullMode(CullMode cullMode) {
            switch (cullMode) {
            case CullMode::NONE: return D3D12_CULL_MODE_NONE;
            case CullMode::FRONT: return D3D12_CULL_MODE_FRONT;
            case CullMode::BACK: return D3D12_CULL_MODE_BACK;
            case CullMode::FRONTBACK: return D3D12_CULL_MODE_NONE;
            default: return D3D12_CULL_MODE_NONE;
            }
        }

        inline D3D_PRIMITIVE_TOPOLOGY ToD3DTopology(PrimitiveTopolgy topology) {
            switch (topology) {
            case PrimitiveTopolgy::TRIANGLE_LIST: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
            case PrimitiveTopolgy::TRIANGLE_STRIP: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
            case PrimitiveTopolgy::LINE_LIST: return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
            default: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
            }
        }

        void D3D12Pipeline::Load(ID3D12Device* device, std::wstring fileName, std::string vertexEntry, std::string fragmentEntry, VertexLayout* vlayout, DescriptorSetLayout* dlayout, PipelineProperties properties, D3D12RenderPass* renderPass)
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
                std::vector<CD3DX12_ROOT_PARAMETER1> rootParameters(dlayout->m_bindings.size() + (properties.vertexConstantBlockSize > 0 ? 1 : 0));
                UINT bsrsrv = 0;
                UINT bsrcvb = 0;
                int i = 0;
                for (i=0; i < dlayout->m_bindings.size(); i++)
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

                if (properties.vertexConstantBlockSize > 0)
                {
                    m_constantBlockSize = properties.vertexConstantBlockSize;
                    rootParameters[i].InitAsConstants(
                        properties.vertexConstantBlockSize / 4,   // number of 32-bit values
                        bsrcvb,    // register (b1)
                        0,    // space
                        D3D12_SHADER_VISIBILITY_ALL);
                    m_constantsShaderRegister = i;
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
                sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
                sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
                sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
                sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
                sampler.MipLODBias = 0;
                sampler.MaxAnisotropy = 0;
                sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
                sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
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

                hr = D3DCompileFromFile(fileName.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, vertexEntry.c_str(), "vs_5_0", compileFlags, 0, &vertexShader, &errors);
                if (errors != nullptr)
                {
                    OutputDebugStringA((char*)errors->GetBufferPointer());
                }
                ThrowIfFailed(hr);

                hr = D3DCompileFromFile(fileName.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, fragmentEntry.c_str(), "ps_5_0", compileFlags, 0, &pixelShader, &errors);
                if (errors != nullptr)
                {
                    OutputDebugStringA((char*)errors->GetBufferPointer());
                }
                ThrowIfFailed(hr);

                size_t inputElemntsNo = vlayout->m_components[0].size() + (vlayout->m_components.size() > 1 ? vlayout->m_components[1].size() : 0);
                std::vector<std::string> componentNames;
                componentNames.reserve(inputElemntsNo);

                std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementDescs;
                for (UINT c = 0; c < vlayout->m_components.size(); c++)
                {
                    UINT offset = 0;
                    for (int i = 0; i < vlayout->m_components[c].size(); i++)
                    {         
                        componentNames.push_back((c>0 ? "INSTANCE_" : "") + vlayout->GetComponentName(vlayout->m_components[c][i]));
                        DXGI_FORMAT format = GetDXFormat(vlayout->m_components[c][i]);
                        D3D12_INPUT_CLASSIFICATION slotClass = c == 0 ? D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA : D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
                        inputElementDescs.push_back({ componentNames.back().c_str(), 0, format, c, offset, slotClass, c});
                        offset += vlayout->GetComponentSize(vlayout->m_components[c][i]);
                    }
                }
                
                // Define the vertex input layout.
              /*  D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =cdfvx f m vgc gfcvnmny bhjugtvjyv ouyvu jbbmn 
                {
                    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
                };*/

                std::vector<BlendAttachmentState> dummyAttachments = { {properties.blendEnable} };

                if (properties.attachmentCount == 0)
                {
                    properties.attachmentCount = dummyAttachments.size();
                    properties.pAttachments = (BlendAttachmentState*)dummyAttachments.data();
                }

                D3D12_BLEND_DESC blendDesc = {};
                blendDesc.AlphaToCoverageEnable = FALSE;
                blendDesc.IndependentBlendEnable = FALSE;

                for (int i = 0; i < properties.attachmentCount; i++)
                {
                    auto& rtBlend = blendDesc.RenderTarget[i];
                    if (properties.pAttachments[i].blend_enable)
                    {                      
                        rtBlend.BlendEnable = TRUE;
                        rtBlend.LogicOpEnable = FALSE;
                        rtBlend.SrcBlend = D3D12_BLEND_SRC_ALPHA;
                        rtBlend.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
                        rtBlend.BlendOp = D3D12_BLEND_OP_ADD;
                        rtBlend.SrcBlendAlpha = D3D12_BLEND_ONE;
                        rtBlend.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
                        rtBlend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
                        rtBlend.LogicOp = D3D12_LOGIC_OP_NOOP;
                        rtBlend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
                    }
                    else
                    {
                        rtBlend.BlendEnable = FALSE;
                        rtBlend.LogicOpEnable = FALSE;
                        rtBlend.SrcBlend = D3D12_BLEND_ONE;
                        rtBlend.DestBlend = D3D12_BLEND_ZERO;
                        rtBlend.BlendOp = D3D12_BLEND_OP_ADD;
                        rtBlend.SrcBlendAlpha = D3D12_BLEND_ONE;
                        rtBlend.DestBlendAlpha = D3D12_BLEND_ZERO;
                        rtBlend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
                        rtBlend.LogicOp = D3D12_LOGIC_OP_NOOP;
                        rtBlend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
                    }
                }

                // Describe and create the graphics pipeline state object (PSO).
                D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
                psoDesc.InputLayout = { inputElementDescs.data(), (UINT)inputElementDescs.size() };
                psoDesc.pRootSignature = m_rootSignature.Get();
                psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
                psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
                psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
                psoDesc.RasterizerState.CullMode = ToD3DCullMode(properties.cullMode);
                //psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
                //if (texturesNo == 0)
                if(properties.depthBias == true)
                {
                    psoDesc.RasterizerState.DepthBias = 0.0f;
                    psoDesc.RasterizerState.DepthBiasClamp = 0.1f;
                    psoDesc.RasterizerState.SlopeScaledDepthBias = 0.01f;
                }
                psoDesc.BlendState = blendDesc;
                psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
                psoDesc.DepthStencilState.DepthEnable = properties.depthTestEnable;
                psoDesc.DepthStencilState.DepthWriteMask = properties.depthWriteEnable ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
                //psoDesc.DepthStencilState.StencilEnable = FALSE;
                psoDesc.SampleMask = UINT_MAX;
                psoDesc.IBStripCutValue = !properties.primitiveRestart ? D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED : D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFFFFFF;
                psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
                std::vector<DXGI_FORMAT> formats;
                psoDesc.NumRenderTargets = 0;//renderPass->m_RTVFormats.size();
                for (int i = 0; i < renderPass->m_subPasses[properties.subpass].outputAttachmanets.size(); i++)
                {
                    if (renderPass->m_subPasses[properties.subpass].outputAttachmanets[i] < renderPass->m_RTVFormats.size())
                    {
                        psoDesc.RTVFormats[i] = renderPass->m_RTVFormats[renderPass->m_subPasses[properties.subpass].outputAttachmanets[i]];
                        psoDesc.NumRenderTargets++;
                    }
                }
                psoDesc.SampleDesc.Count = 1;
                psoDesc.DSVFormat = renderPass->m_DSVFormat;

                m_topology = ToD3DTopology(properties.topology);
                
                ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
            }
        }

        void D3D12Pipeline::LoadCompute(ID3D12Device* device, std::wstring fileName, DescriptorSetLayout* dlayout, size_t constantsSize)
        {
            D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

            // This is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
            featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

            if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
            {
                featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
            }

            std::vector<CD3DX12_DESCRIPTOR_RANGE1> ranges(dlayout->m_bindings.size()+1);
            std::vector<CD3DX12_ROOT_PARAMETER1> rootParameters(dlayout->m_bindings.size()+1 + (constantsSize > 0 ? 1 : 0));
            UINT bsrsrv = 0;
            UINT bsrcvb = 0;
            UINT bsruav = 0;
            int i = 0;
            for (i = 0; i < dlayout->m_bindings.size()+1; i++)
            {
                D3D12_DESCRIPTOR_RANGE_TYPE rangeType;
                if(i< dlayout->m_bindings.size())
                switch (dlayout->m_bindings[i].descriptorType)
                {
                case DescriptorType::UNIFORM_BUFFER:        ranges[i].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, bsrcvb++, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC); break;
                case DescriptorType::IMAGE_SAMPLER: 
                case DescriptorType::INPUT_STORAGE_BUFFER:  ranges[i].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, bsrsrv++, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC); break;
                case DescriptorType::OUTPUT_STORAGE_BUFFER: ranges[i].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, bsruav++, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC); break;
                }
                else
                {
                    ranges[i].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);
                }

                D3D12_SHADER_VISIBILITY sv = D3D12_SHADER_VISIBILITY_ALL;
                rootParameters[i].InitAsDescriptorTable(1, &ranges[i], sv);
            }

            D3D12_STATIC_SAMPLER_DESC sampler = {};
            sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
            sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            sampler.MipLODBias = 0;
            sampler.MaxAnisotropy = 0;
            sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
            sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
            sampler.MinLOD = 0.0f;
            sampler.MaxLOD = D3D12_FLOAT32_MAX;
            sampler.ShaderRegister = 0;
            sampler.RegisterSpace = 0;
            sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

            CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
            rootSignatureDesc.Init_1_1(rootParameters.size(), rootParameters.data(), 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE);

            ComPtr<ID3DBlob> signature;
            ComPtr<ID3DBlob> error;
            ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error));
            ThrowIfFailed(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));

            ComPtr<ID3DBlob> computeShader;
#if defined(_DEBUG)
            // Enable better shader debugging with the graphics debugging tools.
            UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
            UINT compileFlags = 0;
#endif
            Microsoft::WRL::ComPtr<ID3DBlob> errors;

            HRESULT hr;

            hr = D3DCompileFromFile(fileName.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "cs_5_0", compileFlags, 0, &computeShader, &errors);
            if (errors != nullptr)
            {
                OutputDebugStringA((char*)errors->GetBufferPointer());
            }
            ThrowIfFailed(hr);

            D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
            psoDesc.pRootSignature = m_rootSignature.Get(); // You create this separately
            psoDesc.CS = {
                computeShader->GetBufferPointer(),
                computeShader->GetBufferSize()
            };
            device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState));
        }

        void D3D12Pipeline::Draw(CommandBuffer* commandBuffer)
        {
            D3D12CommandBuffer* d3dcommandBuffer = static_cast<D3D12CommandBuffer*>(commandBuffer);
            d3dcommandBuffer->m_commandList->IASetPrimitiveTopology(m_topology);
            d3dcommandBuffer->m_commandList->SetPipelineState(m_pipelineState.Get());
            d3dcommandBuffer->m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
        }

        void D3D12Pipeline::PushConstants(CommandBuffer* commandBuffer, void* constantsData)
        {
            if (m_constantBlockSize > 0)
            {
                D3D12CommandBuffer* d3dcommandBuffer = static_cast<D3D12CommandBuffer*>(commandBuffer);
                d3dcommandBuffer->m_commandList->SetGraphicsRoot32BitConstants(m_constantsShaderRegister, m_constantBlockSize/4, constantsData, 0);
            }
        }
    }
}