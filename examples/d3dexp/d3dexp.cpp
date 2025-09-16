
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <vector>

#include "D3D12Application.h"
#include "D3D12Texture.h"
#include "Geometry.h"
#include "Pipeline.h"
#include "D3D12DescriptorTable.h"
#include "DXSampleHelper.h"

using Microsoft::WRL::ComPtr;
using namespace engine;

class VulkanExample : public D3D12Application
{
public:

	static const UINT TextureWidth = 1024;
	static const UINT TextureHeight = 1024;
	static const UINT TexturePixelSize = 4;    // The number of bytes used to represent a pixel in the texture.

	struct Vertex
	{
		XMFLOAT3 position;
		XMFLOAT2 uv;
	};

	struct SceneConstantBuffer
	{
		//XMFLOAT4 offset;
	   // float padding[60]; // Padding so the constant buffer is 256-byte aligned.
		XMFLOAT4X4 mp;
		XMFLOAT4X4 mv;
		XMFLOAT4X4 mlv;
		//XMFLOAT4 scale;
		//XMFLOAT4 offset;
		float padding[16];
	};
	static_assert((sizeof(SceneConstantBuffer) % 256) == 0, "Constant Buffer size must be 256-byte aligned");

	ComPtr<ID3D12Resource> m_renderTargeto;
	ComPtr<ID3D12DescriptorHeap> m_rtvoHeap;
	ComPtr<ID3D12DescriptorHeap> m_dsvoHeap;
	ComPtr<ID3D12DescriptorHeap> m_srvHeap;

	ComPtr<ID3D12Resource> m_depthStencilOffscreen;

	Pipeline pipelineOffscreen;
	Pipeline pipeline;
	Pipeline pipelineMT;
	Geometry geometry;
	Geometry planegeometry;
	// ComPtr<ID3D12Resource> m_texture;
	D3D12Texture m_texture;
	D3D12Texture m_texture1;

	ComPtr<ID3D12Resource> m_constantBuffer;
	ComPtr<ID3D12Resource> m_constantBufferOffscreen;
	SceneConstantBuffer m_constantBufferData;
	SceneConstantBuffer m_constantBufferData2;
	D3D12DescriptorTable planetable;

	UINT8* m_pCbvDataBegin;
	UINT8* m_pCbvDataBeginOffscreen;

	float planey = -0.5f;

	VulkanExample() : D3D12Application(true)
	{
		zoom = -3.75f;
		rotationSpeed = 0.5f;
		rotation = glm::vec3(15.0f, 0.f, 0.0f);
		title = "Render Engine Empty Scene";
		settings.overlay = true;
		camera.movementSpeed = 20.5f;
		camera.SetPerspective(60.0f, (float)width / (float)height, 0.1f, 1024.0f);
		camera.SetRotation(glm::vec3(0.0f, 0.0f, 0.0f));
		camera.SetPosition(glm::vec3(0.0f, 0.0f, -5.0f));
	}

	~VulkanExample()
	{
		// Clean up used Vulkan resources 
		// Note : Inherited destructor cleans up resources stored in base class
		
	}

	std::wstring GetAssetFullPath(LPCWSTR assetName)
	{
		std::wstring path = L"./../data/";
		return path + assetName;
	}

	void init()
	{	
		m_viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
		m_scissorRect = CD3DX12_RECT(0, 0, static_cast<LONG>(width), static_cast<LONG>(height));
		{
			D3D12_DESCRIPTOR_HEAP_DESC rtvoHeapDesc = {};
			rtvoHeapDesc.NumDescriptors = 1;
			rtvoHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			rtvoHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvoHeapDesc, IID_PPV_ARGS(&m_rtvoHeap)));

			// Describe and create a depth stencil view (DSV) descriptor heap.
			D3D12_DESCRIPTOR_HEAP_DESC dsvoHeapDesc = {};
			dsvoHeapDesc.NumDescriptors = 1;
			dsvoHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
			dsvoHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			ThrowIfFailed(m_device->CreateDescriptorHeap(&dsvoHeapDesc, IID_PPV_ARGS(&m_dsvoHeap)));

