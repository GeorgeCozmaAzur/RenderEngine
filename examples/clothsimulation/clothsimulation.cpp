
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <vector>
#include <random>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include "VulkanApplication.h"
#include "D3D12Application.h"
#include "scene/SimpleModel.h"
#include "scene/UniformBuffersManager.h"
#include "render/vulkan/VulkanDescriptorPool.h"
#include "render/vulkan/VulkanCommandBuffer.h"
#include "ClothCompute.h"
#include "render/vulkan/VulkanMesh.h"

#define SHADOWMAP_DIM 512

using namespace engine;

class VulkanExample : public VulkanApplication
{
public:

	uint32_t readSet = 0;

	scene::ClothComputeObject clothcompute;
	
	scene::RenderObject clothobject;

	render::VertexLayout* vertexLayout = nullptr;
		/*render::VulkanVertexLayout({
		render::VERTEX_COMPONENT_POSITION,
		render::VERTEX_COMPONENT_NORMAL,
		render::VERTEX_COMPONENT_UV
		}, {});*/

	render::DescriptorPool* descriptorPool;
	render::DescriptorPool* descriptorPoolDSV;

	engine::scene::SimpleModel models;
	render::Texture* colorMap;
	render::Texture* clothMap;

	render::Buffer* sceneVertexUniformBuffer;
	scene::UniformBuffersManager uniform_manager;


	float depthBiasConstant = 2.25f;
	float depthBiasSlope = 1.75f;
	struct {
		glm::mat4 depthMVP;
	} uboOffscreenVS;
	glm::mat4 depthProjectionMatrix;
	glm::mat4 depthViewMatrix;
	render::Texture* shadowtex = nullptr;
	render::RenderPass* offscreenPass;
	render::Buffer* uniformBufferoffscreen = nullptr;
	scene::RenderObject shadowobjects;
	std::vector<render::CommandBuffer*> drawShadowCmdBuffers;
	float projectionWidth = 4.0f;
	float projectionDepth = 5.0f;

	glm::vec4 light_pos = glm::vec4(0.0f, -5.0f, 5.0f, 1.0f);
	glm::vec4 depthProjectionStart = glm::vec4(0.0f, -5.0f, 0.0f, 1.0f);

	VulkanExample() : VulkanApplication(true)
	{
		zoom = -3.75f;
		rotationSpeed = 0.5f;
		rotation = glm::vec3(15.0f, 0.f, 0.0f);
		title = "Render Engine Empty Scene";
		settings.overlay = true;
		camera.SetFlipY(false);
		camera.movementSpeed = 2.5f;
		camera.SetPerspective(60.0f, (float)width / (float)height, 0.1f, 1024.0f);
		camera.SetRotation(glm::vec3(0.0f, 0.0f, 0.0f));
		camera.SetPosition(glm::vec3(0.0f, 6.0f, -5.0f));
	}

	~VulkanExample()
	{

	}

	void setupDescriptorPool()
	{
		/*std::vector<VkDescriptorPoolSize> poolSizes = {
			VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 6},
			VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4},
			VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 6}
		};
		descriptorPool = m_device->CreateDescriptorSetsPool(poolSizes, 7);*/

		descriptorPool = m_device->GetDescriptorPool(
			{ 
			{render::DescriptorType::UNIFORM_BUFFER, 6},
			{render::DescriptorType::INPUT_STORAGE_BUFFER, 2},
			{render::DescriptorType::OUTPUT_STORAGE_BUFFER, 2},
			{render::DescriptorType::IMAGE_SAMPLER, 6} 
			}, 7);
		descriptorPoolDSV = m_device->GetDescriptorPool({ {render::DescriptorType::DSV, 1} }, 1);
	}

