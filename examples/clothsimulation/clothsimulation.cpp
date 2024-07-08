
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

#include "vulkanexamplebase.h"
#include "scene/SimpleModel.h"
#include "scene/UniformBuffersManager.h"
#include "ClothCompute.h"

#define SHADOWMAP_DIM 512

using namespace engine;

class VulkanExample : public VulkanExampleBase
{
public:

	uint32_t readSet = 0;

	scene::ClothComputeObject clothcompute;
	
	scene::RenderObject clothobject;

	render::VertexLayout vertexLayout = render::VertexLayout({
		render::VERTEX_COMPONENT_POSITION,
		render::VERTEX_COMPONENT_NORMAL,
		render::VERTEX_COMPONENT_UV
		}, {});

	engine::scene::SimpleModel models;
	render::VulkanTexture* colorMap;
	render::VulkanTexture* clothMap;

	render::VulkanBuffer* sceneVertexUniformBuffer;
	scene::UniformBuffersManager uniform_manager;


	float depthBiasConstant = 2.25f;
	float depthBiasSlope = 1.75f;
	struct {
		glm::mat4 depthMVP;
	} uboOffscreenVS;
	glm::mat4 depthProjectionMatrix;
	glm::mat4 depthViewMatrix;
	render::VulkanTexture* shadowtex = nullptr;
	render::VulkanRenderPass* offscreenPass;
	render::VulkanBuffer* uniformBufferoffscreen = nullptr;
	scene::RenderObject shadowobjects;
	std::vector<VkCommandBuffer> drawShadowCmdBuffers;
	float projectionWidth = 4.0f;
	float projectionDepth = 5.0f;

	glm::vec4 light_pos = glm::vec4(0.0f, -5.0f, 5.0f, 1.0f);
	glm::vec4 depthProjectionStart = glm::vec4(0.0f, -5.0f, 0.0f, 1.0f);

	VulkanExample() : VulkanExampleBase(true)
	{
		zoom = -3.75f;
		rotationSpeed = 0.5f;
		rotation = glm::vec3(15.0f, 0.f, 0.0f);
		title = "Render Engine Empty Scene";
		settings.overlay = true;
		camera.type = scene::Camera::CameraType::firstperson;
		camera.movementSpeed = 2.5f;
		camera.setPerspective(60.0f, (float)width / (float)height, 0.1f, 1024.0f);
		camera.setRotation(glm::vec3(0.0f, 0.0f, 0.0f));
		camera.setTranslation(glm::vec3(0.0f, 5.0f, -5.0f));
	}

	~VulkanExample()
	{

	}

