
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <vector>

#include "D3D12Application.h"
//#include "D3D12Texture.h"
#include "Mesh.h"
#include "render/directx/D3D12Pipeline.h"
#include "render/directx/D3D12Texture.h"
#include "render/directx/D3D12DescriptorHeap.h"
#include "render/directx/D3D12DescriptorTable.h"
#include "render/directx/D3D12Buffer.h"
#include "render/directx/D3D12RenderPass.h"
#include "render/GraphicsDevice.h"
#include "DXSampleHelper.h"
#include <assimp/Importer.hpp> 
#include <assimp/scene.h>     
#include <assimp/postprocess.h>
#include <assimp/cimport.h>

using Microsoft::WRL::ComPtr;
using namespace engine;
using namespace engine::render;

class VulkanExample : public D3D12Application
{
public:

	static const UINT TextureWidth = 1024;
	static const UINT TextureHeight = 1024;
	static const UINT TexturePixelSize = 4;    // The number of bytes used to represent a pixel in the texture.

	struct Vertex
	{
		glm::vec3 position;
		glm::vec2 uv;
	};

	struct SceneConstantBuffer
	{
		//XMFLOAT4 offset;
	   // float padding[60]; // Padding so the constant buffer is 256-byte aligned.
		glm::mat4x4 mp;
		glm::mat4x4 mv;
		glm::mat4x4 mlv;
		//XMFLOAT4 scale;
		//XMFLOAT4 offset;
		float padding[16];
	};
	static_assert((sizeof(SceneConstantBuffer) % 256) == 0, "Constant Buffer size must be 256-byte aligned");

	//ComPtr<ID3D12Resource> m_renderTargeto;
	render::Texture* m_renderTargeto;
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_renderTargetoSRVHandleGPU;
	//ComPtr<ID3D12DescriptorHeap> m_rtvoHeap;
	//ComPtr<ID3D12DescriptorHeap> m_dsvoHeap;
	//ComPtr<ID3D12DescriptorHeap> m_srvHeap;
	//render::D3D12DescriptorHeap m_srvHeap = render::D3D12DescriptorHeap({ {render::DescriptorType::IMAGE_SAMPLER, 3},{render::DescriptorType::UNIFORM_BUFFER, 3} }, 6);
	render::DescriptorPool* m_rtvoHeap;
	render::DescriptorPool* m_dsvoHeap;
	render::DescriptorPool* m_srvHeap;

	//ComPtr<ID3D12Resource> m_depthStencilOffscreen;
	render::Texture* m_depthStencilOffscreen;

	render::RenderPass* offscreenPass;

	render::Pipeline* pipelineOffscreen;
	render::Pipeline* pipeline;
	render::Pipeline* pipelineMT;
	//Geometry geometry;
	//Geometry planegeometry;
	std::vector<Mesh*> meshesmodel;
	std::vector<Mesh*> meshesplane;
	// ComPtr<ID3D12Resource> m_texture;
	render::Texture* m_texture;
	render::Texture* m_texture1;

	//ComPtr<ID3D12Resource> m_constantBuffer;
	render::Buffer* m_constantBuffer;
	//ComPtr<ID3D12Resource> m_constantBufferOffscreen;
	render::Buffer* m_constantBufferOffscreen;
	SceneConstantBuffer m_constantBufferData;
	SceneConstantBuffer m_constantBufferData2;

	render::DescriptorSet* planetable;
	render::DescriptorSet* modeltableoffscreen;
	render::DescriptorSet* modeltable;

	//void* m_pCbvDataBegin;
	UINT8* m_pCbvDataBeginOffscreen;

	render::VertexLayout vertexLayout = render::VertexLayout({
		render::VERTEX_COMPONENT_POSITION,
		render::VERTEX_COMPONENT_UV
		}, {});

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

	std::string GetAssetFullPath(std::string assetName)
	{
		std::string path = "./../data/";
		return path + assetName;
	}