	void setupGeometry()
	{
		vertexLayout = m_device->GetVertexLayout({
		render::VERTEX_COMPONENT_POSITION,
		render::VERTEX_COMPONENT_NORMAL,
		render::VERTEX_COMPONENT_UV
			}, {});

		std::vector<render::MeshData*> pmd = models.LoadGeometry(engine::tools::getAssetPath() + "models/venus.fbx", vertexLayout, 0.15f, 1, glm::vec3(0.0f));
		std::vector<render::MeshData*> pmd1 = models.LoadGeometry(engine::tools::getAssetPath() + "models/plane.obj", vertexLayout, 0.5f, 1);
		pmd.insert(pmd.end(), pmd1.begin(), pmd1.end());
		for (auto geo : pmd)
		{
			//geo->SetIndexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geo->m_indexCount * sizeof(uint32_t), geo->m_indices));
			//geo->SetVertexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geo->m_verticesSize * sizeof(float), geo->m_vertices));
			render::Mesh* m = m_device->GetMesh(geo, vertexLayout, m_loadingCommandBuffer);
			models.AddGeometry(m);
			shadowobjects.AddGeometry(m);
			delete geo;
		}

		shadowobjects.SetVertexLayout(vertexLayout);
		/*scene::Geometry* mygeo = new scene::Geometry;
		*mygeo = *models.m_geometries[0];
		shadowobjects.m_geometries.push_back(mygeo);
		mygeo = new scene::Geometry;
		*mygeo = *models.m_geometries[1];
		shadowobjects.m_geometries.push_back(mygeo);*/

		prepareStorageBuffers();
	}

	void prepareOffscreenRenderpass()
	{
		shadowtex = m_device->GetDepthRenderTarget(SHADOWMAP_DIM, SHADOWMAP_DIM, render::GfxFormat::D32_FLOAT, descriptorPool, descriptorPoolDSV, m_loadingCommandBuffer, true, false);
		offscreenPass = m_device->GetRenderPass(SHADOWMAP_DIM, SHADOWMAP_DIM, {}, shadowtex);
		/*offscreenPass = m_device->GetRenderPass({ { shadowtex->m_format, shadowtex->m_descriptor.imageLayout } });
		render::VulkanFrameBuffer* fb = m_device->GetFrameBuffer(offscreenPass->GetRenderPass(), SHADOWMAP_DIM, SHADOWMAP_DIM, { shadowtex->m_descriptor.imageView });
		offscreenPass->AddFrameBuffer(fb);*/
	}

	void prepareStorageBuffers()
	{
		clothcompute.PrepareStorageBuffers(glm::vec2(2.0f), glm::ivec2(50, 50), m_device, descriptorPool, m_loadingCommandBuffer);
		uint32_t indexBufferSize = static_cast<uint32_t>(clothcompute.m_indices.size()) * sizeof(uint32_t);
		uint32_t indexCount = static_cast<uint32_t>(clothcompute.m_indices.size());

		//scene::Geometry* geo = new scene::Geometry;
		render::MeshData mdata;
		mdata.m_indexCount = indexCount;
		mdata.m_indices = new uint32_t[indexCount];
		memcpy(mdata.m_indices, clothcompute.m_indices.data(), indexCount * mdata.m_indexSize);
		render::Mesh* geo = m_device->GetMesh(&mdata, vertexLayout, m_loadingCommandBuffer);
		//geo->m_indexCount = indexCount;
		//geo->_indexBuffer = vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, indexBufferSize, clothcompute.m_indices.data());
		//geo->_vertexBuffer = clothcompute.storageBuffers.outbuffer;
		geo->SetVertexBuffer(clothcompute.storageBuffers.outbuffer);

		clothobject._vertexLayout = vertexLayout;
		clothobject.AddGeometry(geo);
	}

