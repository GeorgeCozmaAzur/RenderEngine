
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <vector>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include <vulkan/vulkan.h>
#include "VulkanApplication.h"
#include "scene/SimpleModel.h"

#define FB_DIM 512

using namespace engine;

class VulkanExample : public VulkanApplication
{
public:

	// Vertex layout for the models
	render::VertexLayout vertexLayout = render::VertexLayout({
		render::VERTEX_COMPONENT_POSITION,
		render::VERTEX_COMPONENT_UV,
		render::VERTEX_COMPONENT_COLOR,
		render::VERTEX_COMPONENT_NORMAL
		}, {});

	struct {
		engine::scene::SimpleModel* example = new scene::SimpleModel;
		engine::scene::SimpleModel* exampleoffscreen = new scene::SimpleModel;
		engine::scene::SimpleModel* exampleoffscreenback = new scene::SimpleModel;
		engine::scene::SimpleModel* quad = new scene::SimpleModel;
		engine::scene::SimpleModel* plane = new scene::SimpleModel;
	} models;

	struct UBO {
		glm::mat4 projection;
		glm::mat4 view;
		glm::mat4 model;
		glm::vec4 lightPos = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
		glm::vec4 camPos = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	} uboShared;

	struct {
		render::VulkanBuffer* vsOffScreen;
		render::VulkanBuffer* vsOffScreenBack;
		render::VulkanBuffer* vsMirror;
		render::VulkanBuffer* vsModel;
		render::VulkanBuffer* vsDebugQuad;
	} uniformBuffers;

	render::VulkanTexture* colorMap;
	render::VulkanTexture* paraboloidTexFront;
	render::VulkanTexture* frontDepthTex;
	render::VulkanTexture* paraboloidTexBack;
	render::VulkanTexture* backDepthTex;

	struct {
		render::VulkanDescriptorSetLayout* model;
		render::VulkanDescriptorSetLayout* mirror;
	} layouts;

	struct {
		render::VulkanPipeline* offscreen;
		render::VulkanPipeline* offscreenBack;
		render::VulkanPipeline* mirror;
		render::VulkanPipeline* model;
		render::VulkanPipeline* debugQuad;
	} pipelines;

	struct {
		render::VulkanDescriptorSet* offscreen;
		render::VulkanDescriptorSet* offscreenBack;
		render::VulkanDescriptorSet* mirror;
		render::VulkanDescriptorSet* model;
		render::VulkanDescriptorSet* debugQuad;
	} descriptorSets;

	glm::vec3 meshPos = glm::vec3(0.0f, -0.5f, 0.0f);
	glm::vec3 meshRot = glm::vec3(0.0f);

	render::VulkanRenderPass* offscreenRenderPass;
	render::VulkanRenderPass* offscreenBackRenderPass;

	render::VulkanTexture* envMap;

	VulkanExample() : VulkanApplication(true)
	{
		zoom = -2.0f;
		rotationSpeed = 0.5f;
		rotation = glm::vec3(0.0f, 0.f, 0.0f);
		title = "Reflections example";
		settings.overlay = true;
		camera.movementSpeed = 5.0f;
		zoomSpeed = 0.1;
		camera.SetPerspective(60.0f, (float)width / (float)height, 0.1f, 16.0f);
		camera.SetRotation(glm::vec3(0.0f, 0.0f, 0.0f));
		camera.SetPosition(glm::vec3(0.0f, 0.0f, -5.0f));

		// The scene shader uses a clipping plane, so this feature has to be enabled
		enabledFeatures.shaderClipDistance = VK_TRUE;
	}

	~VulkanExample()
	{
		// Clean up used Vulkan resources 
		// Note : Inherited destructor cleans up resources stored in base class
		delete models.example;
		delete models.exampleoffscreen;
		delete models.exampleoffscreenback;
		delete models.quad;
		delete models.plane;
	}