	std::vector<MeshData*> Load(std::string fileName, glm::vec3 atPosition, float scale, VertexLayout *vlayout)
	{
		std::vector<MeshData*> meshes;

		Assimp::Importer Importer;
		const aiScene* pScene;
		static const int defaultFlags = aiProcess_FlipWindingOrder | aiProcess_Triangulate | aiProcess_PreTransformVertices | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals;
		pScene = Importer.ReadFile(fileName.c_str(), defaultFlags);
		if (!pScene)
		{
			std::string error = Importer.GetErrorString();
		}
		
		if (pScene)
		{

			//  parts.clear();
			//  parts.resize(pScene->mNumMeshes);
			const aiVector3D Zero3D(0.0f, 0.0f, 0.0f);

			// Load meshes
			for (unsigned int i = 0; i < pScene->mNumMeshes; i++)
			{
				std::vector<Vertex> vertices;
				std::vector<UINT> modelindices;
				const aiMesh* paiMesh = pScene->mMeshes[i];
				for (unsigned int j = 0; j < paiMesh->mNumVertices; j++)
				{
					const aiVector3D* pPos = &(paiMesh->mVertices[j]);
					const aiVector3D* pNormal = paiMesh->HasNormals() ? &(paiMesh->mNormals[j]) : &Zero3D;
					const aiVector3D* pTexCoord = (paiMesh->HasTextureCoords(0)) ? &(paiMesh->mTextureCoords[0][j]) : &Zero3D;
					Vertex v;
					v.position.x = pPos->x * scale + atPosition.x;
					v.position.y = pPos->y * scale + atPosition.y;
					v.position.z = pPos->z * scale + atPosition.z;
					v.uv.x = pTexCoord->x;
					v.uv.y = pTexCoord->y;
					vertices.push_back(v);
				}

				int indexBase = 0;
				for (unsigned int j = 0; j < paiMesh->mNumFaces; j++)
				{
					const aiFace& Face = paiMesh->mFaces[j];
					if (Face.mNumIndices != 3)
						continue;
					modelindices.push_back(indexBase + Face.mIndices[0]);
					modelindices.push_back(indexBase + Face.mIndices[1]);
					modelindices.push_back(indexBase + Face.mIndices[2]);
					// geometry->m_indices[index_index++] = indexBase + Face.mIndices[1];
				   //  geometry->m_indices[index_index++] = indexBase + Face.mIndices[2];
					 //parts[i].indexCount += 3;
					 //geometry->m_indexCount += 3;
				}
				MeshData* mesh = new MeshData();
				mesh->m_verticesSize = vertices.size() * sizeof(Vertex);
				mesh->m_vertexCount = paiMesh->mNumVertices;
				mesh->m_indexCount = modelindices.size();
				uint32_t floatsno = mesh->m_verticesSize / sizeof(float);
				mesh->m_vertices = new float[floatsno];
				mesh->m_indices = new uint32_t[mesh->m_indexCount];
				memcpy(mesh->m_vertices, vertices.data(), mesh->m_verticesSize);
				memcpy(mesh->m_indices, modelindices.data(), mesh->m_indexCount * sizeof(uint32_t));
				
			}
		}
		return meshes;
	}