	void SetupTextures()
	{
		/*render::Texture2DData data;
		if (vulkanDevice->m_enabledFeatures.textureCompressionBC) {
			data.LoadFromFile(engine::tools::getAssetPath() + "textures/darkmetal_bc3_unorm.ktx", render::GfxFormat::BC3_UNORM_BLOCK);
			colorMap = vulkanDevice->GetTexture(&data, queue);
		}
		else if (vulkanDevice->m_enabledFeatures.textureCompressionASTC_LDR) {
			data.LoadFromFile(engine::tools::getAssetPath() + "textures/darkmetal_astc_8x8_unorm.ktx", render::GfxFormat::ASTC_8x8_UNORM_BLOCK);
			colorMap = vulkanDevice->GetTexture(&data, queue);
		}
		else if (vulkanDevice->m_enabledFeatures.textureCompressionETC2) {
			data.LoadFromFile(engine::tools::getAssetPath() + "textures/darkmetal_etc2_unorm.ktx", render::GfxFormat::ETC2_R8G8B8_UNORM_BLOCK);
			colorMap = vulkanDevice->GetTexture(&data, queue);
		}
		else {
			engine::tools::exitFatal("Device does not support any compressed texture format!", VK_ERROR_FEATURE_NOT_PRESENT);
		}*/
		render::Texture2DData tdata;
		tdata.LoadFromFile("./../data/textures/compass.jpg", render::GfxFormat::R8G8B8A8_UNORM);
		colorMap = m_device->GetTexture(&tdata, descriptorPool, m_loadingCommandBuffer);
		tdata.Destroy();

		/*tdata.LoadFromFile(engine::tools::getAssetPath() + "models/sponza/za_curtain_blue_diff.png", render::GfxFormat::R8G8B8A8_UNORM);
		clothMap = vulkanDevice->GetTexture(&tdata, queue,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			true);*/
		tdata.LoadFromFile(engine::tools::getAssetPath() + "models/sponza/za_curtain_blue_diff.png", render::GfxFormat::R8G8B8A8_UNORM);
		clothMap = m_device->GetTexture(&tdata, descriptorPool, m_loadingCommandBuffer, true);
		//data.Destroy();
	}

	void SetupUniforms()
	{
		//uniforms
		uniform_manager.SetDescriptorPool(descriptorPool);
		uniform_manager.SetEngineDevice(m_device);
		sceneVertexUniformBuffer = uniform_manager.GetGlobalUniformBuffer({ scene::UNIFORM_PROJECTION ,scene::UNIFORM_VIEW ,scene::UNIFORM_LIGHT0_POSITION, scene::UNIFORM_CAMERA_POSITION });

		clothcompute.PrepareUniformBuffer(m_device, descriptorPool, projectionWidth, projectionDepth);

		uniformBufferoffscreen = m_device->GetUniformBuffer(sizeof(uboOffscreenVS), nullptr, descriptorPool);
		//uniformBufferoffscreen->Map();

		updateUniformBuffers();
		updateComputeUBO();
		updateUniformBufferOffscreen();
	}
	void updateComputeUBO()
	{
		if (!paused) {
			clothcompute.ubo.deltaT = 0.00001f;
			// todo: base on frametime
			//compute.ubo.deltaT = frameTimer * 0.0075f;

			if (true) {
				std::default_random_engine rndEngine( 0 );
				std::uniform_real_distribution<float> rd(1.0f, 60.0f);
				clothcompute.ubo.externalForce.x = cos(glm::radians(-timer * 360.0f)) * (rd(rndEngine) - rd(rndEngine));
				clothcompute.ubo.externalForce.z = sin(glm::radians(timer * 360.0f)) * (rd(rndEngine) - rd(rndEngine));
			}
			else {
				clothcompute.ubo.externalForce.x = 0.0f;
				clothcompute.ubo.externalForce.z = 0.0f;
			}
		}
		else {
			clothcompute.ubo.deltaT = 0.0f;
		}
		clothcompute.UpdateUniformBuffer();
	}