			// Describe and create a shader resource view (SRV) heap for the texture.
			D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
			srvHeapDesc.NumDescriptors = 6;
			srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			ThrowIfFailed(m_device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_srvHeap)));

			// Describe and create a constant buffer view (CBV) descriptor heap.
			// Flags indicate that this descriptor heap can be bound to the pipeline 
			// and that descriptors contained in it can be referenced by a root table.
		   /* D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {};
			cbvHeapDesc.NumDescriptors = 1;
			cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			ThrowIfFailed(m_device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&m_cbvHeap)));*/

			m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			m_cbvSrvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}
		planetable.Create(m_device, m_srvHeap, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		// Create frame resources.
		{
			//float width = 512;
			//float height = 512;
			DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
			CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvoHeap->GetCPUDescriptorHandleForHeapStart());
			//m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
			auto const heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

			const D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(format,
				static_cast<UINT64>(TextureWidth),
				static_cast<UINT>(TextureHeight),
				1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

			D3D12_CLEAR_VALUE clearValue = { format, {} };
			// memcpy(clearValue.Color, m_clearColor, sizeof(clearValue.Color));

			D3D12_RESOURCE_STATES m_state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

			// Create a render target
			ThrowIfFailed(
				m_device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
					&desc,
					m_state, &clearValue,
					IID_PPV_ARGS(&m_renderTargeto))
			);

			// SetDebugObjectName(m_resource.Get(), L"RenderTexture RT");

			 // Create RTV.
			m_device->CreateRenderTargetView(m_renderTargeto.Get(), nullptr, rtvHandle);

			CD3DX12_CPU_DESCRIPTOR_HANDLE cbvSrvHandle(m_srvHeap->GetCPUDescriptorHandleForHeapStart(), 2, m_cbvSrvDescriptorSize);
			// Create SRV.
			m_device->CreateShaderResourceView(m_renderTargeto.Get(), nullptr, cbvSrvHandle);
			planetable.AddEntry(2, 1);
		}
		// Create the depth stencil view.
		{
			D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
			depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
			depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

			D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
			depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
			depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
			depthOptimizedClearValue.DepthStencil.Stencil = 0;

			ThrowIfFailed(m_device->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, TextureWidth, TextureHeight, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
				D3D12_RESOURCE_STATE_DEPTH_WRITE,
				&depthOptimizedClearValue,
				IID_PPV_ARGS(&m_depthStencilOffscreen)
			));

			NAME_D3D12_OBJECT(m_depthStencilOffscreen);

			m_device->CreateDepthStencilView(m_depthStencilOffscreen.Get(), &depthStencilDesc, m_dsvoHeap->GetCPUDescriptorHandleForHeapStart());
		}

		pipelineOffscreen.Load(m_device.Get(), GetAssetFullPath(L"shaders/d3dexp/shaders.hlsl"), "VSMain", "PSMainOffscreen", 0, 1);
		pipeline.Load(m_device.Get(), GetAssetFullPath(L"shaders/d3dexp/shaders.hlsl"), "VSMain", "PSMain", 1, 1);
		pipelineMT.Load(m_device.Get(), GetAssetFullPath(L"shaders/d3dexp/shaders.hlsl"), "VSMain", "PSMainMT", 2, 1);

		// Create the command list.
		ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), pipeline.m_pipelineState.Get(), IID_PPV_ARGS(&m_commandList)));

		geometry.Load(m_device.Get(), "./../data/models/venus.fbx", XMFLOAT3(0.0, -0.7, 0.0), 0.03f, m_commandList.Get());
		planegeometry.Load(m_device.Get(), "./../data/models/plane.obj", XMFLOAT3(0.0, planey, 0.0), 0.1f, m_commandList.Get());

		// Create the constant buffer.
		const UINT constantBufferSize = sizeof(SceneConstantBuffer);    // CB size is required to be 256-byte aligned.
		{
			ThrowIfFailed(m_device->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize),
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&m_constantBuffer)));

			CD3DX12_CPU_DESCRIPTOR_HANDLE cbvSrvHandle(m_srvHeap->GetCPUDescriptorHandleForHeapStart(), 1, m_cbvSrvDescriptorSize);
			// Describe and create a constant buffer view.
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
			cbvDesc.BufferLocation = m_constantBuffer->GetGPUVirtualAddress();
			cbvDesc.SizeInBytes = constantBufferSize;
			// m_device->CreateConstantBufferView(&cbvDesc, m_cbvHeap->GetCPUDescriptorHandleForHeapStart());
			m_device->CreateConstantBufferView(&cbvDesc, cbvSrvHandle);

			// Map and initialize the constant buffer. We don't unmap this until the
			// app closes. Keeping things mapped for the lifetime of the resource is okay.
			CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
			ThrowIfFailed(m_constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_pCbvDataBegin)));
			memcpy(m_pCbvDataBegin, &m_constantBufferData, sizeof(m_constantBufferData));

			planetable.AddEntry(1, 2);
		}

		//the other constant buffer
		{
			ThrowIfFailed(m_device->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize),
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&m_constantBufferOffscreen)));

			CD3DX12_CPU_DESCRIPTOR_HANDLE cbvSrvHandle(m_srvHeap->GetCPUDescriptorHandleForHeapStart(), 4, m_cbvSrvDescriptorSize);
			// Describe and create a constant buffer view.
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
			cbvDesc.BufferLocation = m_constantBufferOffscreen->GetGPUVirtualAddress();
			cbvDesc.SizeInBytes = constantBufferSize;
			// m_device->CreateConstantBufferView(&cbvDesc, m_cbvHeap->GetCPUDescriptorHandleForHeapStart());
			m_device->CreateConstantBufferView(&cbvDesc, cbvSrvHandle);

			// Map and initialize the constant buffer. We don't unmap this until the
			// app closes. Keeping things mapped for the lifetime of the resource is okay.
			CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
			ThrowIfFailed(m_constantBufferOffscreen->Map(0, &readRange, reinterpret_cast<void**>(&m_pCbvDataBeginOffscreen)));
			memcpy(m_pCbvDataBeginOffscreen, &m_constantBufferData2, sizeof(m_constantBufferData2));
		}

		m_texture.Load(m_device.Get(), "./../data/textures/compass.jpg", m_commandList.Get(), m_srvHeap->GetCPUDescriptorHandleForHeapStart());
		planetable.AddEntry(0, 0);
		CD3DX12_CPU_DESCRIPTOR_HANDLE cbvSrvHandle1(m_srvHeap->GetCPUDescriptorHandleForHeapStart(), 3, m_cbvSrvDescriptorSize);
		m_texture1.Load(m_device.Get(), "./../data/textures/PlanksBare0002_1_S.jpg", m_commandList.Get(), cbvSrvHandle1);

		// Close the command list and execute it to begin the initial GPU setup.
		ThrowIfFailed(m_commandList->Close());
		ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
		m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		WaitForPreviousFrame();

		m_texture.FreeRamData();
		m_texture1.FreeRamData();
		geometry.FreeRamData();
		planegeometry.FreeRamData();

	}

	//here a descriptor pool will be created for the entire app. Now it contains 1 sampler because this is what the ui overlay needs
	void setupDescriptorPool()
	{
		/*std::vector<VkDescriptorPoolSize> poolSizes = {
			VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}
		};
		vulkanDevice->CreateDescriptorSetsPool(poolSizes, 1);*/
	}

	virtual void BuildCommandBuffers()
	{
		// Command list allocators can only be reset when the associated 
	// command lists have finished execution on the GPU; apps should use 
	// fences to determine GPU execution progress.
		ThrowIfFailed(m_commandAllocator->Reset());

		// However, when ExecuteCommandList() is called on a particular command 
		// list, that command list can then be reset at any time and must be before 
		// re-recording.
		ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), pipelineOffscreen.m_pipelineState.Get()));

		// Set necessary state.
		m_commandList->SetGraphicsRootSignature(pipelineOffscreen.m_rootSignature.Get());

		ID3D12DescriptorHeap* ppHeaps[] = { m_srvHeap.Get() };
		m_commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

		// m_commandList->SetGraphicsRootDescriptorTable(0, m_srvHeap->GetGPUDescriptorHandleForHeapStart());
		CD3DX12_GPU_DESCRIPTOR_HANDLE cbvSrvHandle(m_srvHeap->GetGPUDescriptorHandleForHeapStart(), 4, m_cbvSrvDescriptorSize);
		m_commandList->SetGraphicsRootDescriptorTable(0, cbvSrvHandle);
		//CD3DX12_GPU_DESCRIPTOR_HANDLE cbvSrvHandle1(m_srvHeap->GetGPUDescriptorHandleForHeapStart(), 3, m_cbvSrvDescriptorSize);
		//m_commandList->SetGraphicsRootDescriptorTable(1, cbvSrvHandle1);
		m_commandList->RSSetViewports(1, &m_viewport);
		m_commandList->RSSetScissorRects(1, &m_scissorRect);

		// Indicate that the back buffer will be used as a render target.
		//m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargeto.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));//george rly dono
		m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargeto.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));//george rly dono
		CD3DX12_VIEWPORT viewport(0.0f, 0.0f, static_cast<float>(TextureWidth), static_cast<float>(TextureHeight));
		CD3DX12_RECT   scissorRect(0, 0, static_cast<LONG>(TextureWidth), static_cast<LONG>(TextureHeight));
		m_commandList->RSSetViewports(1, &viewport);
		m_commandList->RSSetScissorRects(1, &scissorRect);
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvoHandle(m_rtvoHeap->GetCPUDescriptorHandleForHeapStart());
		CD3DX12_CPU_DESCRIPTOR_HANDLE dsvoHandle(m_dsvoHeap->GetCPUDescriptorHandleForHeapStart());
		m_commandList->OMSetRenderTargets(1, &rtvoHandle, FALSE, &dsvoHandle);
		//  const float clearColoro[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		 // m_commandList->ClearRenderTargetView(rtvoHandle, clearColoro, 0, nullptr);
		m_commandList->ClearDepthStencilView(m_dsvoHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
		m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_commandList->IASetIndexBuffer(&geometry.m_indexBufferView);
		m_commandList->IASetVertexBuffers(0, 1, &geometry.m_vertexBufferView);
		m_commandList->DrawIndexedInstanced(geometry.m_numIndices, 1, 0, 0, 0);
		m_commandList->IASetIndexBuffer(&planegeometry.m_indexBufferView);
		m_commandList->IASetVertexBuffers(0, 1, &planegeometry.m_vertexBufferView);
		m_commandList->DrawIndexedInstanced(planegeometry.m_numIndices, 1, 0, 0, 0);
		m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargeto.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));//george rly dono


		// Indicate that the back buffer will be used as a render target.
		m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[currentBuffer].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

		m_commandList->RSSetViewports(1, &m_viewport);
		m_commandList->RSSetScissorRects(1, &m_scissorRect);
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), currentBuffer, m_rtvDescriptorSize);
		CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
		m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

		m_commandList->SetPipelineState(pipeline.m_pipelineState.Get());
		m_commandList->SetGraphicsRootSignature(pipeline.m_rootSignature.Get());
		m_commandList->SetGraphicsRootDescriptorTable(0, m_srvHeap->GetGPUDescriptorHandleForHeapStart());
		CD3DX12_GPU_DESCRIPTOR_HANDLE cbvSrvHandleo(m_srvHeap->GetGPUDescriptorHandleForHeapStart(), 1, m_cbvSrvDescriptorSize);
		m_commandList->SetGraphicsRootDescriptorTable(1, cbvSrvHandleo);

		// Record commands.
		const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
		m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
		m_commandList->ClearDepthStencilView(m_dsvHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
		m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_commandList->IASetIndexBuffer(&geometry.m_indexBufferView);
		m_commandList->IASetVertexBuffers(0, 1, &geometry.m_vertexBufferView);
		m_commandList->DrawIndexedInstanced(geometry.m_numIndices, 1, 0, 0, 0);

		m_commandList->SetPipelineState(pipelineMT.m_pipelineState.Get());
		m_commandList->SetGraphicsRootSignature(pipelineMT.m_rootSignature.Get());
		/*m_commandList->SetGraphicsRootDescriptorTable(0, m_srvHeap->GetGPUDescriptorHandleForHeapStart());
		CD3DX12_GPU_DESCRIPTOR_HANDLE cbvSrvHandle2(m_srvHeap->GetGPUDescriptorHandleForHeapStart(), 1, m_cbvSrvDescriptorSize);
		m_commandList->SetGraphicsRootDescriptorTable(2, cbvSrvHandle2);
		CD3DX12_GPU_DESCRIPTOR_HANDLE cbvSrvHandle1(m_srvHeap->GetGPUDescriptorHandleForHeapStart(), 2, m_cbvSrvDescriptorSize);
		m_commandList->SetGraphicsRootDescriptorTable(1, cbvSrvHandle1);*/
		planetable.Draw(m_commandList);

		//m_commandList->IASetIndexBuffer(&planegeometry.m_indexBufferView);
		//m_commandList->IASetVertexBuffers(0, 1, &planegeometry.m_vertexBufferView);
		//// m_commandList->DrawInstanced(m_numIndices, 1, 0, 0);
		//m_commandList->DrawIndexedInstanced(planegeometry.m_numIndices, 1, 0, 0, 0);
		planegeometry.Draw(m_commandList);

		// Indicate that the back buffer will now be used to present.
		m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[currentBuffer].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

		ThrowIfFailed(m_commandList->Close());
	}

	void updateUniformBuffers()
	{
		
	}

	void Prepare()
	{
		init();
		setupDescriptorPool();
		PrepareUI();
		//BuildCommandBuffers();
		prepared = true;
	}
	float offset = 0;
	virtual void update(float dt)
	{
		float m_aspectRatio = static_cast<float>(width) / static_cast<float>(height);
		XMFLOAT3 lightpos(0.8, 1.0, 0.8);
		XMFLOAT3 camerapos(0.0, 0.5, 1.5);
		XMFLOAT3 lookatpoint(0.0, 0.0, 0.0);
		XMVECTOR cameradir = XMVector3Normalize(XMVectorSubtract(XMLoadFloat3(&lookatpoint), XMLoadFloat3(&camerapos)));

		// XMFLOAT3 cameradir = XMFLOAT3(0.0,0.0,0.0) - camerapos;//(0.0,0.0,-1.0);
		XMFLOAT3 cameraup(0.0, 1.0, 0.0); 
		XMMATRIX myviewmat = XMMatrixLookToRH(XMLoadFloat3(&camerapos), cameradir, XMLoadFloat3(&cameraup));
		XMMATRIX projmat = XMMatrixPerspectiveFovRH(0.8f, m_aspectRatio, 0.1f, 300.0f);
		cameradir = XMVector3Normalize(XMVectorSubtract(XMLoadFloat3(&lookatpoint), XMLoadFloat3(&lightpos)));
		XMMATRIX lightviewmat = XMMatrixLookToRH(XMLoadFloat3(&lightpos), cameradir, XMLoadFloat3(&cameraup));

		glm::mat4x4 myvm = camera.GetViewMatrix();
		myvm = glm::transpose(myvm);
		auto xmMatrix = XMFLOAT4X4(&myvm[0][0]);

		XMStoreFloat4x4(&m_constantBufferData.mp, XMMatrixTranspose(projmat));
		XMStoreFloat4x4(&m_constantBufferData.mv, XMMatrixTranspose(myviewmat));
		XMStoreFloat4x4(&m_constantBufferData.mlv, XMMatrixTranspose(lightviewmat));

		m_constantBufferData.mv = xmMatrix;

		//   m_constantBufferData2.scale = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		//   m_constantBufferData2.offset = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
		XMStoreFloat4x4(&m_constantBufferData2.mp, XMMatrixTranspose(projmat));
		XMStoreFloat4x4(&m_constantBufferData2.mv, XMMatrixTranspose(lightviewmat));
		//XMStoreFloat4x4(&m_constantBufferData.mvp, XMMatrixTranspose(XMMatrixTranslation(offset, 0.0, 0.0)));

		const float translationSpeed = 0.005f;
		const float offsetBounds = 1.25f;

		/*m_constantBufferData.offset.x += translationSpeed;
		if (m_constantBufferData.offset.x > offsetBounds)
		{
			m_constantBufferData.offset.x = -offsetBounds;
		}*/
		/* XMFLOAT4X4 shit4;
		 XMStoreFloat4x4(&shit4, XMMatrixTranslation(offset, 0.0, 0.0));

		 XMFLOAT4 shit(0.0f,0.0f,0.0f,0.0f);
		 FXMVECTOR KK = { 0.f, 0.f, 0.f, 0.f };;
		 XMVECTOR kk = XMVector4Transform(KK, XMMatrixTranslation(offset, 0.0, 0.0));*/

		 // = kk.x;
		 //XMStoreFloat4(&m_constantBufferData.offset, kk);
		offset += translationSpeed;
		if (offset > offsetBounds)
		{
			offset = -offsetBounds;
		}
		memcpy(m_pCbvDataBegin, &m_constantBufferData, sizeof(m_constantBufferData));
		memcpy(m_pCbvDataBeginOffscreen, &m_constantBufferData2, sizeof(m_constantBufferData2));
	}

	virtual void ViewChanged()
	{
		updateUniformBuffers();
	}

};

VULKAN_EXAMPLE_MAIN()