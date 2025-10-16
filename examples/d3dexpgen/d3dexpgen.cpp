
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

	/*struct Vertex
	{
		glm::vec3 position;
		glm::vec2 uv;
	};*/

	struct SceneConstantBuffer
	{
		glm::mat4x4 mp;
		glm::mat4x4 mv;
		glm::mat4x4 mlv;
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

	render::VertexLayout* vertexLayout = nullptr;
		/*render::VertexLayout({
		render::VERTEX_COMPONENT_POSITION,
		render::VERTEX_COMPONENT_UV
		}, {});*/

	float planey = -0.5f;

	VulkanExample() : D3D12Application(true)
	{
		zoom = -3.75f;
		rotationSpeed = 0.5f;
		rotation = glm::vec3(15.0f, 0.f, 0.0f);
		title = "Render Engine First Generic";
		settings.overlay = true;
		camera.movementSpeed = 20.5f;
		camera.SetFlipY(false);
		camera.SetPerspective(60.0f, (float)width / (float)height, 1.0f, 30.0f);
		camera.SetRotation(glm::vec3(0.0f, 0.0f, 0.0f));
		camera.SetPosition(glm::vec3(0.8, 0.0, -1.0));
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
			const aiVector3D Zero3D(0.0f, 0.0f, 0.0f);

			// Load meshes
			for (unsigned int i = 0; i < pScene->mNumMeshes; i++)
			{
				float verticesSize = pScene->mMeshes[i]->mNumVertices * vlayout->GetVertexSize(0) / sizeof(float);
				//geometry->m_vertices = new float[geometry->m_verticesSize];

				std::vector<float> vertices(verticesSize);
				std::vector<UINT> modelindices;
				const aiMesh* paiMesh = pScene->mMeshes[i];
				int vertex_index = 0;
				aiColor3D pColor(0.f, 0.f, 0.f);
				for (unsigned int j = 0; j < paiMesh->mNumVertices; j++)
				{
					const aiVector3D* pPos = &(paiMesh->mVertices[j]);
					const aiVector3D* pNormal = paiMesh->HasNormals() ? &(paiMesh->mNormals[j]) : &Zero3D;
					const aiVector3D* pTexCoord = (paiMesh->HasTextureCoords(0)) ? &(paiMesh->mTextureCoords[0][j]) : &Zero3D;
					const aiVector3D* pTangent = (paiMesh->HasTangentsAndBitangents()) ? &(paiMesh->mTangents[j]) : &Zero3D;
					const aiVector3D* pBiTangent = (paiMesh->HasTangentsAndBitangents()) ? &(paiMesh->mBitangents[j]) : &Zero3D;

					for (auto& component : vlayout->m_components[0])
					{
						switch (component) {
						case render::VERTEX_COMPONENT_POSITION:
							vertices[vertex_index++] = atPosition.x + pPos->x * scale;
							vertices[vertex_index++] = atPosition.y + pPos->y * scale;
							vertices[vertex_index++] = atPosition.z + pPos->z * scale;
							break;
						case render::VERTEX_COMPONENT_NORMAL:
							vertices[vertex_index++] = pNormal->x ;
							vertices[vertex_index++] = pNormal->y ;
							vertices[vertex_index++] = pNormal->z ;
							break;
						case render::VERTEX_COMPONENT_UV:
							vertices[vertex_index++] = pTexCoord->x ;
							vertices[vertex_index++] = pTexCoord->y ;
							break;
						case render::VERTEX_COMPONENT_COLOR:
							vertices[vertex_index++] = pColor.r;
							vertices[vertex_index++] = pColor.g;
							vertices[vertex_index++] = pColor.b;
							break;
						case render::VERTEX_COMPONENT_TANGENT:
							vertices[vertex_index++] = pTangent->x;
							vertices[vertex_index++] = pTangent->y;
							vertices[vertex_index++] = pTangent->z;
							break;
						case render::VERTEX_COMPONENT_BITANGENT:
							vertices[vertex_index++] = pBiTangent->x;
							vertices[vertex_index++] = pBiTangent->y;
							vertices[vertex_index++] = pBiTangent->z;
							break;
							// Dummy components for padding
						case render::VERTEX_COMPONENT_DUMMY_FLOAT:
							vertices[vertex_index++] = 0.0f;
							break;
						case render::VERTEX_COMPONENT_DUMMY_VEC4:
							vertices[vertex_index++] = 0.0f;
							vertices[vertex_index++] = 0.0f;
							vertices[vertex_index++] = 0.0f;
							vertices[vertex_index++] = 0.0f;
							break;
						};
					}

					/*v.position.x = pPos->x * scale + atPosition.x;
					v.position.y = pPos->y * scale + atPosition.y;
					v.position.z = pPos->z * scale + atPosition.z;
					v.uv.x = pTexCoord->x;
					v.uv.y = pTexCoord->y;
					vertices.push_back(v);*/
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
				mesh->m_verticesSize = vertices.size() * sizeof(float);
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
		if(m_loadingCommandBuffer)
			m_loadingCommandBuffer->Begin();

		m_rtvoHeap = m_device->GetDescriptorPool({ {render::DescriptorType::RTV, 1} },1);
		m_dsvoHeap = m_device->GetDescriptorPool({ {render::DescriptorType::DSV, 1} },1);
		m_srvHeap = m_device->GetDescriptorPool({ {render::DescriptorType::IMAGE_SAMPLER, 3},{render::DescriptorType::UNIFORM_BUFFER, 3} }, 6);

		m_renderTargeto = m_device->GetRenderTarget(TextureWidth, TextureHeight, render::GfxFormat::R8G8B8A8_UNORM, m_srvHeap, m_rtvoHeap, m_loadingCommandBuffer);
		m_depthStencilOffscreen = m_device->GetDepthRenderTarget(TextureWidth, TextureHeight, render::GfxFormat::D32_FLOAT, m_srvHeap, m_dsvoHeap, m_loadingCommandBuffer, false, true);
		offscreenPass = m_device->GetRenderPass(TextureWidth, TextureHeight, m_renderTargeto, m_depthStencilOffscreen);

		render::DescriptorSetLayout* odsl = m_device->GetDescriptorSetLayout({ {render::DescriptorType::UNIFORM_BUFFER, render::ShaderStage::VERTEX} });

		render::DescriptorSetLayout* mdsl = m_device->GetDescriptorSetLayout({
						{render::DescriptorType::IMAGE_SAMPLER,render::ShaderStage::FRAGMENT } ,
						{render::DescriptorType::UNIFORM_BUFFER,render::ShaderStage::VERTEX }
			});

		render::DescriptorSetLayout* pdsl = m_device->GetDescriptorSetLayout({
						{render::DescriptorType::UNIFORM_BUFFER, render::ShaderStage::VERTEX},
						{render::DescriptorType::IMAGE_SAMPLER, render::ShaderStage::FRAGMENT},
						{render::DescriptorType::IMAGE_SAMPLER, render::ShaderStage::FRAGMENT}
			});

		vertexLayout = m_device->GetVertexLayout({
		render::VERTEX_COMPONENT_POSITION,
		render::VERTEX_COMPONENT_UV
			}, {});

		std::vector<MeshData*> meshdatas = Load("./../data/models/venus.fbx", glm::vec3(0.0, -0.5, 0.0), 0.3f, vertexLayout);
		std::vector<MeshData*> meshdatas2 = Load("./../data/models/plane.obj", glm::vec3(0.0, planey, 0.0), 1.0f, vertexLayout);
		for (auto md : meshdatas)
		{
			meshesmodel.push_back(m_device->GetMesh(md, vertexLayout, m_loadingCommandBuffer));
		}
		for (auto md : meshdatas2)
		{
			meshesplane.push_back(m_device->GetMesh(md, vertexLayout, m_loadingCommandBuffer));
		}

		m_constantBuffer = m_device->GetUniformBuffer(sizeof(SceneConstantBuffer), &m_constantBufferData,m_srvHeap);
		m_constantBufferOffscreen = m_device->GetUniformBuffer(sizeof(SceneConstantBuffer), &m_constantBufferData2, m_srvHeap);

		Texture2DData tdata;
		tdata.LoadFromFile("./../data/textures/compass.jpg", GfxFormat::R8G8B8A8_UNORM);
		m_texture = m_device->GetTexture(&tdata, m_srvHeap, m_loadingCommandBuffer);
		Texture2DData tdata2;
		tdata2.LoadFromFile("./../data/textures/PlanksBare0002_1_S.jpg", GfxFormat::R8G8B8A8_UNORM);
		m_texture1 = m_device->GetTexture(&tdata2, m_srvHeap, m_loadingCommandBuffer);

		planetable = m_device->GetDescriptorSet(pdsl, m_srvHeap,{ m_constantBuffer }, { m_texture , m_renderTargeto });
		//modeltable = m_device->GetDescriptorSet(mdsl, m_srvHeap,{ m_constantBuffer }, { m_texture });
		modeltableoffscreen = m_device->GetDescriptorSet(odsl, m_srvHeap,{ m_constantBufferOffscreen }, {  });

		render::PipelineProperties props;
		props.depthBias = true;
		pipelineOffscreen = m_device->GetPipeline(GetAssetFullPath("shaders/d3dexp/shaders.hlsl"), "VSMain", "", "PSMainOffscreen", vertexLayout, odsl, props, m_mainRenderPass);
		//pipelineOffscreen = m_device->GetPipeline(GetAssetFullPath("shaders/d3dexp/offscreen.vert.spv"), "VSMain", GetAssetFullPath("shaders/d3dexp/offscreenvariancecolor.frag.spv"), "PSMainOffscreen",vertexLayout, odsl, props, offscreenPass);
		props.depthBias = false;
		props.vertexConstantBlockSize = sizeof(glm::vec4);
		pipelineMT = m_device->GetPipeline(GetAssetFullPath("shaders/d3dexp/shaders.hlsl"), "VSMain", "", "PSMainMT", vertexLayout, pdsl, props, nullptr);
		//pipelineMT = m_device->GetPipeline(GetAssetFullPath("shaders/d3dexp/scene.vert.spv"), "VSMain", GetAssetFullPath("shaders/d3dexp/scene.frag.spv"), "PSMainMT", vertexLayout, pdsl, props, m_mainRenderPass);

		if (m_loadingCommandBuffer)	
		{
			// Close the command list and execute it to begin the initial GPU setup.
			m_loadingCommandBuffer->End();
			SubmitOnQueue(m_loadingCommandBuffer);
		}

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
		int i = currentBuffer;
		for (int32_t i = 0; i < m_drawCommandBuffers.size(); ++i)
		{
			m_drawCommandBuffers[i]->Begin();

			m_srvHeap->Draw(m_drawCommandBuffers[i]);

			offscreenPass->Begin(m_drawCommandBuffers[i]);
			pipelineOffscreen->Draw(m_drawCommandBuffers[i]);
			modeltableoffscreen->Draw(m_drawCommandBuffers[i], pipelineOffscreen);
			for (auto m : meshesmodel)
			{
				m->Draw(m_drawCommandBuffers[i]);
			}
			for (auto m : meshesplane)
			{
				m->Draw(m_drawCommandBuffers[i]);
			}
			offscreenPass->End(m_drawCommandBuffers[i]);

			m_mainRenderPass->Begin(m_drawCommandBuffers[i], i);
			//pipeline->Draw(m_commandBuffers[i]);
			//modeltable->Draw(m_commandBuffers[i]);
			float colorDwords[4] = { 0.5f,1.0f,0.5f,1.0f };
			pipelineMT->Draw(m_drawCommandBuffers[i]);
			pipelineMT->PushConstants(m_drawCommandBuffers[i], colorDwords);
			planetable->Draw(m_drawCommandBuffers[i], pipelineMT);
			for (auto m : meshesmodel)
				m->Draw(m_drawCommandBuffers[i]);

			for (auto m : meshesplane)
				m->Draw(m_drawCommandBuffers[i]);
			m_mainRenderPass->End(m_drawCommandBuffers[i], i);

			m_drawCommandBuffers[i]->End();
		}
	}

	void updateUniformBuffers()
	{
		
	}

	void Prepare()
	{
		init();
		PrepareUI();
		BuildCommandBuffers();
		prepared = true;
	}
	float offset = 0;
	virtual void update(float dt)
	{
		float m_aspectRatio = static_cast<float>(width) / static_cast<float>(height);
		glm::vec3 lightpos(8.8, 10.0, 8.8);
		glm::vec3 camerapos(0.0, 0.5, 1.5);
		glm::vec3 lookatpoint(0.0, 0.0, 0.0);
		
		glm::mat4 depthProjectionMatrix = glm::perspective(0.8f, 1.0f, 0.1f, 300.0f);
		glm::mat4 depthViewMatrix = glm::lookAt(lightpos, glm::vec3(0.0f), glm::vec3(0, 1, 0));
		glm::mat4x4 myvm = camera.GetViewMatrix();
		myvm = glm::transpose(myvm);

		/*m_constantBufferData.mp = glm::transpose(camera.GetPerspectiveMatrix());
		m_constantBufferData.mv = glm::transpose(camera.GetViewMatrix());
		m_constantBufferData.mlv = glm::transpose(depthViewMatrix);*/
		m_constantBufferData.mp = camera.GetPerspectiveMatrix();
		m_constantBufferData.mv = camera.GetViewMatrix();
		m_constantBufferData.mlv = depthViewMatrix;
		m_constantBufferData2.mp = m_constantBufferData.mp;
		m_constantBufferData2.mv = m_constantBufferData.mlv;

		const float translationSpeed = 0.005f;
		const float offsetBounds = 1.25f;

		offset += translationSpeed;
		if (offset > offsetBounds)
		{
			offset = -offsetBounds;
		}
		//memcpy(m_pCbvDataBegin, &m_constantBufferData, sizeof(m_constantBufferData));
		m_constantBuffer->MemCopy(&m_constantBufferData, sizeof(m_constantBufferData));
		m_constantBufferOffscreen->MemCopy(&m_constantBufferData2, sizeof(m_constantBufferData2));
		//memcpy(m_pCbvDataBeginOffscreen, &m_constantBufferData2, sizeof(m_constantBufferData2));

		//BuildCommandBuffers();
	}

	virtual void ViewChanged()
	{
		updateUniformBuffers();
	}

};

VULKAN_EXAMPLE_MAIN()