	void SetupDescriptors()
	{
		/*std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> modelbindings
		{
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
		};*/
		std::vector<render::LayoutBinding> modelbindings = {
						{render::DescriptorType::UNIFORM_BUFFER, render::ShaderStage::VERTEX},
						{render::DescriptorType::IMAGE_SAMPLER, render::ShaderStage::FRAGMENT}
			};
		models.SetDescriptorSetLayout(m_device->GetDescriptorSetLayout(modelbindings));

		models.AddDescriptor(m_device->GetDescriptorSet(models._descriptorLayout, descriptorPool, { sceneVertexUniformBuffer }, { colorMap }));
		/*models.AddDescriptor(m_device->GetDescriptorSet(descriptorPool, { &sceneVertexUniformBuffer->m_descriptor }, {&colorMap->m_descriptor},
			models._descriptorLayout->m_descriptorSetLayout, models._descriptorLayout->m_setLayoutBindings));*/

		clothobject.SetDescriptorSetLayout(m_device->GetDescriptorSetLayout(modelbindings));
		clothobject.AddDescriptor(m_device->GetDescriptorSet(clothobject._descriptorLayout, descriptorPool, { sceneVertexUniformBuffer }, { clothMap }));
		/*clothobject.AddDescriptor(m_device->GetDescriptorSet(descriptorPool, { &sceneVertexUniformBuffer->m_descriptor }, { &clothMap->m_descriptor },
			clothobject._descriptorLayout->m_descriptorSetLayout, clothobject._descriptorLayout->m_setLayoutBindings));*/

		/*std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> offscreenbindings
		{
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT}
		};*/
		std::vector<render::LayoutBinding> offscreenbindings = {
						{render::DescriptorType::UNIFORM_BUFFER, render::ShaderStage::VERTEX}
		};
		shadowobjects.SetDescriptorSetLayout(m_device->GetDescriptorSetLayout(offscreenbindings));
		shadowobjects.AddDescriptor(m_device->GetDescriptorSet(shadowobjects._descriptorLayout, descriptorPool, { uniformBufferoffscreen }, {  }));
		/*shadowobjects.AddDescriptor(m_device->GetDescriptorSet(descriptorPool, { &uniformBufferoffscreen->m_descriptor }, {},
			shadowobjects._descriptorLayout->m_descriptorSetLayout, shadowobjects._descriptorLayout->m_setLayoutBindings));*/
	}

	void setupPipelines()
	{
		std::vector<VkVertexInputAttributeDescription> vertexInputAttributes;
		vertexInputAttributes.push_back(VkVertexInputAttributeDescription{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 });
		render::PipelineProperties props;
		props.depthBias = true;;

		shadowobjects.AddPipeline(m_device->GetPipeline(
			GetShadersPath() + "shadowmapping/offscreen" + GetVertexShadersExt(), "", "", "",
			shadowobjects._vertexLayout, shadowobjects._descriptorLayout, props, offscreenPass));

		props.depthBias = false;
		models.AddPipeline(m_device->GetPipeline(
			GetShadersPath() + "basic/phong" + GetVertexShadersExt(), "VSMain", GetShadersPath() + "basic/phongtextured" + GetFragShadersExt(), "PSMainTextured",
			models._vertexLayout, models._descriptorLayout, props, m_mainRenderPass));

		/*std::vector<VkVertexInputBindingDescription> inputBindings = {
			VkVertexInputBindingDescription{0, sizeof(scene::ClothComputeObject::ClothVertex), VK_VERTEX_INPUT_RATE_VERTEX}
		};

		std::vector<VkVertexInputAttributeDescription> inputAttributes = {
			VkVertexInputAttributeDescription{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(scene::ClothComputeObject::ClothVertex, position)},
			VkVertexInputAttributeDescription{1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(scene::ClothComputeObject::ClothVertex, normal)},
			VkVertexInputAttributeDescription{2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(scene::ClothComputeObject::ClothVertex, uv)}
			
		};*/
		clothobject._vertexLayout = m_device->GetVertexLayout({
		render::VERTEX_COMPONENT_POSITION4D,
		render::VERTEX_COMPONENT_DUMMY_VEC4,
		render::VERTEX_COMPONENT_TANGENT4,
		render::VERTEX_COMPONENT_COLOR4
			}, {});
		//render::PipelineProperties props;
		props.cullMode = render::CullMode::NONE;
		props.topology = render::PrimitiveTopolgy::TRIANGLE_STRIP;
		props.primitiveRestart = VK_TRUE;
		//clothobject.AddPipeline(vulkanDevice->GetPipeline( ((render::VulkanDescriptorSetLayout*)clothobject._descriptorLayout)->m_descriptorSetLayout, inputBindings, inputAttributes,
		//	engine::tools::getAssetPath() + "shaders/clothsimulation/clothphong.vert.spv", engine::tools::getAssetPath() + "shaders/basic/phongtextured.frag.spv", mainRenderPass->GetRenderPass(), pipelineCache, props));
	
		clothobject.AddPipeline(m_device->GetPipeline(
			GetShadersPath() + "clothsimulation/clothphong" + GetVertexShadersExt(), "VSMain", GetShadersPath() + "basic/phongtextured" + GetFragShadersExt(), "PSMainTextured",
			clothobject._vertexLayout, clothobject._descriptorLayout, props, m_mainRenderPass));
	}

