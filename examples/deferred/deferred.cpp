
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
#include "vulkanexamplebase.h"
#include "scene/SimpleModel.h"

using namespace engine;

class VulkanExample : public VulkanExampleBase
{
public:

	// Vertex layout for the models
	render::VertexLayout vertexLayout = render::VertexLayout({
		render::VERTEX_COMPONENT_POSITION,
		render::VERTEX_COMPONENT_UV,
		render::VERTEX_COMPONENT_COLOR,
		render::VERTEX_COMPONENT_NORMAL
		}, {});

	render::VertexLayout vertexLayoutInstanced = render::VertexLayout({
		render::VERTEX_COMPONENT_POSITION,
		render::VERTEX_COMPONENT_UV,
		render::VERTEX_COMPONENT_COLOR,
		render::VERTEX_COMPONENT_NORMAL
		},
		{ render::VERTEX_COMPONENT_POSITION });

	struct {
		scene::SimpleModel example;
		scene::SimpleModel quad;
		scene::SimpleModel plane;
	} models;

	struct UBO {
		glm::mat4 projection;
		glm::mat4 view;
		glm::mat4 model;
		//glm::vec4 lightPos = glm::vec4(5.0f, 0.0f, 0.0f, 1.0f);
	} uboShared;

#define LIGHTS_NO 50
	struct Light {
		glm::vec4 position;
		glm::vec3 color;
		float radius;
	};
	struct {
		Light lights[LIGHTS_NO];
		glm::vec4 viewPos;
		int debugDisplayTarget = 0;
	} uboDeferred;

	struct {
		render::VulkanBuffer* vsModel;
		render::VulkanBuffer* vsDebugQuad;
		render::VulkanBuffer* fsdeferred;
	} uniformBuffers;

	render::VulkanTexture* colorMap;
	render::VulkanTexture* colorTex;
	render::VulkanTexture* depthTex;
	render::VulkanTexture* scenecolor;
	render::VulkanTexture* scenepositions;
	render::VulkanTexture* scenenormals;
	render::VulkanTexture* scenedepth;

	struct {
		render::VulkanDescriptorSetLayout* model;
		render::VulkanDescriptorSetLayout* deferred;
	} layouts;

	struct {
		render::VulkanPipeline* plane;
		render::VulkanPipeline* model;
		render::VulkanPipeline* debugQuad;
		render::VulkanPipeline* deferred;
	} pipelines;

	struct {
		render::VulkanDescriptorSet* model;
		render::VulkanDescriptorSet* debugQuad;
		render::VulkanDescriptorSet* deferred;
	} descriptorSets;

	glm::vec3 meshPos = glm::vec3(0.0f, -1.5f, 0.0f);
	glm::vec3 meshRot = glm::vec3(0.0f);

	render::VulkanRenderPass* scenepass = nullptr;
	render::VulkanRenderPass* lightingpass = nullptr;

	glm::vec3 light_positions[LIGHTS_NO];
	glm::vec3 light_colors[LIGHTS_NO];

	VulkanExample() : VulkanExampleBase(true)
	{
		zoom = -6.75f;
		rotationSpeed = 0.5f;
		rotation = glm::vec3(0.0f, 0.f, 0.0f);
		title = "Deferred lighting example";
		settings.overlay = true;
		camera.type = scene::Camera::CameraType::lookat;
		camera.movementSpeed = 20.5f;
		camera.setPerspective(60.0f, (float)width / (float)height, 0.1f, 1024.0f);
		camera.setRotation(glm::vec3(0.0f, 0.0f, 0.0f));
		camera.setTranslation(glm::vec3(0.0f, 00.0f, -5.0f));
	}

	~VulkanExample()
	{
		// Clean up used Vulkan resources 
		// Note : Inherited destructor cleans up resources stored in base class
	}

	float randomFloat() {
		return (float)rand() / ((float)RAND_MAX + 1);
	}

