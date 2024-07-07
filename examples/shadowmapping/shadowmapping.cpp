/*
* Vulkan Example - Shadow mapping for directional light sources
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <vector>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vulkan/vulkan.h>
#include "vulkanexamplebase.h"
#include "render/VulkanRenderPass.h"
#include "scene/SimpleModel.h"
#include "scene/UniformBuffersManager.h"

#define ENABLE_VALIDATION true

// 16 bits of depth is enough for such a small scene
#define DEPTH_FORMAT VK_FORMAT_D16_UNORM

// Shadowmap properties
#if defined(__ANDROID__)
#define SHADOWMAP_DIM 256
#else
#define SHADOWMAP_DIM 512
#endif
#define SHADOWMAP_FILTER VK_FILTER_LINEAR

// Offscreen frame buffer properties
#define FB_COLOR_FORMAT VK_FORMAT_R8G8B8A8_UNORM
using namespace engine;
using namespace engine::render;
class VulkanExample : public VulkanExampleBase
{
public:
	bool displayShadowMap = false;
	bool filterPCF = false;

	// Keep depth range as small as possible
	// for better shadow map precision
	float zNear = 1.0f;
	float zFar = 906.0f;

	// Depth bias (and slope) are used to avoid shadowing artefacts
	// Constant depth bias factor (always applied)
	float depthBiasConstant = 0.5f;
	// Slope depth bias factor, applied depending on polygon's slope
	float depthBiasSlope = 0.75f;

	glm::vec3 lightPos = glm::vec3();
	float lightFOV = 45.0f;

	scene::UniformBuffersManager uniform_manager;

	std::vector<scene::SimpleModel*> scenes;
	std::vector<scene::SimpleModel*> offscreen_scenes;
	std::vector<scene::SimpleModel*> trees;

	std::vector<std::string> sceneNames;
	int32_t sceneIndex = 0;

	struct {
		VulkanBuffer *scene;
		VulkanBuffer *offscreen;
		VulkanBuffer *debug;
		VulkanBuffer *tree;
	} uniformBuffers;

	struct {
		glm::mat4 projection;
		glm::mat4 model;
	} uboVSquad;

	struct {
		glm::mat4 projection;
		glm::mat4 view;
		glm::mat4 model;
		glm::mat4 depthBiasMVP;
		glm::vec3 lightPos;
	} uboVSscene;

	struct {
		glm::mat4 depthMVP;
	} uboOffscreenVS;

	struct {
		VertexLayout *offscreen_vlayout;
		VulkanDescriptorSetLayout *offscreen_layout;
		VertexLayout *scene_vlayout;
		VulkanDescriptorSetLayout *scene_layout;
		VulkanDescriptorSetLayout *filter_layout;
	} layouts;

	struct {
		VulkanPipeline *quad;
		VulkanPipeline *offscreen;
		VulkanPipeline *sceneShadow;
		VulkanPipeline *sceneShadowPCF;
		//VulkanPipeline* filterSM;
	} pipelines;

	struct {
		VulkanDescriptorSet *offscreen;
		VulkanDescriptorSet *scene;
		VulkanDescriptorSet *tree;
		//VulkanDescriptorSet *filterSM;
	} descriptorSets;

	VulkanTexture* trunktex;

	VulkanTexture* shadowtex = nullptr;
	VulkanTexture* shadowmapColor = nullptr;
	//VulkanTexture* shadowmapColorFiltered = nullptr;

	VulkanRenderPass *offscreenPass;
	//VulkanRenderPass *filterPass;

	struct {
		int32_t techniqueIndex = 0;
	} uboFS;
	render::VulkanBuffer* uniformBufferFS = nullptr;

	std::vector<std::string> techniquesNames;
	//int32_t techniqueIndex = 0;

	VulkanExample() : VulkanExampleBase(ENABLE_VALIDATION)
	{
		zoom = -20.0f;
		rotation = { -15.0f, -390.0f, 0.0f };
		title = "Projected shadow mapping example";
		timerSpeed *= 0.5f;
		settings.overlay = true;

		enabledFeatures.samplerAnisotropy = VK_TRUE;
	}

	~VulkanExample()
	{
		for (auto scene : scenes)
			delete scene;
		for (auto scene : offscreen_scenes)
			delete scene;

		delete layouts.offscreen_vlayout;
		delete layouts.scene_vlayout;
	}

	// Set up a separate render pass for the offscreen frame buffer
	// This is necessary as the offscreen frame buffer attachments use formats different to those from the example render pass
	void prepareOffscreenRenderpass()
	{
		shadowtex = vulkanDevice->GetDepthRenderTarget(SHADOWMAP_DIM, SHADOWMAP_DIM, true);
		shadowmapColor = vulkanDevice->GetColorRenderTarget(SHADOWMAP_DIM, SHADOWMAP_DIM, FB_COLOR_HDR_FORMAT);
		/*shadowmapColorFiltered = vulkanDevice->GetColorRenderTarget(SHADOWMAP_DIM, SHADOWMAP_DIM, FB_COLOR_HDR_FORMAT);*/
		offscreenPass = vulkanDevice->GetRenderPass({ { shadowmapColor->m_format, shadowmapColor->m_descriptor.imageLayout}, { shadowtex->m_format, shadowtex->m_descriptor.imageLayout }});
		VulkanFrameBuffer *fb = vulkanDevice->GetFrameBuffer(offscreenPass->GetRenderPass(), SHADOWMAP_DIM, SHADOWMAP_DIM, { shadowmapColor->m_descriptor.imageView, shadowtex->m_descriptor.imageView });
		offscreenPass->AddFrameBuffer(fb);

		/*filterPass = vulkanDevice->GetRenderPass({ shadowmapColorFiltered }, 0);
		fb = vulkanDevice->GetFrameBuffer(filterPass->GetRenderPass(), SHADOWMAP_DIM, SHADOWMAP_DIM, { shadowmapColorFiltered->m_descriptor.imageView }, { { 30.8f, 100.95f, 300.f, 1.0f } });
		filterPass->AddFrameBuffer(fb);*/
	}

	// Setup the offscreen framebuffer for rendering the scene from light's point-of-view to
	// The depth attachment of this framebuffer will then be used to sample from in the fragment shader of the shadowing pass
	void prepareOffscreenFramebuffer()
	{
		prepareOffscreenRenderpass();
	}

	

	void loadAssets()
	{
		// Vertex layout for the models

		layouts.offscreen_vlayout = new VertexLayout({ VERTEX_COMPONENT_POSITION }, {});
		layouts.scene_vlayout = new VertexLayout({
			VERTEX_COMPONENT_POSITION,
			VERTEX_COMPONENT_UV,
			VERTEX_COMPONENT_COLOR,
			VERTEX_COMPONENT_NORMAL
			},
			{});

		std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> bindings
		{
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT}
		};

		std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> offscreenbindings
		{
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT}
		};

		std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> filterbindings
		{
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
		};

		scenes.resize(2);
		offscreen_scenes.resize(2);
		scenes[0] = new scene::SimpleModel;
		scenes[1] = new scene::SimpleModel;
		offscreen_scenes[0] = new scene::SimpleModel;
		offscreen_scenes[1] = new scene::SimpleModel;
		scenes[0]->LoadGeometry(engine::tools::getAssetPath() + "models/vulkanscene_shadow.dae", layouts.scene_vlayout, 4.0f,1);
		//scenes[0]->LoadGeometry(engine::tools::getAssetPath() + "models/sponza/crytek-sponza-huge-vray.obj", layouts.scene_vlayout, 0.004f, 1);
		scenes[1]->LoadGeometry(engine::tools::getAssetPath() + "models/samplescene.dae", layouts.scene_vlayout, 0.25f, 1);

		for (int r = 0; r < offscreen_scenes.size(); r++)
		{
			offscreen_scenes[r]->_vertexLayout = scenes[r]->_vertexLayout;
		}

		sceneNames = {"Vulkan scene", "Teapots and pillars" };

		layouts.offscreen_layout = vulkanDevice->GetDescriptorSetLayout(offscreenbindings);
		layouts.scene_layout = vulkanDevice->GetDescriptorSetLayout(bindings);
		//layouts.filter_layout = vulkanDevice->GetDescriptorSetLayout(filterbindings);
		for (auto off : offscreen_scenes)
			off->SetDescriptorSetLayout(layouts.offscreen_layout);
		for (auto scene : scenes)
			scene->SetDescriptorSetLayout(layouts.scene_layout);

		for (auto scene : scenes)
		{
			for (auto geo : scene->m_geometries)
			{
				geo->SetIndexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geo->m_indexCount * sizeof(uint32_t), geo->m_indices));
				geo->SetVertexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geo->m_verticesSize * sizeof(float), geo->m_vertices));
			}
		}
		
		for (int i = 0;i < scenes.size();i++)
		{
			for (auto geo : scenes[i]->m_geometries)
			{
				scene::Geometry* mygeo = new scene::Geometry;
				*mygeo = *geo;
				offscreen_scenes[i]->m_geometries.push_back(mygeo);
			}
		}

		techniquesNames = { "Simple", "PCF", "Variance" };
	}

	void setupDescriptorPool()
	{
		// Example uses three ubos and two image samplers
		std::vector<VkDescriptorPoolSize> poolSizes = {
			engine::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 12),
			engine::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 6)
		};

		vulkanDevice->CreateDescriptorSetsPool(poolSizes, 4);
	}

	void setupDescriptorSets()
	{
		descriptorSets.offscreen = vulkanDevice->GetDescriptorSet({&uniformBuffers.offscreen->m_descriptor }, {},
			layouts.offscreen_layout->m_descriptorSetLayout, layouts.offscreen_layout->m_setLayoutBindings);

		descriptorSets.scene = vulkanDevice->GetDescriptorSet({ &uniformBuffers.scene->m_descriptor, &uniformBufferFS->m_descriptor }, { &shadowmapColor->m_descriptor },
			layouts.scene_layout->m_descriptorSetLayout, layouts.scene_layout->m_setLayoutBindings);

		for(auto scene:scenes)
		scene->AddDescriptor(descriptorSets.scene);

		for(auto off:offscreen_scenes)
		off->AddDescriptor(descriptorSets.offscreen);

		//descriptorSets.filterSM = vulkanDevice->GetDescriptorSet({}, { &shadowmapColor->m_descriptor }, layouts.filter_layout->m_descriptorSetLayout, layouts.filter_layout->m_setLayoutBindings);
	}

	void preparePipelines()
	{
		std::vector<VkVertexInputAttributeDescription> vertexInputAttributes;
		vertexInputAttributes.push_back(engine::initializers::vertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0));

		pipelines.offscreen = vulkanDevice->GetPipeline(layouts.offscreen_layout->m_descriptorSetLayout, layouts.scene_vlayout->m_vertexInputBindings, vertexInputAttributes,
			engine::tools::getAssetPath() + "shaders/shadowmapping/offscreen.vert.spv", engine::tools::getAssetPath() + "shaders/shadowmapping/offscreenvariancecolor.frag.spv", offscreenPass->GetRenderPass(), pipelineCache//);
			,false, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, nullptr, 0, nullptr, true);

		pipelines.sceneShadow = vulkanDevice->GetPipeline(layouts.scene_layout->m_descriptorSetLayout, layouts.scene_vlayout->m_vertexInputBindings, layouts.scene_vlayout->m_vertexInputAttributes,
			engine::tools::getAssetPath() + "shaders/shadowmapping/scene.vert.spv", engine::tools::getAssetPath() + "shaders/shadowmapping/scene.frag.spv", mainRenderPass->GetRenderPass(), pipelineCache);

		/*pipelines.filterSM = vulkanDevice->GetPipeline(layouts.filter_layout->m_descriptorSetLayout, {}, {},
			engine::tools::getAssetPath() + "shaders/posteffects/screenquad.vert.spv", engine::tools::getAssetPath() + "shaders/shadowmapping/gaussfilter.frag.spv",
			filterPass->GetRenderPass(), pipelineCache, false, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);*/
		

		for (auto off : offscreen_scenes)
			off->AddPipeline(pipelines.offscreen);

		for (auto scene : scenes)
			scene->AddPipeline(pipelines.sceneShadow);
	}

	// Prepare and initialize uniform buffer containing shader uniforms
	void prepareUniformBuffers()
	{
		// Debug quad vertex shader uniform buffer block		
		uniform_manager.SetDevice(device);
		uniform_manager.SetEngineDevice(vulkanDevice);	

		//uniformBuffers.tree = uniform_manager.GetGlobalUniformBuffer({ scene::UNIFORM_PROJECTION ,scene::UNIFORM_VIEW });
		uniformBuffers.offscreen = vulkanDevice->GetBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			sizeof(uboOffscreenVS));
		VK_CHECK_RESULT(uniformBuffers.offscreen->map());

		uniformBuffers.scene = vulkanDevice->GetBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			sizeof(uboVSscene));
	
		VK_CHECK_RESULT(uniformBuffers.scene->map());

		uniformBufferFS = vulkanDevice->GetUniformBuffer(sizeof(uboFS), true, queue);
		uniformBufferFS->map();

		updateLight();
		updateUniformBufferOffscreen();
		updateUniformBuffers();
	}

	void updateLight()
	{
		// Animate the light source
		lightPos.x = cos(glm::radians(timer * 360.0f)) * 40.0f;
		lightPos.y = -10.0f;// +sin(glm::radians(timer * 360.0f)) * 20.0f;
		lightPos.z = sin(glm::radians(timer * 360.0f)) * 20.0f;
	}

	void updateUniformBuffers()
	{
		// 3D scene
		uboVSscene.projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, zNear, zFar);

		uboVSscene.view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, zoom));
		uboVSscene.view = glm::rotate(uboVSscene.view, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		uboVSscene.view = glm::rotate(uboVSscene.view, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		uboVSscene.view = glm::rotate(uboVSscene.view, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

		uboVSscene.model = glm::mat4(1.0f);

		uboVSscene.lightPos = lightPos;

		uboVSscene.depthBiasMVP = uboOffscreenVS.depthMVP;

		memcpy(uniformBuffers.scene->m_mapped, &uboVSscene, sizeof(uboVSscene));

		uniformBufferFS->copyTo(&uboFS, sizeof(uboFS));

		/*uniform_manager.UpdateGlobalParams(scene::UNIFORM_PROJECTION, &uboVSscene.projection, 0, sizeof(camera.matrices.perspective));
		uniform_manager.UpdateGlobalParams(scene::UNIFORM_VIEW, &uboVSscene.view, 0, sizeof(camera.matrices.view));
		uniform_manager.Update();*/

	}

	void updateUniformBufferOffscreen()
	{
		// Matrix from light's point of view
		glm::mat4 depthProjectionMatrix = glm::perspective(glm::radians(lightFOV), 1.0f, zNear, zFar);
		glm::mat4 depthViewMatrix = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0, 1, 0));
		glm::mat4 depthModelMatrix = glm::mat4(1.0f);

		uboOffscreenVS.depthMVP = depthProjectionMatrix * depthViewMatrix * depthModelMatrix;

		memcpy(uniformBuffers.offscreen->m_mapped, &uboOffscreenVS, sizeof(uboOffscreenVS));

	}

	void buildCommandBuffers()
	{
		VkCommandBufferBeginInfo cmdBufInfo{};
		cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		VkClearValue clearValues[2];
		VkViewport viewport;
		VkRect2D scissor;
		VkDeviceSize offsets[1] = { 0 };

		for (int32_t i = 0; i < drawCmdBuffers.size(); ++i)
		{
			VK_CHECK_RESULT(vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufInfo));

			/*
				First render pass: Generate shadow map by rendering the scene from light's POV
			*/
			{

				offscreenPass->Begin(drawCmdBuffers[i], 0);

				// Set depth bias (aka "Polygon offset")
				// Required to avoid shadow mapping artefacts
				vkCmdSetDepthBias(
					drawCmdBuffers[i],
					depthBiasConstant,
					0.0f,
					depthBiasSlope);

				offscreen_scenes[sceneIndex]->Draw(drawCmdBuffers[i]);

				offscreenPass->End(drawCmdBuffers[i]);
			}

		/*	filterPass->Begin(drawCmdBuffers[i], 0);

			pipelines.filterSM->Draw(drawCmdBuffers[i]);
			descriptorSets.filterSM->Draw(drawCmdBuffers[i], pipelines.filterSM->getPipelineLayout(),0);
			vkCmdDraw(drawCmdBuffers[i], 3, 1, 0, 0);

			filterPass->End(drawCmdBuffers[i]);*/

			/*
				Note: Explicit synchronization is not required between the render pass, as this is done implicit via sub pass dependencies
			*/

			/*
				Second pass: Scene rendering with applied shadow map
			*/

			{
				mainRenderPass->Begin(drawCmdBuffers[i], i);

				// Visualize shadow map
				if (displayShadowMap) {

				}

				// 3D scene							
				scenes[sceneIndex]->Draw(drawCmdBuffers[i]);

				drawUI(drawCmdBuffers[i]);

				mainRenderPass->End(drawCmdBuffers[i]);
			}

			VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
		}
	}

	void draw()
	{
		VulkanExampleBase::prepareFrame();

		// Command buffer to be sumitted to the queue
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];

		// Submit to queue
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

		VulkanExampleBase::submitFrame();
	}

	void prepare()
	{
		VulkanExampleBase::prepare();
		loadAssets();
		prepareOffscreenFramebuffer();
		prepareUniformBuffers();
		preparePipelines();
		setupDescriptorPool();
		prepareUI();
		setupDescriptorSets();
		buildCommandBuffers();
		prepared = true;
	}

	virtual void render()
	{
		if (!prepared)
			return;
		draw();
		if (!paused || camera.updated)
		{
			updateLight();
			updateUniformBufferOffscreen();
			updateUniformBuffers();
		}
	}

	virtual void update(float dt)
	{

	}

	virtual void OnUpdateUIOverlay(engine::scene::UIOverlay *overlay)
	{
		if (overlay->header("Settings")) {
			if (overlay->comboBox("Scenes", &sceneIndex, sceneNames)) {
				buildCommandBuffers();
			}
			if (overlay->checkBox("Display shadow render target", &displayShadowMap)) {
				buildCommandBuffers();
			}
			if (overlay->comboBox("Technique", &uboFS.techniqueIndex, techniquesNames)) {
				//buildCommandBuffers();
				//uniformBufferFS->copyTo(&uboFS, sizeof(uboFS));
			}
		}
	}
};

VULKAN_EXAMPLE_MAIN()