	void init()
	{
		models.plane->LoadGeometry(engine::tools::getAssetPath() + "models/audi/Audi_r8.obj", &vertexLayout, 0.0005f, 1);
		models.example->LoadGeometry(engine::tools::getAssetPath() + "models/chinesedragon.dae", &vertexLayout, 0.1f, 1, glm::vec3(0.0, -0.25, -1.5));
		models.example->LoadGeometry(engine::tools::getAssetPath() + "models/oak_trunk.dae", &vertexLayout, 2.0f, 1, glm::vec3(-2.0, -0.5, 0.0));
		models.example->LoadGeometry(engine::tools::getAssetPath() + "models/cube.obj", &vertexLayout, 2.0f, 1, glm::vec3(0.0, -0.5, 1.5));
		models.example->LoadGeometry(engine::tools::getAssetPath() + "models/torusknot.obj", &vertexLayout, 0.02f, 1, glm::vec3(2.0,-0.5,0.0));

		// Textures
		if (vulkanDevice->m_enabledFeatures.textureCompressionBC) {
			colorMap = vulkanDevice->GetTexture(engine::tools::getAssetPath() + "textures/darkmetal_bc3_unorm.ktx", VK_FORMAT_BC3_UNORM_BLOCK, queue);
		}
		else if (vulkanDevice->m_enabledFeatures.textureCompressionASTC_LDR) {
			colorMap = vulkanDevice->GetTexture(engine::tools::getAssetPath() + "textures/darkmetal_astc_8x8_unorm.ktx", VK_FORMAT_ASTC_8x8_UNORM_BLOCK, queue);
		}
		else if (vulkanDevice->m_enabledFeatures.textureCompressionETC2) {
			colorMap = vulkanDevice->GetTexture(engine::tools::getAssetPath() + "textures/darkmetal_etc2_unorm.ktx", VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK, queue);
		}
		else {
			engine::tools::exitFatal("Device does not support any compressed texture format!", VK_ERROR_FEATURE_NOT_PRESENT);
		}

		envMap = vulkanDevice->GetTextureCubeMap(engine::tools::getAssetPath() + "textures/hdr/pisa_cube.ktx", VK_FORMAT_R16G16B16A16_SFLOAT, queue);

		paraboloidTexFront = vulkanDevice->GetColorRenderTarget(FB_DIM, FB_DIM, FB_COLOR_FORMAT);
		frontDepthTex = vulkanDevice->GetDepthRenderTarget(FB_DIM, FB_DIM, false);
		paraboloidTexBack = vulkanDevice->GetColorRenderTarget(FB_DIM, FB_DIM, FB_COLOR_FORMAT);
		backDepthTex = vulkanDevice->GetDepthRenderTarget(FB_DIM, FB_DIM, false);

		//generate index and vertex buffers from the data that we just loaded
		for (auto geo : models.plane->m_geometries)
		{
			geo->SetIndexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geo->m_indexCount * sizeof(uint32_t), geo->m_indices));
			geo->SetVertexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geo->m_verticesSize * sizeof(float), geo->m_vertices));
		}
		for (auto geo : models.example->m_geometries)
		{
			geo->SetIndexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geo->m_indexCount * sizeof(uint32_t), geo->m_indices));
			geo->SetVertexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geo->m_verticesSize * sizeof(float), geo->m_vertices));
		}

		//transfers the geometries from one object to another
		*models.exampleoffscreen = *models.example;
		*models.exampleoffscreenback = *models.example;

		//setup layouts
		std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> modelbindings
		{
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT}
		};

		std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> mirrorbindings
		{
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
		};

		layouts.model = vulkanDevice->GetDescriptorSetLayout(modelbindings);
		layouts.mirror = vulkanDevice->GetDescriptorSetLayout(mirrorbindings);

		models.example->SetDescriptorSetLayout(layouts.model);
		models.plane->SetDescriptorSetLayout(layouts.mirror);
	}

	void setupDescriptorPool()
	{
		// setup descriptor pool
		std::vector<VkDescriptorPoolSize> poolSizes = {
			VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 8},
			VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10}
		};
		vulkanDevice->CreateDescriptorSetsPool(poolSizes, 5);
	}

	void prepareUniformBuffers()
	{
		uniformBuffers.vsModel = vulkanDevice->GetBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, sizeof(uboShared));

		uniformBuffers.vsMirror = vulkanDevice->GetBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, sizeof(uboShared));

		uniformBuffers.vsOffScreen = vulkanDevice->GetBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, sizeof(uboShared));

		uniformBuffers.vsOffScreenBack = vulkanDevice->GetBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, sizeof(uboShared));

		uniformBuffers.vsDebugQuad = vulkanDevice->GetBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, sizeof(uboShared));


		// Map persistent
		VK_CHECK_RESULT(uniformBuffers.vsModel->Map());
		VK_CHECK_RESULT(uniformBuffers.vsMirror->Map());
		VK_CHECK_RESULT(uniformBuffers.vsOffScreen->Map());
		VK_CHECK_RESULT(uniformBuffers.vsOffScreenBack->Map());
		VK_CHECK_RESULT(uniformBuffers.vsDebugQuad->Map());
		updateModelsMatrix();
		updateUniformBuffers();
		updateUniformBufferOffscreen();
	}

	void updateModelsMatrix()
	{
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::rotate(modelMatrix, glm::radians(-30.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		uboShared.model = modelMatrix;
	}

	void updateUniformBufferOffscreen()
	{
		uboShared.projection = glm::perspective(glm::radians(60.0f), (float)width / (float)height, 0.1f, 256.0f);
		glm::mat4 viewMatrix = glm::mat4(1.0f);
		

		//uboShared.model = glm::rotate(viewMatrix, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		uboShared.view = viewMatrix;
		
		uniformBuffers.vsOffScreen->MemCopy(&uboShared, sizeof(uboShared));

		//uboShared.model = glm::rotate(viewMatrix, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		uboShared.view = glm::rotate(viewMatrix, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		
		uniformBuffers.vsOffScreenBack->MemCopy(&uboShared, sizeof(uboShared));
	}

	void updateUniformBuffers()
	{
		// Mesh
		uboShared.projection = camera.GetPerspectiveMatrix();//glm::perspective(glm::radians(170.0f), (float)width / (float)height, 256.0f, 0.1f);
		glm::vec4 campos = glm::vec4(0.0f, 0.0f, zoom, 1.0f);
		glm::mat4 viewMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(campos));

		glm::vec4 testvec(0.0,0.0,3.0,1.0);
		testvec = uboShared.projection * testvec;

		uboShared.view = camera.GetViewMatrix();
		//uboShared.model = glm::mat4(1.0f);
		uboShared.camPos = -glm::vec4(camera.GetPosition(), -1.0);

		uniformBuffers.vsModel->MemCopy(&uboShared, sizeof(uboShared));

		uboShared.model = glm::mat4(1.0f);
		uniformBuffers.vsMirror->MemCopy(&uboShared, sizeof(uboShared));

		// Debug quad
		/*uboShared.projection = glm::ortho(4.0f, 0.0f, 0.0f, 4.0f * (float)height / (float)width, -1.0f, 1.0f);
		uboShared.model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));

		uniformBuffers.vsDebugQuad->MemCopy(&uboShared, sizeof(uboShared));*/
	}

	void setupDescriptors()
	{
		descriptorSets.model = vulkanDevice->GetDescriptorSet({ &uniformBuffers.vsModel->m_descriptor }, {},
			layouts.model->m_descriptorSetLayout, layouts.model->m_setLayoutBindings);

		descriptorSets.offscreen = vulkanDevice->GetDescriptorSet({&uniformBuffers.vsOffScreen->m_descriptor }, {},
			layouts.model->m_descriptorSetLayout, layouts.model->m_setLayoutBindings);

		descriptorSets.offscreenBack = vulkanDevice->GetDescriptorSet({ &uniformBuffers.vsOffScreenBack->m_descriptor }, {},
			layouts.model->m_descriptorSetLayout, layouts.model->m_setLayoutBindings);

		descriptorSets.mirror = vulkanDevice->GetDescriptorSet({ &uniformBuffers.vsMirror->m_descriptor }, 
			{ &colorMap->m_descriptor, &paraboloidTexFront->m_descriptor, &paraboloidTexBack->m_descriptor, &envMap->m_descriptor },
			layouts.mirror->m_descriptorSetLayout, layouts.mirror->m_setLayoutBindings);

		models.example->AddDescriptor(descriptorSets.model);
		models.exampleoffscreen->AddDescriptor(descriptorSets.offscreen);
		models.exampleoffscreenback->AddDescriptor(descriptorSets.offscreenBack);
		models.plane->AddDescriptor(descriptorSets.mirror);
	}

	void setupPipelines()
	{
		offscreenRenderPass = vulkanDevice->GetRenderPass({ { paraboloidTexFront->m_format, paraboloidTexFront->m_descriptor.imageLayout },{ frontDepthTex->m_format, frontDepthTex->m_descriptor.imageLayout } });
		render::VulkanFrameBuffer* fb = vulkanDevice->GetFrameBuffer(offscreenRenderPass->GetRenderPass(), FB_DIM, FB_DIM, { paraboloidTexFront->m_descriptor.imageView, frontDepthTex->m_descriptor.imageView });
		fb->m_clearColor = { { 0.1f, 0.1f, 0.1f, 0.0f } };
		offscreenRenderPass->AddFrameBuffer(fb);

		offscreenBackRenderPass = vulkanDevice->GetRenderPass({ { paraboloidTexBack->m_format, paraboloidTexBack->m_descriptor.imageLayout },{ backDepthTex->m_format, backDepthTex->m_descriptor.imageLayout } });
		fb = vulkanDevice->GetFrameBuffer(offscreenBackRenderPass->GetRenderPass(), FB_DIM, FB_DIM, { paraboloidTexBack->m_descriptor.imageView, backDepthTex->m_descriptor.imageView });
		fb->m_clearColor = { { 0.1f, 0.1f, 0.1f, 0.0f } };
		offscreenBackRenderPass->AddFrameBuffer(fb);

		render::PipelineProperties props;

		//TODO that clip plane
		pipelines.model = vulkanDevice->GetPipeline(layouts.model->m_descriptorSetLayout, vertexLayout.m_vertexInputBindings, vertexLayout.m_vertexInputAttributes,
			engine::tools::getAssetPath() + "shaders/paraboloidreflections/phong.vert.spv", engine::tools::getAssetPath() + "shaders/paraboloidreflections/phong.frag.spv", mainRenderPass->GetRenderPass(), pipelineCache, props);

		pipelines.offscreen = vulkanDevice->GetPipeline(layouts.model->m_descriptorSetLayout, vertexLayout.m_vertexInputBindings, vertexLayout.m_vertexInputAttributes,
			engine::tools::getAssetPath() + "shaders/paraboloidreflections/paraboloidprojection.vert.spv", engine::tools::getAssetPath() + "shaders/paraboloidreflections/phong.frag.spv", offscreenRenderPass->GetRenderPass(), pipelineCache, props);
		
		pipelines.offscreenBack = vulkanDevice->GetPipeline(layouts.model->m_descriptorSetLayout, vertexLayout.m_vertexInputBindings, vertexLayout.m_vertexInputAttributes,
			engine::tools::getAssetPath() + "shaders/paraboloidreflections/paraboloidprojection.vert.spv", engine::tools::getAssetPath() + "shaders/paraboloidreflections/phong.frag.spv", offscreenBackRenderPass->GetRenderPass(), pipelineCache, props);

		pipelines.mirror = vulkanDevice->GetPipeline(layouts.mirror->m_descriptorSetLayout, vertexLayout.m_vertexInputBindings, vertexLayout.m_vertexInputAttributes,
			engine::tools::getAssetPath() + "shaders/paraboloidreflections/mirror.vert.spv", engine::tools::getAssetPath() + "shaders/paraboloidreflections/mirror.frag.spv", mainRenderPass->GetRenderPass(), pipelineCache);

		models.example->AddPipeline(pipelines.model);
		models.exampleoffscreen->AddPipeline(pipelines.offscreen);
		models.exampleoffscreenback->AddPipeline(pipelines.offscreen);
		models.plane->AddPipeline(pipelines.mirror);	
	}

	void BuildCommandBuffers()
	{
		VkCommandBufferBeginInfo cmdBufInfo{};
		cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		for (int32_t i = 0; i < drawCommandBuffers.size(); ++i)
		{
			VK_CHECK_RESULT(vkBeginCommandBuffer(drawCommandBuffers[i], &cmdBufInfo));

			offscreenRenderPass->Begin(drawCommandBuffers[i], 0);
			models.exampleoffscreen->Draw(drawCommandBuffers[i]);
			offscreenRenderPass->End(drawCommandBuffers[i]);

			offscreenBackRenderPass->Begin(drawCommandBuffers[i], 0);
			models.exampleoffscreenback->Draw(drawCommandBuffers[i]);
			offscreenBackRenderPass->End(drawCommandBuffers[i]);

			mainRenderPass->Begin(drawCommandBuffers[i], i);
			float depthBiasConstant = 2.25f;
			float depthBiasSlope = 1.75f;
			vkCmdSetDepthBias(
				drawCommandBuffers[i],
				depthBiasConstant,
				0.0f,
				depthBiasSlope);
			//draw here
			models.example->Draw(drawCommandBuffers[i]);
			models.plane->Draw(drawCommandBuffers[i]);

			DrawUI(drawCommandBuffers[i]);

			mainRenderPass->End(drawCommandBuffers[i]);

			VK_CHECK_RESULT(vkEndCommandBuffer(drawCommandBuffers[i]));
		}
	}

	void Prepare()
	{
		
		init();
		setupDescriptorPool();
		PrepareUI();
		prepareUniformBuffers();
		setupDescriptors();
		setupPipelines();
		BuildCommandBuffers();
		prepared = true;
	}

	virtual void update(float dt)
	{
		updateModelsMatrix();
		updateUniformBuffers();
		updateUniformBufferOffscreen();
	}

	virtual void ViewChanged()
	{
		//updateUniformBuffers();
	}

	virtual void OnUpdateUIOverlay(engine::scene::UIOverlay* overlay)
	{
		if (overlay->header("Settings")) {

		}
	}

};

VULKAN_EXAMPLE_MAIN()