	void prepareCompute()
	{
		/*std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> computebindings
		{
			{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},
			{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT}
		};
		clothcompute._vulkanDescriptorLayout = m_device->GetDescriptorSetLayout(computebindings);*/
		clothcompute._vulkanDescriptorLayout = m_device->GetDescriptorSetLayout({
						{render::DescriptorType::INPUT_STORAGE_BUFFER, render::ShaderStage::COMPUTE},
						{render::DescriptorType::OUTPUT_STORAGE_BUFFER, render::ShaderStage::COMPUTE},
						{render::DescriptorType::UNIFORM_BUFFER, render::ShaderStage::COMPUTE},
						{render::DescriptorType::IMAGE_SAMPLER, render::ShaderStage::COMPUTE}
			});

		clothcompute.m_vulkanDescriptorSets.resize(2);
		/*render::VulkanDescriptorPool* vpool = (render::VulkanDescriptorPool*)descriptorPool;
		clothcompute.m_vulkanDescriptorSets[0] = vulkanDevice->GetDescriptorSet(vpool->m_vkPool, { &clothcompute.storageBuffers.inbuffer->m_descriptor, &clothcompute.storageBuffers.outbuffer->m_descriptor, &clothcompute.m_uniformBuffer->m_descriptor }, { &shadowtex->m_descriptor },
			clothcompute._vulkanDescriptorLayout->m_descriptorSetLayout, clothcompute._vulkanDescriptorLayout->m_setLayoutBindings);
		clothcompute.m_vulkanDescriptorSets[1] = vulkanDevice->GetDescriptorSet(vpool->m_vkPool, { &clothcompute.storageBuffers.outbuffer->m_descriptor, &clothcompute.storageBuffers.inbuffer->m_descriptor, &clothcompute.m_uniformBuffer->m_descriptor }, { &shadowtex->m_descriptor },
			clothcompute._vulkanDescriptorLayout->m_descriptorSetLayout, clothcompute._vulkanDescriptorLayout->m_setLayoutBindings);*/
		clothcompute.m_vulkanDescriptorSets[0] = m_device->GetDescriptorSet(clothcompute._vulkanDescriptorLayout, descriptorPool,
			{ clothcompute.storageBuffers.inbuffer, clothcompute.storageBuffers.outbuffer, clothcompute.m_uniformBuffer }, { shadowtex });
		clothcompute.m_vulkanDescriptorSets[1] = m_device->GetDescriptorSet(clothcompute._vulkanDescriptorLayout, descriptorPool,
			{ clothcompute.storageBuffers.outbuffer, clothcompute.storageBuffers.inbuffer, clothcompute.m_uniformBuffer }, { shadowtex });

		std::string fileName = GetShadersPath() + "clothsimulation/cloth" + GetComputeShadersExt();
		//clothcompute._vulkanPipeline = vulkanDevice->GetComputePipeline(fileName, device, clothcompute._vulkanDescriptorLayout->m_descriptorSetLayout, pipelineCache, 0);
		clothcompute._vulkanPipeline = m_device->GetComputePipeline(fileName, "", clothcompute._vulkanDescriptorLayout, 0);
	}

	void CreateAllCommandBuffers()
	{
		/*if (vulkanDevice->queueFamilyIndices.graphicsFamily == UINT32_MAX)
			return;*/
		// Create one command buffer for each swap chain image and reuse for rendering

		/*drawShadowCmdBuffers = vulkanDevice->CreatedrawCommandBuffers(swapChain.swapChainImageViews.size(), vulkanDevice->queueFamilyIndices.graphicsFamily);
		clothcompute.commandBuffers = vulkanDevice->CreatedrawCommandBuffers(swapChain.swapChainImageViews.size(), vulkanDevice->queueFamilyIndices.graphicsFamily);
		drawCommandBuffers = vulkanDevice->CreatedrawCommandBuffers(swapChain.swapChainImageViews.size(), vulkanDevice->queueFamilyIndices.graphicsFamily);

		allDrawCommandBuffers.push_back(drawShadowCmdBuffers);
		allDrawCommandBuffers.push_back(clothcompute.commandBuffers);
		allDrawCommandBuffers.push_back(drawCommandBuffers);*/
		drawShadowCmdBuffers = CreateCommandBuffers();
		clothcompute.commandBuffers = CreateCommandBuffers();
		m_drawCommandBuffers = CreateCommandBuffers();

	}