	void setupGeometry()
	{
		models.LoadGeometry(engine::tools::getAssetPath() + "models/venus.fbx", &vertexLayout, 0.15f, 1, glm::vec3(0.0f));
		models.LoadGeometry(engine::tools::getAssetPath() + "models/plane.obj", &vertexLayout, 0.5f, 1);
		for (auto geo : models.m_geometries)
		{
			geo->SetIndexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geo->m_indexCount * sizeof(uint32_t), geo->m_indices));
			geo->SetVertexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geo->m_verticesSize * sizeof(float), geo->m_vertices));
		}

		shadowobjects.SetVertexLayout(&vertexLayout);
		scene::Geometry* mygeo = new scene::Geometry;
		*mygeo = *models.m_geometries[0];
		shadowobjects.m_geometries.push_back(mygeo);
		mygeo = new scene::Geometry;
		*mygeo = *models.m_geometries[1];
		shadowobjects.m_geometries.push_back(mygeo);

		prepareStorageBuffers();
	}

	void prepareOffscreenRenderpass()
	{
		shadowtex = vulkanDevice->GetDepthRenderTarget(SHADOWMAP_DIM, SHADOWMAP_DIM, true);
		offscreenPass = vulkanDevice->GetRenderPass({ { shadowtex->m_format, shadowtex->m_descriptor.imageLayout } });
		render::VulkanFrameBuffer* fb = vulkanDevice->GetFrameBuffer(offscreenPass->GetRenderPass(), SHADOWMAP_DIM, SHADOWMAP_DIM, { shadowtex->m_descriptor.imageView });
		offscreenPass->AddFrameBuffer(fb);
	}

	void prepareStorageBuffers()
	{
		clothcompute.PrepareStorageBuffers(glm::vec2(2.0f), glm::ivec2(50, 50), vulkanDevice, queue);
		uint32_t indexBufferSize = static_cast<uint32_t>(clothcompute.m_indices.size()) * sizeof(uint32_t);
		uint32_t indexCount = static_cast<uint32_t>(clothcompute.m_indices.size());

		scene::Geometry* geo = new scene::Geometry;
		geo->m_indexCount = indexCount;
		geo->SetIndexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, indexBufferSize, clothcompute.m_indices.data()));
		geo->SetVertexBuffer(clothcompute.storageBuffers.outbuffer);

		clothobject._vertexLayout = &vertexLayout;
		clothobject.AddGeometry(geo);
	}

	void SetupTextures()
	{
		// Textures
		if (vulkanDevice->features.textureCompressionBC) {
			colorMap = vulkanDevice->GetTexture(engine::tools::getAssetPath() + "textures/darkmetal_bc3_unorm.ktx", VK_FORMAT_BC3_UNORM_BLOCK, queue);
		}
		else if (vulkanDevice->features.textureCompressionASTC_LDR) {
			colorMap = vulkanDevice->GetTexture(engine::tools::getAssetPath() + "textures/darkmetal_astc_8x8_unorm.ktx", VK_FORMAT_ASTC_8x8_UNORM_BLOCK, queue);
		}
		else if (vulkanDevice->features.textureCompressionETC2) {
			colorMap = vulkanDevice->GetTexture(engine::tools::getAssetPath() + "textures/darkmetal_etc2_unorm.ktx", VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK, queue);
		}
		else {
			engine::tools::exitFatal("Device does not support any compressed texture format!", VK_ERROR_FEATURE_NOT_PRESENT);
		}
		clothMap = vulkanDevice->GetTexture(engine::tools::getAssetPath() + "models/sponza/za_curtain_blue_diff.png", VK_FORMAT_R8G8B8A8_UNORM, queue);
		
	}

	void SetupUniforms()
	{
		//uniforms
		uniform_manager.SetDevice(vulkanDevice->logicalDevice);
		uniform_manager.SetEngineDevice(vulkanDevice);
		sceneVertexUniformBuffer = uniform_manager.GetGlobalUniformBuffer({ scene::UNIFORM_PROJECTION ,scene::UNIFORM_VIEW ,scene::UNIFORM_LIGHT0_POSITION, scene::UNIFORM_CAMERA_POSITION });

		clothcompute.PrepareUniformBuffer(vulkanDevice, projectionWidth, projectionDepth);

		uniformBufferoffscreen = vulkanDevice->GetUniformBuffer(sizeof(uboOffscreenVS), true, queue);
		uniformBufferoffscreen->map();

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

	void setupDescriptorPool()
	{
		std::vector<VkDescriptorPoolSize> poolSizes = {
			VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 6},
			VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4},
			VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 6}
		};
		vulkanDevice->CreateDescriptorSetsPool(poolSizes, 7);
	}

	void SetupDescriptors()
	{
		std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> modelbindings
		{
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
		};
		models.SetDescriptorSetLayout(vulkanDevice->GetDescriptorSetLayout(modelbindings));

		models.AddDescriptor(vulkanDevice->GetDescriptorSet({ &sceneVertexUniformBuffer->m_descriptor }, {&colorMap->m_descriptor},
			models._descriptorLayout->m_descriptorSetLayout, models._descriptorLayout->m_setLayoutBindings));

		clothobject.SetDescriptorSetLayout(vulkanDevice->GetDescriptorSetLayout(modelbindings));
		clothobject.AddDescriptor(vulkanDevice->GetDescriptorSet({ &sceneVertexUniformBuffer->m_descriptor }, { &clothMap->m_descriptor },
			clothobject._descriptorLayout->m_descriptorSetLayout, clothobject._descriptorLayout->m_setLayoutBindings));

		std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> offscreenbindings
		{
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT}
		};
		shadowobjects.SetDescriptorSetLayout(vulkanDevice->GetDescriptorSetLayout(offscreenbindings));
		shadowobjects.AddDescriptor(vulkanDevice->GetDescriptorSet({ &uniformBufferoffscreen->m_descriptor }, {},
			shadowobjects._descriptorLayout->m_descriptorSetLayout, shadowobjects._descriptorLayout->m_setLayoutBindings));
	}

	void setupPipelines()
	{
		std::vector<VkVertexInputAttributeDescription> vertexInputAttributes;
		vertexInputAttributes.push_back(VkVertexInputAttributeDescription{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 });
		shadowobjects.AddPipeline(vulkanDevice->GetPipeline(shadowobjects._descriptorLayout->m_descriptorSetLayout, shadowobjects._vertexLayout->m_vertexInputBindings, shadowobjects._vertexLayout->m_vertexInputAttributes,
			engine::tools::getAssetPath() + "shaders/shadowmapping/offscreen.vert.spv", "", offscreenPass->GetRenderPass(), pipelineCache//);
			, false, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, nullptr, 0, nullptr, true));

		models.AddPipeline(vulkanDevice->GetPipeline(models._descriptorLayout->m_descriptorSetLayout, vertexLayout.m_vertexInputBindings, vertexLayout.m_vertexInputAttributes,
			engine::tools::getAssetPath() + "shaders/basic/phong.vert.spv", engine::tools::getAssetPath() + "shaders/basic/phongtextured.frag.spv", mainRenderPass->GetRenderPass(), pipelineCache));

		std::vector<VkVertexInputBindingDescription> inputBindings = {
			VkVertexInputBindingDescription{0, sizeof(scene::ClothComputeObject::ClothVertex), VK_VERTEX_INPUT_RATE_VERTEX}
		};

		std::vector<VkVertexInputAttributeDescription> inputAttributes = {
			VkVertexInputAttributeDescription{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(scene::ClothComputeObject::ClothVertex, position)},
			VkVertexInputAttributeDescription{1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(scene::ClothComputeObject::ClothVertex, normal)},
			VkVertexInputAttributeDescription{2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(scene::ClothComputeObject::ClothVertex, uv)}
			
		};
		render::PipelineProperties props;
		props.cullMode = VK_CULL_MODE_NONE;
		props.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		props.primitiveRestart = VK_TRUE;
		clothobject.AddPipeline(vulkanDevice->GetPipeline(clothobject._descriptorLayout->m_descriptorSetLayout, inputBindings, inputAttributes,
			engine::tools::getAssetPath() + "shaders/basic/phong.vert.spv", engine::tools::getAssetPath() + "shaders/basic/phongtextured.frag.spv", mainRenderPass->GetRenderPass(), pipelineCache, props));
	}

	void prepareCompute()
	{
		std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> computebindings
		{
			{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},
			{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT}
		};
		clothcompute._descriptorLayout = vulkanDevice->GetDescriptorSetLayout(computebindings);
		clothcompute.m_descriptorSets.resize(2);
		clothcompute.m_descriptorSets[0] = vulkanDevice->GetDescriptorSet({ &clothcompute.storageBuffers.inbuffer->m_descriptor, &clothcompute.storageBuffers.outbuffer->m_descriptor, &clothcompute.m_uniformBuffer->m_descriptor }, { &shadowtex->m_descriptor },
			clothcompute._descriptorLayout->m_descriptorSetLayout, clothcompute._descriptorLayout->m_setLayoutBindings);
		clothcompute.m_descriptorSets[1] = vulkanDevice->GetDescriptorSet({ &clothcompute.storageBuffers.outbuffer->m_descriptor, &clothcompute.storageBuffers.inbuffer->m_descriptor, &clothcompute.m_uniformBuffer->m_descriptor }, { &shadowtex->m_descriptor },
			clothcompute._descriptorLayout->m_descriptorSetLayout, clothcompute._descriptorLayout->m_setLayoutBindings);

		std::string fileName = engine::tools::getAssetPath() + "shaders/clothsimulation/" + "cloth" + ".comp.spv";
		clothcompute._pipeline = vulkanDevice->GetComputePipeline(fileName, device, clothcompute._descriptorLayout->m_descriptorSetLayout, pipelineCache, 0);
	}

	void init()
	{	
		prepareOffscreenRenderpass();
		setupGeometry();
		SetupTextures();
		SetupUniforms();
		setupDescriptorPool();
		SetupDescriptors();
		setupPipelines();
		prepareCompute();
	}

	void addComputeToGraphicsBarriers(VkCommandBuffer commandBuffer)
	{
		VkBufferMemoryBarrier bufferBarrier{};
		bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		bufferBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		bufferBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
		bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		bufferBarrier.size = VK_WHOLE_SIZE;
		std::vector<VkBufferMemoryBarrier> bufferBarriers;
		bufferBarrier.buffer = clothcompute.storageBuffers.inbuffer->m_buffer;
		bufferBarriers.push_back(bufferBarrier);
		bufferBarrier.buffer = clothcompute.storageBuffers.outbuffer->m_buffer;
		bufferBarriers.push_back(bufferBarrier);
		vkCmdPipelineBarrier(
			commandBuffer,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
			VK_FLAGS_NONE,
			0, nullptr,
			static_cast<uint32_t>(bufferBarriers.size()), bufferBarriers.data(),
			0, nullptr);
	}

	void buildShadowCommandBuffers()
	{
		if (swapChain.queueNodeIndex != UINT32_MAX)
		{
			drawShadowCmdBuffers = vulkanDevice->createdrawCommandBuffers(swapChain.imageCount, swapChain.queueNodeIndex);//george carefull
		}
		VkCommandBufferBeginInfo cmdBufInfo{};
		cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		for (int32_t i = 0; i < drawShadowCmdBuffers.size(); ++i)
		{

			VK_CHECK_RESULT(vkBeginCommandBuffer(drawShadowCmdBuffers[i], &cmdBufInfo));

			offscreenPass->Begin(drawShadowCmdBuffers[i], 0);

			vkCmdSetDepthBias(
				drawShadowCmdBuffers[i],
				depthBiasConstant,
				0.0f,
				depthBiasSlope);

			shadowobjects.Draw(drawShadowCmdBuffers[i]);

			offscreenPass->End(drawShadowCmdBuffers[i]);

			VK_CHECK_RESULT(vkEndCommandBuffer(drawShadowCmdBuffers[i]));
		}
	}

	void buildComputeCommandBuffers()
	{
		if (swapChain.queueNodeIndex != UINT32_MAX)
		{
			clothcompute.commandBuffers = vulkanDevice->createdrawCommandBuffers(swapChain.imageCount, swapChain.queueNodeIndex);
		}

		VkCommandBufferBeginInfo cmdBufInfo{};
		cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		for (int32_t i = 0; i < clothcompute.commandBuffers.size(); ++i)
		{
			VK_CHECK_RESULT(vkBeginCommandBuffer(clothcompute.commandBuffers[i], &cmdBufInfo));

			vkCmdBindPipeline(clothcompute.commandBuffers[i], VK_PIPELINE_BIND_POINT_COMPUTE, clothcompute._pipeline->getPipeline());

			const uint32_t iterations = 64;
			for (uint32_t j = 0; j < iterations; j++) {
				readSet = 1 - readSet;

				clothcompute.m_descriptorSets[readSet]->Draw(clothcompute.commandBuffers[i], clothcompute._pipeline->getPipelineLayout(),0, VK_PIPELINE_BIND_POINT_COMPUTE);
				vkCmdDispatch(clothcompute.commandBuffers[i], clothcompute.m_gridSize.x / 10, clothcompute.m_gridSize.y / 10, 1);
			}

			addComputeToGraphicsBarriers(clothcompute.commandBuffers[i]);
			vkEndCommandBuffer(clothcompute.commandBuffers[i]);

		}
	}

	void buildCommandBuffers()
	{
		VkCommandBufferBeginInfo cmdBufInfo{};
		cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		for (int32_t i = 0; i < drawCmdBuffers.size(); ++i)
		{
			VK_CHECK_RESULT(vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufInfo));

			//addComputeToGraphicsBarriers(drawCmdBuffers[i]);

			mainRenderPass->Begin(drawCmdBuffers[i], i);

			models.Draw(drawCmdBuffers[i]);
		
			clothobject.Draw(drawCmdBuffers[i]);

			drawUI(drawCmdBuffers[i]);

			mainRenderPass->End(drawCmdBuffers[i]);

			VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
		}
	}

	void updateUniformBufferOffscreen()
	{

		float widthSize = projectionWidth * 0.5;
		depthProjectionMatrix = glm::ortho(-widthSize, widthSize, -widthSize, widthSize, 0.0f, projectionDepth);
		glm::vec3 lp = glm::vec3(depthProjectionStart);
		glm::mat4 rotM = glm::mat4(1.0f);
		rotM = glm::rotate(rotM, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		glm::mat4 transM = glm::translate(glm::mat4(1.0f), -lp);
		depthViewMatrix = rotM * transM;

		uboOffscreenVS.depthMVP = depthProjectionMatrix * depthViewMatrix /** depthModelMatrix*/;

		memcpy(uniformBufferoffscreen->m_mapped, &uboOffscreenVS, sizeof(uboOffscreenVS));

	}

	void updateUniformBuffers()
	{
		uniform_manager.UpdateGlobalParams(scene::UNIFORM_PROJECTION, &camera.matrices.perspective, 0, sizeof(camera.matrices.perspective));
		uniform_manager.UpdateGlobalParams(scene::UNIFORM_VIEW, &camera.matrices.view, 0, sizeof(camera.matrices.view));
		uniform_manager.UpdateGlobalParams(scene::UNIFORM_LIGHT0_POSITION, &light_pos, 0, sizeof(light_pos));
		glm::vec3 cucu = -camera.position;
		uniform_manager.UpdateGlobalParams(scene::UNIFORM_CAMERA_POSITION, &cucu, 0, sizeof(camera.position));

		uniform_manager.Update();
	}

	void draw()
	{
		VulkanExampleBase::prepareFrame();

		std::vector<VkCommandBuffer> cmdBuffers = { drawShadowCmdBuffers[currentBuffer], clothcompute.commandBuffers[currentBuffer],  drawCmdBuffers[currentBuffer] };

		submitInfo.commandBufferCount = cmdBuffers.size();
		submitInfo.pCommandBuffers = cmdBuffers.data();
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

		VulkanExampleBase::submitFrame();
	}

	void prepare()
	{
		VulkanExampleBase::prepare();
		init();
		
		prepareUI();

		buildShadowCommandBuffers();
		buildComputeCommandBuffers();
		buildCommandBuffers();
		
		prepared = true;
	}

	virtual void render()
	{
		if (!prepared)
			return;
		draw();
	}

	virtual void update(float dt)
	{
		updateComputeUBO();
	}

	virtual void viewChanged()
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
				clothcompute.ubo.pinnedCorners.x = clothcompute.pinnedCorners.x;
				updateComputeUBO();
			}
			if (overlay->checkBox("Pin2", &clothcompute.pinnedCorners.y))
			{
				clothcompute.ubo.pinnedCorners.y = clothcompute.pinnedCorners.y;
				updateComputeUBO();
			}
			if (overlay->checkBox("Pin3", &clothcompute.pinnedCorners.z))
			{
				clothcompute.ubo.pinnedCorners.z = clothcompute.pinnedCorners.z;
				updateComputeUBO();
			}
			if (overlay->checkBox("Pin4", &clothcompute.pinnedCorners.w))
			{
				clothcompute.ubo.pinnedCorners.w = clothcompute.pinnedCorners.w;
				updateComputeUBO();
			}
		}
	}

};

VULKAN_EXAMPLE_MAIN()