
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <vector>

#include "D3D12Application.h"
#include "VulkanApplication.h"
#include "Mesh.h"
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

	render::Texture* m_renderTargeto;
	render::Texture* m_depthStencilOffscreen;
	render::RenderPass* offscreenPass;

	render::DescriptorPool* m_rtvoHeap;
	render::DescriptorPool* m_dsvoHeap;
	render::DescriptorPool* m_srvHeap;

	render::Pipeline* pipelineOffscreen;
	render::Pipeline* pipeline;
	render::Pipeline* pipelineMT;

	std::vector<Mesh*> meshesmodel;
	std::vector<Mesh*> meshesplane;

	render::Texture* m_texture;
	render::Texture* m_texture1;

	render::Buffer* m_constantBuffer;
	render::Buffer* m_constantBufferOffscreen;
	SceneConstantBuffer m_constantBufferData;
	SceneConstantBuffer m_constantBufferData2;

	render::DescriptorSet* planetable;
	render::DescriptorSet* modeltableoffscreen;
	render::DescriptorSet* modeltable;

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
		title = "Render Engine First Generic";
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
				meshes.push_back(mesh);
			}
		}
		return meshes;
	}

	void init()
	{	

		m_rtvoHeap = m_device->GetDescriptorPool({ {render::DescriptorType::RTV, 1} },1);
		m_dsvoHeap = m_device->GetDescriptorPool({ {render::DescriptorType::DSV, 1} },1);
		m_srvHeap = m_device->GetDescriptorPool({ {render::DescriptorType::IMAGE_SAMPLER, 3},{render::DescriptorType::UNIFORM_BUFFER, 3} }, 6);

		m_renderTargeto = m_device->GetRenderTarget(TextureWidth, TextureHeight, render::GfxFormat::R8G8B8A8_UNORM, m_srvHeap, m_rtvoHeap, m_commandBuffer, false);
		m_depthStencilOffscreen = m_device->GetRenderTarget(TextureWidth, TextureHeight, render::GfxFormat::D32_FLOAT, m_srvHeap, m_dsvoHeap, m_commandBuffer, true);
		offscreenPass = m_device->GetRenderPass(TextureWidth, TextureHeight, m_renderTargeto, m_depthStencilOffscreen);

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
		props.depthBias = false;
		pipeline = m_device->GetPipeLine(GetAssetFullPath("shaders/d3dexp/shaders.hlsl"), "VSMain", "", "PSMain", &vertexLayout, &mdsl, props, nullptr);
		pipelineMT = m_device->GetPipeLine(GetAssetFullPath("shaders/d3dexp/shaders.hlsl"), "VSMain", "", "PSMainMT", &vertexLayout, &pdsl, props, nullptr);

		std::vector<MeshData*> meshdatas = Load("./../data/models/venus.fbx", glm::vec3(0.0, -0.5, 0.0), 0.03f, &vertexLayout);
		std::vector<MeshData*> meshdatas2 = Load("./../data/models/plane.obj", glm::vec3(0.0, planey, 0.0), 0.1f, &vertexLayout);
		for (auto md : meshdatas)
		{
			meshesmodel.push_back(m_device->GetMesh(md,&vertexLayout,m_commandBuffer));
		}
		for (auto md : meshdatas2)
		{
			meshesplane.push_back(m_device->GetMesh(md, &vertexLayout, m_commandBuffer));
		}

		m_constantBuffer = m_device->GetUniformBuffer(sizeof(SceneConstantBuffer), &m_constantBufferData,m_srvHeap);
		m_constantBufferOffscreen = m_device->GetUniformBuffer(sizeof(SceneConstantBuffer), &m_constantBufferData2, m_srvHeap);

		Texture2DData tdata;
		tdata.LoadFromFile("./../data/textures/compass.jpg", GfxFormat::R8G8B8A8_UNORM);
		m_texture = m_device->GetTexture(&tdata, m_srvHeap, m_commandBuffer);
		Texture2DData tdata2;
		tdata2.LoadFromFile("./../data/textures/PlanksBare0002_1_S.jpg", GfxFormat::R8G8B8A8_UNORM);
		m_texture1 = m_device->GetTexture(&tdata2, m_srvHeap, m_commandBuffer);

		planetable = m_device->GetDescriptorSet(&pdsl, { m_constantBuffer }, { m_texture , m_renderTargeto });
		modeltable = m_device->GetDescriptorSet(&mdsl, { m_constantBuffer }, { m_texture });
		modeltableoffscreen = m_device->GetDescriptorSet(&odsl, { m_constantBufferOffscreen }, {  });
		

		// Close the command list and execute it to begin the initial GPU setup.
		m_commandBuffer->End();
		SubmitOnQueue(m_commandBuffer);
		//ID3D12CommandList* ppCommandLists[] = { m_commandBuffer.m_commandList.Get() };
		//m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		WaitForDevice();

		m_device->FreeLoadStaggingBuffers();

		for (auto md : meshdatas)
		{
			delete md;
		}
		for (auto md : meshdatas2)
		{
			delete md;
		}

	}

	virtual void BuildCommandBuffers()
	{
		m_commandBuffer->Begin();
		
		m_srvHeap->Draw(m_commandBuffer);

		offscreenPass->Begin(m_commandBuffer);
		pipelineOffscreen->Draw(m_commandBuffer);
		modeltableoffscreen->Draw(m_commandBuffer);
		for (auto m : meshesmodel)
		{
			m->Draw(m_commandBuffer);
		}
		for (auto m : meshesplane)
		{
			m->Draw(m_commandBuffer);
		}
		offscreenPass->End(m_commandBuffer);

		m_mainRenderPass->Begin(m_commandBuffer, currentBuffer);
		pipeline->Draw(m_commandBuffer);
		modeltable->Draw(m_commandBuffer);
		for (auto m : meshesmodel)
			m->Draw(m_commandBuffer);
		pipelineMT->Draw(m_commandBuffer);
		planetable->Draw(m_commandBuffer);
		for (auto m : meshesplane)
			m->Draw(m_commandBuffer);
		m_mainRenderPass->End(m_commandBuffer, currentBuffer);

		m_commandBuffer->End();
	}

	void updateUniformBuffers()
	{
		
	}

	void Prepare()
	{
		init();
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

		// XMFLOAT3 cameradir = XMFLOAT3(0.0,0.0,0.0) - camerapos;//(0.0,0.0,-1.0);
		//glm::vec3 cameraup(0.0, 1.0, 0.0);
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

		glm::mat4 depthProjectionMatrix = glm::perspective(0.8f, 1.0f, 0.1f, 300.0f);
		glm::mat4 depthViewMatrix = glm::lookAt(lightpos, glm::vec3(0.0f), glm::vec3(0, 1, 0));
		glm::mat4x4 myvm = camera.GetViewMatrix();
		myvm = glm::transpose(myvm);

		m_constantBufferData.mp = glm::transpose(depthProjectionMatrix);
		m_constantBufferData.mv = myvm;
		m_constantBufferData.mlv = glm::transpose(depthViewMatrix);
		m_constantBufferData2.mp = m_constantBufferData.mp;
		m_constantBufferData2.mv = m_constantBufferData.mlv;

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