	void init()
	{
		scenecolor = vulkanDevice->GetColorRenderTarget(width, height, FB_COLOR_FORMAT);
		scenepositions = vulkanDevice->GetColorRenderTarget(width, height, VK_FORMAT_R16G16B16A16_SFLOAT);
		scenenormals = vulkanDevice->GetColorRenderTarget(width, height, VK_FORMAT_R16G16B16A16_SFLOAT);
		scenedepth = vulkanDevice->GetDepthRenderTarget(width, height, false);

		scenepass = vulkanDevice->GetRenderPass({ scenecolor ,scenepositions, scenenormals, scenedepth }, 0);
		render::VulkanFrameBuffer* fb = vulkanDevice->GetFrameBuffer(scenepass->GetRenderPass(), width, height, 
			{ scenecolor->m_descriptor.imageView, scenepositions->m_descriptor.imageView, scenenormals->m_descriptor.imageView, scenedepth->m_descriptor.imageView },
			{ { 0.95f, 0.95f, 0.95f, 1.0f } });
		scenepass->AddFrameBuffer(fb);

		models.plane.LoadGeometry(engine::tools::getAssetPath() + "models/plane.obj", &vertexLayout, 10.0f, 1, glm::vec3(0.0,1.5,0.0));
		models.example.LoadGeometry(engine::tools::getAssetPath() + "models/chinesedragon.dae", &vertexLayoutInstanced, 0.3f, LIGHTS_NO, glm::vec3(0.0, 0.0, 0.0));

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

		std::vector<glm::vec3> models_positions;
		models_positions.resize(LIGHTS_NO);

		for (int i = 0; i < LIGHTS_NO; i++)
		{
			float x = randomFloat() * 100 - 50;
			float y = randomFloat() * 100 - 50;
			light_positions[i].x = x;
			light_positions[i].z = y;
			light_positions[i].y = -1.0f;

			models_positions[i] = glm::vec3(x, 0, y);

			light_colors[i].x = randomFloat();
			light_colors[i].y = randomFloat();
			light_colors[i].z = randomFloat();
		}

		for (auto geo : models.plane.m_geometries)
		{
			geo->SetIndexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geo->m_indexCount * sizeof(uint32_t), geo->m_indices));
			geo->SetVertexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geo->m_verticesSize * sizeof(float), geo->m_vertices));
		}

		for (auto geo : models.example.m_geometries)
		{
			geo->SetIndexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geo->m_indexCount * sizeof(uint32_t), geo->m_indices));
			geo->SetVertexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geo->m_verticesSize * sizeof(float), geo->m_vertices));
			geo->SetInstanceBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
				queue, models_positions.size() * sizeof(glm::vec3), models_positions.data(),
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));
		}

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

		models.example.SetDescriptorSetLayout(layouts.model);

		std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> deferredbindings
		{
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT}
		};
		layouts.deferred = vulkanDevice->GetDescriptorSetLayout(deferredbindings);
		
	}

	void setupDescriptorPool()
	{
		// Example uses three ubos and two image samplers
		std::vector<VkDescriptorPoolSize> poolSizes = {
			engine::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 6),
			engine::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 8)
		};
		vulkanDevice->CreateDescriptorSetsPool(poolSizes, 4);
	}

	void prepareUniformBuffers()
	{
		uniformBuffers.vsModel = vulkanDevice->GetBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, sizeof(uboShared));

		uniformBuffers.fsdeferred = vulkanDevice->GetBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, sizeof(uboDeferred));

		uniformBuffers.vsDebugQuad = vulkanDevice->GetBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, sizeof(uboShared));


		// Map persistent
		VK_CHECK_RESULT(uniformBuffers.vsModel->map());
		VK_CHECK_RESULT(uniformBuffers.fsdeferred->map());
		VK_CHECK_RESULT(uniformBuffers.vsDebugQuad->map());
		updateUniformBuffers();
	}

	void updateUniformBuffers()
	{
		// Mesh
		uboShared.projection = glm::perspective(glm::radians(60.0f), (float)width / (float)height, 0.1f, 256.0f);
		glm::mat4 viewMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, zoom));

		uboShared.view = viewMatrix * glm::translate(glm::mat4(1.0f), cameraPos);
		uboShared.view = glm::rotate(uboShared.view, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		uboShared.view = glm::rotate(uboShared.view, glm::radians(rotation.y + meshRot.y), glm::vec3(0.0f, 1.0f, 0.0f));
		uboShared.view = glm::rotate(uboShared.view, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

		uboShared.model = glm::translate(uboShared.model, meshPos);

		memcpy(uniformBuffers.vsModel->m_mapped, &uboShared, sizeof(uboShared));

		// Debug quad
		uboShared.projection = glm::ortho(4.0f, 0.0f, 0.0f, 4.0f * (float)height / (float)width, -1.0f, 1.0f);
		uboShared.model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));

		memcpy(uniformBuffers.vsDebugQuad->m_mapped, &uboShared, sizeof(uboShared));

		for (int i = 0; i < LIGHTS_NO; i++)
		{
			uboDeferred.lights[i].radius = 0.25f;
		}
		memcpy(uniformBuffers.fsdeferred->m_mapped, &uboDeferred, sizeof(uboDeferred));
	}

	void setupDescriptors()
	{
		descriptorSets.model = vulkanDevice->GetDescriptorSet({ &uniformBuffers.vsModel->m_descriptor }, {},
			layouts.model->m_descriptorSetLayout, layouts.model->m_setLayoutBindings);

		models.example.AddDescriptor(descriptorSets.model);
		models.plane.AddDescriptor(descriptorSets.model);

		descriptorSets.deferred = vulkanDevice->GetDescriptorSet({ &uniformBuffers.fsdeferred->m_descriptor },
			{ &scenecolor->m_descriptor , &scenepositions->m_descriptor, &scenenormals->m_descriptor }, layouts.deferred->m_descriptorSetLayout, layouts.deferred->m_setLayoutBindings);

	}

	void setupPipelines()
	{

		std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStates;
		blendAttachmentStates.push_back(engine::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE));
		blendAttachmentStates.push_back(engine::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE));
		blendAttachmentStates.push_back(engine::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE));

		pipelines.plane = vulkanDevice->GetPipeline(layouts.model->m_descriptorSetLayout, vertexLayout.m_vertexInputBindings, vertexLayout.m_vertexInputAttributes,
			engine::tools::getAssetPath() + "shaders/basicdeferred/basictexturedcolored.vert.spv", engine::tools::getAssetPath() + "shaders/basicdeferred/basiccolored.frag.spv", 
			scenepass->GetRenderPass(), pipelineCache, false, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, nullptr, blendAttachmentStates.size(), blendAttachmentStates.data());
		pipelines.model = vulkanDevice->GetPipeline(layouts.model->m_descriptorSetLayout, vertexLayoutInstanced.m_vertexInputBindings, vertexLayoutInstanced.m_vertexInputAttributes,
			engine::tools::getAssetPath() + "shaders/basicdeferred/basictexturedcoloredinstanced.vert.spv", engine::tools::getAssetPath() + "shaders/basicdeferred/basiccolored.frag.spv",
			scenepass->GetRenderPass(), pipelineCache, false, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, nullptr, blendAttachmentStates.size(), blendAttachmentStates.data());
		models.example.AddPipeline(pipelines.model);
		models.plane.AddPipeline(pipelines.plane);

		pipelines.deferred = vulkanDevice->GetPipeline(layouts.deferred->m_descriptorSetLayout, {}, {},
			engine::tools::getAssetPath() + "shaders/posteffects/screenquad.vert.spv", engine::tools::getAssetPath() + "shaders/basicdeferred/deferredlighting.frag.spv",
			mainRenderPass->GetRenderPass(), pipelineCache, false, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	}

	void buildCommandBuffers()
	{
		VkCommandBufferBeginInfo cmdBufInfo{};
		cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		for (int32_t i = 0; i < drawCmdBuffers.size(); ++i)
		{
			VK_CHECK_RESULT(vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufInfo));

			scenepass->Begin(drawCmdBuffers[i], 0);
			models.example.Draw(drawCmdBuffers[i]);
			models.plane.Draw(drawCmdBuffers[i]);
			scenepass->End(drawCmdBuffers[i]);

			mainRenderPass->Begin(drawCmdBuffers[i], i);
			//draw here
			pipelines.deferred->Draw(drawCmdBuffers[i]);
			descriptorSets.deferred->Draw(drawCmdBuffers[i], pipelines.deferred->getPipelineLayout(), 0);
			vkCmdDraw(drawCmdBuffers[i], 3, 1, 0, 0);

			drawUI(drawCmdBuffers[i]);

			mainRenderPass->End(drawCmdBuffers[i]);

			VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
		}
	}

	void prepare()
	{
		VulkanExampleBase::prepare();
		init();
		setupDescriptorPool();
		prepareUI();
		prepareUniformBuffers();
		setupDescriptors();
		setupPipelines();
		buildCommandBuffers();
		prepared = true;
	}

	void draw()
	{
		VulkanExampleBase::prepareFrame();

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

		VulkanExampleBase::submitFrame();
	}

	virtual void render()
	{
		if (!prepared)
			return;
		updateUniformBuffers();
		draw();
	}

	virtual void update(float dt)
	{
		for (int i = 0; i < LIGHTS_NO; i++)
		{
			uboDeferred.lights[i].position.x = light_positions[i].x + cos(glm::radians(timer * 360.0f)) * 3.0f;
			uboDeferred.lights[i].position.z = light_positions[i].z + sin(glm::radians(timer * 360.0f)) * 3.0f;
			uboDeferred.lights[i].position.y = -1.0f;

			uboDeferred.lights[i].color.x = light_colors[i].x;
			uboDeferred.lights[i].color.y = light_colors[i].y;
			uboDeferred.lights[i].color.z = light_colors[i].z;
		}
	}

	virtual void viewChanged()
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