	void init()
	{	
		if (m_loadingCommandBuffer)
			m_loadingCommandBuffer->Begin();

		setupDescriptorPool();
		prepareOffscreenRenderpass();
		setupGeometry();
		SetupTextures();
		SetupUniforms();
		SetupDescriptors();
		prepareCompute();
		setupPipelines();
		PrepareUI();

		if (m_loadingCommandBuffer)
		{
			m_loadingCommandBuffer->End();
			SubmitOnQueue(m_loadingCommandBuffer);
		}

		WaitForDevice();

		m_device->FreeLoadStaggingBuffers();
	}

	/*void addComputeToGraphicsBarriers(VkCommandBuffer commandBuffer)
	{
		VkBufferMemoryBarrier bufferBarrier{};
		bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		bufferBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		bufferBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
		bufferBarrier.size = VK_WHOLE_SIZE;
		std::vector<VkBufferMemoryBarrier> bufferBarriers;
		bufferBarrier.buffer = ((render::VulkanBuffer*)clothcompute.storageBuffers.inbuffer)->GetVkBuffer();
		bufferBarriers.push_back(bufferBarrier);
		bufferBarrier.buffer = ((render::VulkanBuffer*)clothcompute.storageBuffers.outbuffer)->GetVkBuffer();
		bufferBarriers.push_back(bufferBarrier);
		vkCmdPipelineBarrier(
			commandBuffer,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
			VK_FLAGS_NONE,
			0, nullptr,
			static_cast<uint32_t>(bufferBarriers.size()), bufferBarriers.data(),
			0, nullptr);
	}*/

	void buildShadowCommandBuffers()
	{
		/*VkCommandBufferBeginInfo cmdBufInfo{};
		cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;*/
		for (int32_t i = 0; i < drawShadowCmdBuffers.size(); ++i)
		{

			//VK_CHECK_RESULT(vkBeginCommandBuffer(drawShadowCmdBuffers[i], &cmdBufInfo));
			drawShadowCmdBuffers[i]->Begin();

			offscreenPass->Begin(drawShadowCmdBuffers[i], 0);

			shadowobjects.Draw(drawShadowCmdBuffers[i]);

			offscreenPass->End(drawShadowCmdBuffers[i]);

			//VK_CHECK_RESULT(vkEndCommandBuffer(drawShadowCmdBuffers[i]));
			drawShadowCmdBuffers[i]->End();
		}
	}

	void buildComputeCommandBuffers()
	{	
		for (int32_t i = 0; i < clothcompute.commandBuffers.size(); ++i)
		{
			clothcompute.commandBuffers[i]->Begin();

			clothcompute._vulkanPipeline->Draw(clothcompute.commandBuffers[i]);

			const uint32_t iterations = 64;
			for (uint32_t j = 0; j < iterations; j++) {
				readSet = 1 - readSet;

				clothcompute.m_vulkanDescriptorSets[readSet]->Draw(clothcompute.commandBuffers[i], clothcompute._vulkanPipeline);
				DispatchCompute(clothcompute.commandBuffers[i], clothcompute.m_gridSize.x / 10, clothcompute.m_gridSize.y / 10, 1);
			}

			PipelineBarrier(clothcompute.commandBuffers[i], { clothcompute.storageBuffers.inbuffer , clothcompute.storageBuffers.outbuffer }, {});
			clothcompute.commandBuffers[i]->End();
		}
	}