	void init()
	{	
		m_viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
		m_scissorRect = CD3DX12_RECT(0, 0, static_cast<LONG>(width), static_cast<LONG>(height));
		{
			//D3D12_DESCRIPTOR_HEAP_DESC rtvoHeapDesc = {};
			//rtvoHeapDesc.NumDescriptors = 1;
			//rtvoHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			//rtvoHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			//ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvoHeapDesc, IID_PPV_ARGS(&m_rtvoHeap)));

			//// Describe and create a depth stencil view (DSV) descriptor heap.
			//D3D12_DESCRIPTOR_HEAP_DESC dsvoHeapDesc = {};
			//dsvoHeapDesc.NumDescriptors = 1;
			//dsvoHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
			//dsvoHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			//ThrowIfFailed(m_device->CreateDescriptorHeap(&dsvoHeapDesc, IID_PPV_ARGS(&m_dsvoHeap)));
			m_rtvoHeap = m_device->GetDescriptorPool({ {render::DescriptorType::RTV, 1} },1);
			m_dsvoHeap = m_device->GetDescriptorPool({ {render::DescriptorType::DSV, 1} },1);
			m_srvHeap = m_device->GetDescriptorPool({ {render::DescriptorType::IMAGE_SAMPLER, 3},{render::DescriptorType::UNIFORM_BUFFER, 3} }, 6);

			// Describe and create a shader resource view (SRV) heap for the texture.
			/*D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
			srvHeapDesc.NumDescriptors = 6;
			srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			ThrowIfFailed(m_device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_srvHeap)));*/
			//m_srvHeap.Create(m_device);

			// Describe and create a constant buffer view (CBV) descriptor heap.
			// Flags indicate that this descriptor heap can be bound to the pipeline 
			// and that descriptors contained in it can be referenced by a root table.
		   /* D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {};
			cbvHeapDesc.NumDescriptors = 1;
			cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			ThrowIfFailed(m_device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&m_cbvHeap)));*/

			//m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			//m_cbvSrvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}
		//planetable.Create(m_device, m_srvHeap, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		// Create frame resources.
		{
			//float width = 512;
			//float height = 512;
			//DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
			//CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvoHeap->GetCPUDescriptorHandleForHeapStart());
			////m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
			//auto const heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

			//const D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(format,
			//	static_cast<UINT64>(TextureWidth),
			//	static_cast<UINT>(TextureHeight),
			//	1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

			//D3D12_CLEAR_VALUE clearValue = { format, {} };
			//// memcpy(clearValue.Color, m_clearColor, sizeof(clearValue.Color));

			//D3D12_RESOURCE_STATES m_state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

			//// Create a render target
			//ThrowIfFailed(
			//	m_device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE,
			//		&desc,
			//		m_state, &clearValue,
			//		IID_PPV_ARGS(&m_renderTargeto))
			//);

			// SetDebugObjectName(m_resource.Get(), L"RenderTexture RT");

			 // Create RTV.
			//m_device->CreateRenderTargetView(m_renderTargeto.Get(), nullptr, rtvHandle);

			//CD3DX12_CPU_DESCRIPTOR_HANDLE cbvSrvHandle;// (m_srvHeap->GetCPUDescriptorHandleForHeapStart(), 2, m_cbvSrvDescriptorSize);
			//CD3DX12_GPU_DESCRIPTOR_HANDLE cbvSrvHandleGPU;// (m_srvHeap->GetGPUDescriptorHandleForHeapStart(), 2, m_cbvSrvDescriptorSize);
			//m_srvHeap.GetAvailableHandles(cbvSrvHandle, m_renderTargetoSRVHandleGPU);
			// Create SRV.
			//m_device->CreateShaderResourceView(m_renderTargeto.Get(), nullptr, cbvSrvHandle);

			//m_renderTargeto.Create(m_device.Get(), TextureWidth, TextureHeight, render::GfxFormat::R8G8B8A8_UNORM, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			//m_renderTargeto.CreateDescriptor(m_device.Get(), cbvSrvHandle, m_renderTargetoSRVHandleGPU, rtvHandle);

			m_renderTargeto = m_device->GetRenderTarget(TextureWidth, TextureHeight, render::GfxFormat::R8G8B8A8_UNORM, m_srvHeap, m_rtvoHeap, m_commandBuffer, false);
			m_depthStencilOffscreen = m_device->GetRenderTarget(TextureWidth, TextureHeight, render::GfxFormat::D32_FLOAT, m_srvHeap, m_dsvoHeap, m_commandBuffer, true);
			
			//planetable.AddEntry(m_renderTargetoSRVHandleGPU, 1);
		}
		// Create the depth stencil view.
		{
	/*		D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
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
				&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, TextureWidth, TextureHeight, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
				D3D12_RESOURCE_STATE_DEPTH_WRITE,
				&depthOptimizedClearValue,
				IID_PPV_ARGS(&m_depthStencilOffscreen)
			));

			NAME_D3D12_OBJECT(m_depthStencilOffscreen);

			m_device->CreateDepthStencilView(m_depthStencilOffscreen.Get(), &depthStencilDesc, m_dsvoHeap->GetCPUDescriptorHandleForHeapStart());*/

			/*D3D12_CPU_DESCRIPTOR_HANDLE emptyCPUHandle{};
			D3D12_GPU_DESCRIPTOR_HANDLE emptyGPUHandle{};
			m_depthStencilOffscreen.Create(m_device.Get(), TextureWidth, TextureHeight, render::GfxFormat::D32_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, D3D12_RESOURCE_STATE_DEPTH_WRITE);
			m_depthStencilOffscreen.CreateDescriptor(m_device.Get(), emptyCPUHandle, emptyGPUHandle, m_dsvoHeap->GetCPUDescriptorHandleForHeapStart());*/
		}

		render::DescriptorSetLayout odsl({ {render::DescriptorType::UNIFORM_BUFFER, render::ShaderStage::VERTEX} });

		render::DescriptorSetLayout mdsl({ 
						{render::DescriptorType::IMAGE_SAMPLER,render::ShaderStage::FRAGMENT } ,
						{render::DescriptorType::UNIFORM_BUFFER,render::ShaderStage::VERTEX }
			});

		render::DescriptorSetLayout pdsl({
						{render::DescriptorType::IMAGE_SAMPLER, render::ShaderStage::FRAGMENT},
						{render::DescriptorType::IMAGE_SAMPLER, render::ShaderStage::FRAGMENT},
						{render::DescriptorType::UNIFORM_BUFFER, render::ShaderStage::VERTEX}
			});

		render::PipelineProperties props;
		props.depthBias = true;
		pipelineOffscreen = m_device->GetPipeLine(GetAssetFullPath("shaders/d3dexp/shaders.hlsl"), "VSMain", "", "PSMainOffscreen", &vertexLayout, &odsl, props, nullptr);
		//pipelineOffscreen.Load(m_device.Get(), GetAssetFullPath(L"shaders/d3dexp/shaders.hlsl"), "VSMain", "PSMainOffscreen", &odsl, props);
		props.depthBias = false;
		pipeline = m_device->GetPipeLine(GetAssetFullPath("shaders/d3dexp/shaders.hlsl"), "VSMain", "", "PSMain", &vertexLayout, &mdsl, props, nullptr);
		//pipeline.Load(m_device.Get(), GetAssetFullPath(L"shaders/d3dexp/shaders.hlsl"), "VSMain", "PSMain", &mdsl, props);
		//pipelineMT.Load(m_device.Get(), GetAssetFullPath(L"shaders/d3dexp/shaders.hlsl"), "VSMain", "PSMainMT", &pdsl, props);
		pipelineMT = m_device->GetPipeLine(GetAssetFullPath("shaders/d3dexp/shaders.hlsl"), "VSMain", "", "PSMainMT", &vertexLayout, &pdsl, props, nullptr);


		// Create the command list.
		//ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), pipeline.m_pipelineState.Get(), IID_PPV_ARGS(&m_commandList)));

		std::vector<MeshData*> meshdatas = Load("./../data/models/venus.fbx", glm::vec3(0.0, -0.7, 0.0), 0.03f, &vertexLayout);
		std::vector<MeshData*> meshdatas2 = Load("./../data/models/plane.obj", glm::vec3(0.0, planey, 0.0), 0.1f, &vertexLayout);
		for (auto md : meshdatas)
		{
			meshesmodel.push_back(m_device->GetMesh(md,&vertexLayout,m_commandBuffer));
		}
		for (auto md : meshdatas2)
		{
			meshesplane.push_back(m_device->GetMesh(md, &vertexLayout, m_commandBuffer));
		}

		//geometry.Load(m_device.Get(), "./../data/models/venus.fbx", XMFLOAT3(0.0, -0.7, 0.0), 0.03f, m_commandBuffer.m_commandList.Get());
		//planegeometry.Load(m_device.Get(), "./../data/models/plane.obj", XMFLOAT3(0.0, planey, 0.0), 0.1f, m_commandBuffer.m_commandList.Get());

		// Create the constant buffer.
		const UINT64 constantBufferSize = sizeof(SceneConstantBuffer);    // CB size is required to be 256-byte aligned.
		{
			//CD3DX12_CPU_DESCRIPTOR_HANDLE cbvSrvHandle(m_srvHeap->GetCPUDescriptorHandleForHeapStart(), 1, m_cbvSrvDescriptorSize);
			//CD3DX12_GPU_DESCRIPTOR_HANDLE cbvSrvHandleGPU(m_srvHeap->GetGPUDescriptorHandleForHeapStart(), 1, m_cbvSrvDescriptorSize);
			//CD3DX12_CPU_DESCRIPTOR_HANDLE cbvSrvHandle;
			//CD3DX12_GPU_DESCRIPTOR_HANDLE cbvSrvHandleGPU;
			//m_srvHeap.GetAvailableHandles(cbvSrvHandle, cbvSrvHandleGPU);
			/*ThrowIfFailed(m_device->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize),
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&m_constantBuffer)));

			CD3DX12_CPU_DESCRIPTOR_HANDLE cbvSrvHandle(m_srvHeap->GetCPUDescriptorHandleForHeapStart(), 1, m_cbvSrvDescriptorSize);
			CD3DX12_GPU_DESCRIPTOR_HANDLE cbvSrvHandleGPU(m_srvHeap->GetGPUDescriptorHandleForHeapStart(), 1, m_cbvSrvDescriptorSize);
			// Describe and create a constant buffer view.
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
			cbvDesc.BufferLocation = m_constantBuffer->GetGPUVirtualAddress();
			cbvDesc.SizeInBytes = constantBufferSize;
			// m_device->CreateConstantBufferView(&cbvDesc, m_cbvHeap->GetCPUDescriptorHandleForHeapStart());
			m_device->CreateConstantBufferView(&cbvDesc, cbvSrvHandle);

			// Map and initialize the constant buffer. We don't unmap this until the
			// app closes. Keeping things mapped for the lifetime of the resource is okay.
			CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
			ThrowIfFailed(m_constantBuffer->Map(0, &readRange, &m_pCbvDataBegin));
			memcpy(m_pCbvDataBegin, &m_constantBufferData, sizeof(m_constantBufferData));*/

			//m_constantBuffer.Create(m_device.Get(), sizeof(SceneConstantBuffer), &m_constantBufferData, cbvSrvHandle, cbvSrvHandleGPU);
			m_constantBuffer = m_device->GetUniformBuffer(sizeof(SceneConstantBuffer), &m_constantBufferData,m_srvHeap);

			//planetable.AddEntry(cbvSrvHandleGPU, 2);
			//modeltable.AddEntry(cbvSrvHandleGPU, 1);
		}

		//the other constant buffer
		{
			/*CD3DX12_CPU_DESCRIPTOR_HANDLE cbvSrvHandle;
			CD3DX12_GPU_DESCRIPTOR_HANDLE cbvSrvHandleGPU;
			m_srvHeap.GetAvailableHandles(cbvSrvHandle, cbvSrvHandleGPU);*/
			//ThrowIfFailed(m_device->CreateCommittedResource(
			//	&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			//	D3D12_HEAP_FLAG_NONE,
			//	&CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize),
			//	D3D12_RESOURCE_STATE_GENERIC_READ,
			//	nullptr,
			//	IID_PPV_ARGS(&m_constantBufferOffscreen)));

			////CD3DX12_CPU_DESCRIPTOR_HANDLE cbvSrvHandle(m_srvHeap->GetCPUDescriptorHandleForHeapStart(), 4, m_cbvSrvDescriptorSize);
			
			//// Describe and create a constant buffer view.
			//D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
			//cbvDesc.BufferLocation = m_constantBufferOffscreen->GetGPUVirtualAddress();
			//cbvDesc.SizeInBytes = constantBufferSize;
			//// m_device->CreateConstantBufferView(&cbvDesc, m_cbvHeap->GetCPUDescriptorHandleForHeapStart());
			//m_device->CreateConstantBufferView(&cbvDesc, cbvSrvHandle);
			////m_device->CreateConstantBufferView(&cbvDesc, cbvSrvHandle);

			//// Map and initialize the constant buffer. We don't unmap this until the
			//// app closes. Keeping things mapped for the lifetime of the resource is okay.
			//CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
			//ThrowIfFailed(m_constantBufferOffscreen->Map(0, &readRange, reinterpret_cast<void**>(&m_pCbvDataBeginOffscreen)));
			//memcpy(m_pCbvDataBeginOffscreen, &m_constantBufferData2, sizeof(m_constantBufferData2));
			//m_constantBufferOffscreen.Create(m_device.Get(), sizeof(SceneConstantBuffer), &m_constantBufferData2, cbvSrvHandle, cbvSrvHandleGPU);
			m_constantBufferOffscreen = m_device->GetUniformBuffer(sizeof(SceneConstantBuffer), &m_constantBufferData2, m_srvHeap);

			//modeltableoffscreen.AddEntry(cbvSrvHandleGPU, 0);
		}

		/*CD3DX12_CPU_DESCRIPTOR_HANDLE cbvSrvHandlet;
		CD3DX12_GPU_DESCRIPTOR_HANDLE cbvSrvHandleGPUt;
		m_srvHeap.GetAvailableHandles(cbvSrvHandlet, cbvSrvHandleGPUt);
		m_texture.Load(m_device.Get(), "./../data/textures/compass.jpg", m_commandBuffer.m_commandList.Get(), cbvSrvHandlet, cbvSrvHandleGPUt);*/
		Texture2DData tdata;
		tdata.LoadFromFile("./../data/textures/compass.jpg", GfxFormat::R8G8B8A8_UNORM);
		m_texture = m_device->GetTexture(&tdata, m_srvHeap, m_commandBuffer);
		
		//CD3DX12_GPU_DESCRIPTOR_HANDLE cbvSrvHandleGPU(m_srvHeap->GetGPUDescriptorHandleForHeapStart(), 0, m_cbvSrvDescriptorSize);
		//modeltable.AddEntry(cbvSrvHandleGPUt, 0);
		//planetable.AddEntry(cbvSrvHandleGPUt, 0);
		/*CD3DX12_CPU_DESCRIPTOR_HANDLE cbvSrvHandle1(m_srvHeap->GetCPUDescriptorHandleForHeapStart(), 3, m_cbvSrvDescriptorSize);
		CD3DX12_GPU_DESCRIPTOR_HANDLE cbvSrvHandleGPU1(m_srvHeap->GetGPUDescriptorHandleForHeapStart(), 3, m_cbvSrvDescriptorSize);*/
		//CD3DX12_CPU_DESCRIPTOR_HANDLE cbvSrvHandle1;
		//CD3DX12_GPU_DESCRIPTOR_HANDLE cbvSrvHandleGPU1;
		//m_srvHeap.GetAvailableHandles(cbvSrvHandle1, cbvSrvHandleGPU1);
		//m_texture1.Load(m_device.Get(), "./../data/textures/PlanksBare0002_1_S.jpg", m_commandBuffer.m_commandList.Get(), cbvSrvHandle1, cbvSrvHandleGPU1);
		Texture2DData tdata2;
		tdata.LoadFromFile("./../data/textures/PlanksBare0002_1_S.jpg", GfxFormat::R8G8B8A8_UNORM);
		m_texture1 = m_device->GetTexture(&tdata, m_srvHeap, m_commandBuffer);

		planetable = m_device->GetDescriptorSet(&pdsl, { m_constantBuffer }, { m_texture , m_renderTargeto });
		modeltable = m_device->GetDescriptorSet(&pdsl, { m_constantBuffer }, { m_texture });
		modeltableoffscreen = m_device->GetDescriptorSet(&pdsl, { m_constantBufferOffscreen }, {  });

		//planetable.Create({ {m_texture.m_GPUHandle,0}, {m_renderTargetoSRVHandleGPU,1},{m_constantBuffer.m_GPUHandle,2} });
		//planetable.Create(&pdsl, { m_constantBuffer.m_GPUHandle }, { m_texture.m_GPUHandle, m_renderTargetoSRVHandleGPU });
		////modeltable.Create({ {m_texture.m_GPUHandle,0}, {m_constantBuffer.m_GPUHandle,1} });
		//modeltable.Create(&mdsl, { m_constantBuffer.m_GPUHandle }, { m_texture.m_GPUHandle });
		////modeltableoffscreen.Create({ {m_constantBufferOffscreen.m_GPUHandle,0} });
		//modeltableoffscreen.Create(&odsl, {m_constantBufferOffscreen.m_GPUHandle},{});

		//offscreenPass.Create(TextureWidth, TextureHeight, m_renderTargeto.m_CPURTVHandle, m_depthStencilOffscreen.m_CPURTVHandle, m_renderTargeto.m_texture.Get());
		offscreenPass = m_device->GetRenderPass(TextureWidth, TextureHeight, m_renderTargeto, m_depthStencilOffscreen);

		// Close the command list and execute it to begin the initial GPU setup.
		//ThrowIfFailed(m_commandList->Close());
		m_commandBuffer->End();
		SubmitOnQueue(m_commandBuffer);
		//ID3D12CommandList* ppCommandLists[] = { m_commandBuffer.m_commandList.Get() };
		//m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		WaitForPreviousFrame();

		//m_texture->FreeRamData();//GEORGE TODO
		//m_texture1.FreeRamData();
		//geometry.FreeRamData();
		//planegeometry.FreeRamData();

		for (auto md : meshdatas)
		{
			delete md;
		}
		for (auto md : meshdatas2)
		{
			delete md;
		}

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
		D3D12CommandBuffer* d3dcommandBuffer = dynamic_cast<D3D12CommandBuffer*>(m_commandBuffer);
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList = d3dcommandBuffer->m_commandList;
		// Command list allocators can only be reset when the associated 
	// command lists have finished execution on the GPU; apps should use 
	// fences to determine GPU execution progress.
		//ThrowIfFailed(m_commandBuffer.m_commandAllocator->Reset());

		// However, when ExecuteCommandList() is called on a particular command 
		// list, that command list can then be reset at any time and must be before 
		// re-recording.
		
		//ThrowIfFailed(m_commandList->Reset(m_commandBuffer.m_commandAllocator.Get(), pipelineOffscreen.m_pipelineState.Get()));
		m_commandBuffer->Begin();

		//m_commandList->SetPipelineState(pipelineOffscreen.m_pipelineState.Get());
		//m_commandList->SetGraphicsRootSignature(pipelineOffscreen.m_rootSignature.Get());
		pipelineOffscreen->Draw(m_commandBuffer);

		D3D12DescriptorHeap* dHeap = dynamic_cast<D3D12DescriptorHeap*>(m_srvHeap);
		ID3D12DescriptorHeap* ppHeaps[] = { dHeap->m_heap.Get() };
		m_commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

		offscreenPass->Begin(m_commandBuffer);

		// m_commandList->SetGraphicsRootDescriptorTable(0, m_srvHeap->GetGPUDescriptorHandleForHeapStart());
		//CD3DX12_GPU_DESCRIPTOR_HANDLE cbvSrvHandle(m_srvHeap->GetGPUDescriptorHandleForHeapStart(), 4, m_cbvSrvDescriptorSize);
		//m_commandList->SetGraphicsRootDescriptorTable(0, cbvSrvHandle);
		modeltableoffscreen->Draw(m_commandBuffer);
		//CD3DX12_GPU_DESCRIPTOR_HANDLE cbvSrvHandle1(m_srvHeap->GetGPUDescriptorHandleForHeapStart(), 3, m_cbvSrvDescriptorSize);
		//m_commandList->SetGraphicsRootDescriptorTable(1, cbvSrvHandle1);
		//m_commandList->RSSetViewports(1, &m_viewport);
		//m_commandList->RSSetScissorRects(1, &m_scissorRect);

		// Indicate that the back buffer will be used as a render target.
		//m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargeto.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));//george rly dono
		/*m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargeto.m_texture.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));
		CD3DX12_VIEWPORT viewport(0.0f, 0.0f, static_cast<float>(TextureWidth), static_cast<float>(TextureHeight));
		CD3DX12_RECT   scissorRect(0, 0, static_cast<LONG>(TextureWidth), static_cast<LONG>(TextureHeight));
		m_commandList->RSSetViewports(1, &viewport);
		m_commandList->RSSetScissorRects(1, &scissorRect);
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvoHandle(m_rtvoHeap->GetCPUDescriptorHandleForHeapStart());
		CD3DX12_CPU_DESCRIPTOR_HANDLE dsvoHandle(m_dsvoHeap->GetCPUDescriptorHandleForHeapStart());
		m_commandList->OMSetRenderTargets(1, &rtvoHandle, FALSE, &dsvoHandle);
		const float clearColoro[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		m_commandList->ClearRenderTargetView(rtvoHandle, clearColoro, 0, nullptr);
		m_commandList->ClearDepthStencilView(m_dsvoHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);*/
		

		for (auto m : meshesmodel)
		{
			m->Draw(m_commandBuffer);
		}
		for (auto m : meshesplane)
		{
			m->Draw(m_commandBuffer);
		}
		/*m_commandList->IASetIndexBuffer(&geometry.m_indexBufferView);
		m_commandList->IASetVertexBuffers(0, 1, &geometry.m_vertexBufferView);
		m_commandList->DrawIndexedInstanced(geometry.m_numIndices, 1, 0, 0, 0);*/
		//geometry.Draw(m_commandList);
		/*m_commandList->IASetIndexBuffer(&planegeometry.m_indexBufferView);
		m_commandList->IASetVertexBuffers(0, 1, &planegeometry.m_vertexBufferView);
		m_commandList->DrawIndexedInstanced(planegeometry.m_numIndices, 1, 0, 0, 0);*/
		//planegeometry.Draw(m_commandList);
		//m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargeto.m_texture.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));//george rly dono
		offscreenPass->End(m_commandBuffer);

		// Indicate that the back buffer will be used as a render target.
		m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[currentBuffer].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

		m_commandList->RSSetViewports(1, &m_viewport);
		m_commandList->RSSetScissorRects(1, &m_scissorRect);
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), currentBuffer, m_rtvDescriptorSize);
		CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
		m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

