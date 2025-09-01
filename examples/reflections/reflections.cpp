
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
		engine::scene::SimpleModel* quad = new scene::SimpleModel;
		engine::scene::SimpleModel* plane = new scene::SimpleModel;
	} models;

	VkDescriptorPool descriptorPool;

	struct UBO {
		glm::mat4 projection;
		glm::mat4 model;
		glm::vec4 lightPos = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	} uboShared;

	struct {
		render::VulkanBuffer* vsOffScreen;
		render::VulkanBuffer* vsMirror;
		render::VulkanBuffer* vsModel;
		render::VulkanBuffer* vsDebugQuad;
	} uniformBuffers;

	render::VulkanTexture* colorMap;
	render::VulkanTexture* colorTex;
	render::VulkanTexture* depthTex;

	struct {
		render::VulkanDescriptorSetLayout* model;
		render::VulkanDescriptorSetLayout* mirror;
	} layouts;

	struct {
		render::VulkanPipeline* offscreen;
		render::VulkanPipeline* mirror;
		render::VulkanPipeline* model;
		render::VulkanPipeline* debugQuad;
	} pipelines;

	struct {
		render::VulkanDescriptorSet* offscreen;
		render::VulkanDescriptorSet* mirror;
		render::VulkanDescriptorSet* model;
		render::VulkanDescriptorSet* debugQuad;
	} descriptorSets;

	glm::vec3 meshPos = glm::vec3(0.0f, -1.5f, 0.0f);
	glm::vec3 meshRot = glm::vec3(0.0f);

	render::VulkanRenderPass* offscreenRenderPass;

	VulkanExample() : VulkanApplication(true)
	{
		zoom = -3.75f;
		rotationSpeed = 0.5f;
		rotation = glm::vec3(0.0f, 0.f, 0.0f);
		title = "Reflections example";
		settings.overlay = true;
		camera.movementSpeed = 20.5f;
		camera.SetPerspective(60.0f, (float)width / (float)height, 0.1f, 1024.0f);
		camera.SetRotation(glm::vec3(0.0f, 0.0f, 0.0f));
		camera.SetPosition(glm::vec3(0.0f, -5.0f, -5.0f));

		// The scene shader uses a clipping plane, so this feature has to be enabled
		enabledFeatures.shaderClipDistance = VK_TRUE;
	}

	~VulkanExample()
	{
		// Clean up used Vulkan resources 
		// Note : Inherited destructor cleans up resources stored in base class
		delete models.example;
		delete models.exampleoffscreen;
		delete models.quad;
		delete models.plane;
	}

	void init()
	{
		models.plane->LoadGeometry(engine::tools::getAssetPath() + "models/plane.obj", &vertexLayout, 0.5f, 1);
		models.example->LoadGeometry(engine::tools::getAssetPath() + "models/chinesedragon.dae", &vertexLayout, 0.3f, 1);	

		render::Texture2DData data;
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
		}
		data.Destroy();

		colorTex = vulkanDevice->GetColorRenderTarget(FB_DIM, FB_DIM, FB_COLOR_FORMAT);
		depthTex = vulkanDevice->GetDepthRenderTarget(FB_DIM, FB_DIM, false);

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

		//setup layouts
		std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> modelbindings
		{
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT}
		};

		std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> mirrorbindings
		{
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
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
			VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 6},
			VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 8}
		};
		descriptorPool = vulkanDevice->CreateDescriptorSetsPool(poolSizes, 4);
	}

	void prepareUniformBuffers()
	{
		uniformBuffers.vsModel = vulkanDevice->GetBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, sizeof(uboShared));

		uniformBuffers.vsMirror = vulkanDevice->GetBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, sizeof(uboShared));

		uniformBuffers.vsOffScreen = vulkanDevice->GetBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, sizeof(uboShared));

		uniformBuffers.vsDebugQuad = vulkanDevice->GetBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, sizeof(uboShared));


		// Map persistent
		VK_CHECK_RESULT(uniformBuffers.vsModel->Map());
		VK_CHECK_RESULT(uniformBuffers.vsMirror->Map());
		VK_CHECK_RESULT(uniformBuffers.vsOffScreen->Map());
		VK_CHECK_RESULT(uniformBuffers.vsDebugQuad->Map());
		updateUniformBuffers();
		updateUniformBufferOffscreen();
	}

	void updateUniformBufferOffscreen()
	{
		uboShared.projection = glm::perspective(glm::radians(60.0f), (float)width / (float)height, 0.1f, 256.0f);
		glm::mat4 viewMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, zoom));

		uboShared.model = viewMatrix * glm::translate(glm::mat4(1.0f), cameraPos);
		uboShared.model = glm::rotate(uboShared.model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		uboShared.model = glm::rotate(uboShared.model, glm::radians(rotation.y + meshRot.y), glm::vec3(0.0f, 1.0f, 0.0f));
		uboShared.model = glm::rotate(uboShared.model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

		uboShared.model = glm::scale(uboShared.model, glm::vec3(1.0f, -1.0f, 1.0f));
		uboShared.model = glm::translate(uboShared.model, meshPos);

		uniformBuffers.vsOffScreen->MemCopy(&uboShared, sizeof(uboShared));
	}

	void updateUniformBuffers()
	{
		// Mesh
		uboShared.projection = glm::perspective(glm::radians(60.0f), (float)width / (float)height, 0.1f, 256.0f);
		glm::mat4 viewMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, zoom));

		uboShared.model = viewMatrix * glm::translate(glm::mat4(1.0f), cameraPos);
		uboShared.model = glm::rotate(uboShared.model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		uboShared.model = glm::rotate(uboShared.model, glm::radians(rotation.y + meshRot.y), glm::vec3(0.0f, 1.0f, 0.0f));
		uboShared.model = glm::rotate(uboShared.model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

		uboShared.model = glm::translate(uboShared.model, meshPos);

		uniformBuffers.vsModel->MemCopy(&uboShared, sizeof(uboShared));

		// Mirror
		uboShared.model = viewMatrix * glm::translate(glm::mat4(1.0f), cameraPos);
		uboShared.model = glm::rotate(uboShared.model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		uboShared.model = glm::rotate(uboShared.model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		uboShared.model = glm::rotate(uboShared.model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

		uniformBuffers.vsMirror->MemCopy(&uboShared, sizeof(uboShared));

		// Debug quad
		uboShared.projection = glm::ortho(4.0f, 0.0f, 0.0f, 4.0f * (float)height / (float)width, -1.0f, 1.0f);
		uboShared.model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));

		uniformBuffers.vsDebugQuad->MemCopy(&uboShared, sizeof(uboShared));
	}

	void setupDescriptors()
	{
		descriptorSets.model = vulkanDevice->GetDescriptorSet(descriptorPool, { &uniformBuffers.vsModel->m_descriptor }, {},
			layouts.model->m_descriptorSetLayout, layouts.model->m_setLayoutBindings);

		descriptorSets.offscreen = vulkanDevice->GetDescriptorSet(descriptorPool, {&uniformBuffers.vsOffScreen->m_descriptor }, {},
			layouts.model->m_descriptorSetLayout, layouts.model->m_setLayoutBindings);

		descriptorSets.mirror = vulkanDevice->GetDescriptorSet(descriptorPool, { &uniformBuffers.vsMirror->m_descriptor }, { &colorMap->m_descriptor, &colorTex->m_descriptor },
			layouts.mirror->m_descriptorSetLayout, layouts.mirror->m_setLayoutBindings);


		models.example->AddDescriptor(descriptorSets.model);
		models.exampleoffscreen->AddDescriptor(descriptorSets.offscreen);
		models.plane->AddDescriptor(descriptorSets.mirror);
	}

	void setupPipelines()
	{
		offscreenRenderPass = vulkanDevice->GetRenderPass({ { colorTex->m_format, colorTex->m_descriptor.imageLayout },{ depthTex->m_format, depthTex->m_descriptor.imageLayout } });
		render::VulkanFrameBuffer* fb = vulkanDevice->GetFrameBuffer(offscreenRenderPass->GetRenderPass(), FB_DIM, FB_DIM, { colorTex->m_descriptor.imageView, depthTex->m_descriptor.imageView });
		fb->m_clearColor = { { 0.1f, 0.1f, 0.1f, 0.0f } };
		offscreenRenderPass->AddFrameBuffer(fb);

		//TODO that clip plane
		pipelines.model = vulkanDevice->GetPipeline(layouts.model->m_descriptorSetLayout, vertexLayout.m_vertexInputBindings, vertexLayout.m_vertexInputAttributes,
			engine::tools::getAssetPath() + "shaders/offscreen/phong.vert.spv", engine::tools::getAssetPath() + "shaders/offscreen/phong.frag.spv", mainRenderPass->GetRenderPass(), pipelineCache);

		pipelines.offscreen = vulkanDevice->GetPipeline(layouts.model->m_descriptorSetLayout, vertexLayout.m_vertexInputBindings, vertexLayout.m_vertexInputAttributes,
			engine::tools::getAssetPath() + "shaders/offscreen/phong.vert.spv", engine::tools::getAssetPath() + "shaders/offscreen/phong.frag.spv", offscreenRenderPass->GetRenderPass(), pipelineCache);

		pipelines.mirror = vulkanDevice->GetPipeline(layouts.mirror->m_descriptorSetLayout, vertexLayout.m_vertexInputBindings, vertexLayout.m_vertexInputAttributes,
			engine::tools::getAssetPath() + "shaders/offscreen/mirror.vert.spv", engine::tools::getAssetPath() + "shaders/offscreen/mirror.frag.spv", mainRenderPass->GetRenderPass(), pipelineCache);

		models.example->AddPipeline(pipelines.model);
		models.exampleoffscreen->AddPipeline(pipelines.offscreen);
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

			mainRenderPass->Begin(drawCommandBuffers[i], i);
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
		updateUniformBuffers();
		updateUniformBufferOffscreen();
	}

	virtual void ViewChanged()
	{
		updateUniformBuffers();
	}

	virtual void OnUpdateUIOverlay(engine::scene::UIOverlay* overlay)
	{
		if (overlay->header("Settings")) {

		}
	}

};

VULKAN_EXAMPLE_MAIN()