	void BuildCommandBuffers()
	{
		/*VkCommandBufferBeginInfo cmdBufInfo{};
		cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;*/

		for (int32_t i = 0; i < m_drawCommandBuffers.size(); ++i)
		{
			//VK_CHECK_RESULT(vkBeginCommandBuffer(drawCommandBuffers[i], &cmdBufInfo));
			m_drawCommandBuffers[i]->Begin();

			//addComputeToGraphicsBarriers(drawCmdBuffers[i]);

			m_mainRenderPass->Begin(m_drawCommandBuffers[i], i);

			descriptorPool->Draw(m_drawCommandBuffers[i]);

			models.Draw(m_drawCommandBuffers[i]);
		
			clothobject.Draw(m_drawCommandBuffers[i]);

			DrawUI(m_drawCommandBuffers[i]);

			m_mainRenderPass->End(m_drawCommandBuffers[i], i);

			//VK_CHECK_RESULT(vkEndCommandBuffer(drawCommandBuffers[i]));
			m_drawCommandBuffers[i]->End();
		}
	}

	void updateUniformBufferOffscreen()
	{

		float widthSize = projectionWidth * 0.5f;
		depthProjectionMatrix = glm::ortho(-widthSize, widthSize, -widthSize, widthSize, 0.0f, projectionDepth);
		glm::vec3 lp = glm::vec3(depthProjectionStart);
		glm::mat4 rotM = glm::mat4(1.0f);
		rotM = glm::rotate(rotM, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		glm::mat4 transM = glm::translate(glm::mat4(1.0f), -lp);
		depthViewMatrix = rotM * transM;

		uboOffscreenVS.depthMVP = depthProjectionMatrix * depthViewMatrix /** depthModelMatrix*/;

		uniformBufferoffscreen->MemCopy(&uboOffscreenVS, sizeof(uboOffscreenVS));

	}

	void updateUniformBuffers()
	{
		glm::mat4 perspectiveMatrix = camera.GetPerspectiveMatrix();
		glm::mat4 viewMatrix = camera.GetViewMatrix();

		uniform_manager.UpdateGlobalParams(scene::UNIFORM_PROJECTION, &perspectiveMatrix, 0, sizeof(perspectiveMatrix));
		uniform_manager.UpdateGlobalParams(scene::UNIFORM_VIEW, &viewMatrix, 0, sizeof(viewMatrix));
		uniform_manager.UpdateGlobalParams(scene::UNIFORM_LIGHT0_POSITION, &light_pos, 0, sizeof(light_pos));
		glm::vec3 cucu = -camera.GetPosition();
		uniform_manager.UpdateGlobalParams(scene::UNIFORM_CAMERA_POSITION, &cucu, 0, sizeof(camera.GetPosition()));

		uniform_manager.Update(nullptr);
	}

	void Prepare()
	{	
		init();

		buildShadowCommandBuffers();
		buildComputeCommandBuffers();
		BuildCommandBuffers();
		
		prepared = true;
	}

	virtual void update(float dt)
	{
		updateComputeUBO();
	}

	virtual void ViewChanged()
	{
		updateUniformBuffers();
	}

	virtual void OnUpdateUIOverlay(engine::scene::UIOverlay *overlay)
	{
		if (overlay->header("Settings")) {
			if (ImGui::SliderFloat("Vertex mass", &clothcompute.ubo.particleMass, 0.05f, 1.0f))
			{
			}
			if (ImGui::SliderFloat("Stiffness", &clothcompute.ubo.springStiffness, 1000.0f, 10000.0f))
			{
			}
			if (ImGui::SliderFloat("Damping", &clothcompute.ubo.damping, 0.1f, 1.0f))
			{
			}
			if (overlay->checkBox("Pin1", &clothcompute.pinnedCorners.x))
			{
				clothcompute.ubo.pinnedCorners.x = (float)(clothcompute.pinnedCorners.x);
				updateComputeUBO();
			}
			if (overlay->checkBox("Pin2", &clothcompute.pinnedCorners.y))
			{
				clothcompute.ubo.pinnedCorners.y = (float)(clothcompute.pinnedCorners.y);
				updateComputeUBO();
			}
			if (overlay->checkBox("Pin3", &clothcompute.pinnedCorners.z))
			{
				clothcompute.ubo.pinnedCorners.z = (float)(clothcompute.pinnedCorners.z);
				updateComputeUBO();
			}
			if (overlay->checkBox("Pin4", &clothcompute.pinnedCorners.w))
			{
				clothcompute.ubo.pinnedCorners.w = (float)(clothcompute.pinnedCorners.w);
				updateComputeUBO();
			}
		}
	}

};

VULKAN_EXAMPLE_MAIN()