		//m_commandList->SetPipelineState(pipeline.m_pipelineState.Get());
		//m_commandList->SetGraphicsRootSignature(pipeline.m_rootSignature.Get());
		pipeline->Draw(m_commandBuffer);
		/*m_commandList->SetGraphicsRootDescriptorTable(0, m_srvHeap->GetGPUDescriptorHandleForHeapStart());
		CD3DX12_GPU_DESCRIPTOR_HANDLE cbvSrvHandleo(m_srvHeap->GetGPUDescriptorHandleForHeapStart(), 1, m_cbvSrvDescriptorSize);
		m_commandList->SetGraphicsRootDescriptorTable(1, cbvSrvHandleo);*/
		//modeltable.Draw(m_commandList);
		modeltable->Draw(m_commandBuffer);

		// Record commands.
		const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
		m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
		m_commandList->ClearDepthStencilView(m_dsvHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
		m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		/*m_commandList->IASetIndexBuffer(&geometry.m_indexBufferView);
		m_commandList->IASetVertexBuffers(0, 1, &geometry.m_vertexBufferView);
		m_commandList->DrawIndexedInstanced(geometry.m_numIndices, 1, 0, 0, 0);*/
		//geometry.Draw(m_commandList);
		for (auto m : meshesmodel)
			m->Draw(m_commandBuffer);

		//m_commandList->SetPipelineState(pipelineMT.m_pipelineState.Get());
		//m_commandList->SetGraphicsRootSignature(pipelineMT.m_rootSignature.Get());
		pipelineMT->Draw(m_commandBuffer);
		/*m_commandList->SetGraphicsRootDescriptorTable(0, m_srvHeap->GetGPUDescriptorHandleForHeapStart());
		CD3DX12_GPU_DESCRIPTOR_HANDLE cbvSrvHandle2(m_srvHeap->GetGPUDescriptorHandleForHeapStart(), 1, m_cbvSrvDescriptorSize);
		m_commandList->SetGraphicsRootDescriptorTable(2, cbvSrvHandle2);
		CD3DX12_GPU_DESCRIPTOR_HANDLE cbvSrvHandle1(m_srvHeap->GetGPUDescriptorHandleForHeapStart(), 2, m_cbvSrvDescriptorSize);
		m_commandList->SetGraphicsRootDescriptorTable(1, cbvSrvHandle1);*/
		//planetable.Draw(m_commandList);
		planetable->Draw(m_commandBuffer);

		//m_commandList->IASetIndexBuffer(&planegeometry.m_indexBufferView);
		//m_commandList->IASetVertexBuffers(0, 1, &planegeometry.m_vertexBufferView);
		//// m_commandList->DrawInstanced(m_numIndices, 1, 0, 0);
		//m_commandList->DrawIndexedInstanced(planegeometry.m_numIndices, 1, 0, 0, 0);
		//planegeometry.Draw(m_commandList);
		for (auto m : meshesplane)
			m->Draw(m_commandBuffer);

		// Indicate that the back buffer will now be used to present.
		m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[currentBuffer].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

		//ThrowIfFailed(m_commandList->Close());
		m_commandBuffer->End();
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
		glm::vec3 lightpos(0.8, 1.0, 0.8);
		glm::vec3 camerapos(0.0, 0.5, 1.5);
		glm::vec3 lookatpoint(0.0, 0.0, 0.0);
		//XMVECTOR cameradir = XMVector3Normalize(XMVectorSubtract(XMLoadFloat3(&lookatpoint), XMLoadFloat3(&camerapos)));

		//// XMFLOAT3 cameradir = XMFLOAT3(0.0,0.0,0.0) - camerapos;//(0.0,0.0,-1.0);
		//XMFLOAT3 cameraup(0.0, 1.0, 0.0); 
		//XMMATRIX myviewmat = XMMatrixLookToRH(XMLoadFloat3(&camerapos), cameradir, XMLoadFloat3(&cameraup));
		//XMMATRIX projmat = XMMatrixPerspectiveFovRH(0.8f, m_aspectRatio, 0.1f, 300.0f);
		//cameradir = XMVector3Normalize(XMVectorSubtract(XMLoadFloat3(&lookatpoint), XMLoadFloat3(&lightpos)));
		//XMMATRIX lightviewmat = XMMatrixLookToRH(XMLoadFloat3(&lightpos), cameradir, XMLoadFloat3(&cameraup));

		//glm::mat4x4 myvm = camera.GetViewMatrix();
		//myvm = glm::transpose(myvm);
		//auto xmMatrix = XMFLOAT4X4(&myvm[0][0]);

		//XMStoreFloat4x4(&m_constantBufferData.mp, XMMatrixTranspose(projmat));
		//XMStoreFloat4x4(&m_constantBufferData.mv, XMMatrixTranspose(myviewmat));
		//XMStoreFloat4x4(&m_constantBufferData.mlv, XMMatrixTranspose(lightviewmat));

		//m_constantBufferData.mv = xmMatrix;

		////   m_constantBufferData2.scale = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		////   m_constantBufferData2.offset = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
		//XMStoreFloat4x4(&m_constantBufferData2.mp, XMMatrixTranspose(projmat));
		//XMStoreFloat4x4(&m_constantBufferData2.mv, XMMatrixTranspose(lightviewmat));
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
		//memcpy(m_pCbvDataBegin, &m_constantBufferData, sizeof(m_constantBufferData));
		m_constantBuffer->MemCopy(&m_constantBufferData, sizeof(m_constantBufferData));
		m_constantBufferOffscreen->MemCopy(&m_constantBufferData2, sizeof(m_constantBufferData2));
		//memcpy(m_pCbvDataBeginOffscreen, &m_constantBufferData2, sizeof(m_constantBufferData2));
	}

	virtual void ViewChanged()
	{
		updateUniformBuffers();
	}

};

VULKAN_EXAMPLE_MAIN()