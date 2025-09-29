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
#include "VulkanApplication.h"
#include "render/vulkan/VulkanRenderPass.h"
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
class VulkanExample : public VulkanApplication
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
		VulkanVertexLayout*offscreen_vlayout;
		VulkanDescriptorSetLayout *offscreen_layout;
		VulkanVertexLayout *scene_vlayout;
		VulkanDescriptorSetLayout *scene_layout;
		VulkanDescriptorSetLayout *filter_layout;
	} layouts;

	VkDescriptorPool descriptorPool;

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

	VulkanExample() : VulkanApplication(ENABLE_VALIDATION)
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

		layouts.offscreen_vlayout = new VulkanVertexLayout({ VERTEX_COMPONENT_POSITION }, {});
		layouts.scene_vlayout = new VulkanVertexLayout({
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
			VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 12},
			VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 6}
		};

		descriptorPool = vulkanDevice->CreateDescriptorSetsPool(poolSizes, 4);
	}

	void setupDescriptorSets()
	{
		descriptorSets.offscreen = vulkanDevice->GetDescriptorSet(descriptorPool, {&uniformBuffers.offscreen->m_descriptor }, {},
			layouts.offscreen_layout->m_descriptorSetLayout, layouts.offscreen_layout->m_setLayoutBindings);

		descriptorSets.scene = vulkanDevice->GetDescriptorSet(descriptorPool, { &uniformBuffers.scene->m_descriptor, &uniformBufferFS->m_descriptor }, { &shadowmapColor->m_descriptor },
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
		vertexInputAttributes.push_back(VkVertexInputAttributeDescription{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 });

		render::PipelineProperties props;
		props.depthBias = true;
		pipelines.offscreen = vulkanDevice->GetPipeline(layouts.offscreen_layout->m_descriptorSetLayout, layouts.scene_vlayout->m_vertexInputBindings, vertexInputAttributes,
			engine::tools::getAssetPath() + "shaders/shadowmapping/offscreen.vert.spv", engine::tools::getAssetPath() + "shaders/shadowmapping/offscreenvariancecolor.frag.spv", offscreenPass->GetRenderPass(), pipelineCache, props);

		props.depthBias = false;
		pipelines.sceneShadow = vulkanDevice->GetPipeline(layouts.scene_layout->m_descriptorSetLayout, layouts.scene_vlayout->m_vertexInputBindings, layouts.scene_vlayout->m_vertexInputAttributes,
			engine::tools::getAssetPath() + "shaders/shadowmapping/scene.vert.spv", engine::tools::getAssetPath() + "shaders/shadowmapping/scene.frag.spv", mainRenderPass->GetRenderPass(), pipelineCache, props);

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
		VK_CHECK_RESULT(uniformBuffers.offscreen->Map());

		uniformBuffers.scene = vulkanDevice->GetBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			sizeof(uboVSscene));
	
		VK_CHECK_RESULT(uniformBuffers.scene->Map());

		uniformBufferFS = vulkanDevice->GetUniformBuffer(sizeof(uboFS), true, queue);
		uniformBufferFS->Map();

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

		uniformBuffers.scene->MemCopy(&uboVSscene, sizeof(uboVSscene));

		uniformBufferFS->MemCopy(&uboFS, sizeof(uboFS));

		/*uniform_manager.UpdateGlobalParams(scene::UNIFORM_PROJECTION, &uboVSscene.projection, 0, sizeof(perspectiveMatrix));
		uniform_manager.UpdateGlobalParams(scene::UNIFORM_VIEW, &uboVSscene.view, 0, sizeof(viewMatrix));
		uniform_manager.Update();*/

	}

	void updateUniformBufferOffscreen()
	{
		// Matrix from light's point of view
		glm::mat4 depthProjectionMatrix = glm::perspective(glm::radians(lightFOV), 1.0f, zNear, zFar);
		glm::mat4 depthViewMatrix = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0, 1, 0));
		glm::mat4 depthModelMatrix = glm::mat4(1.0f);

		uboOffscreenVS.depthMVP = depthProjectionMatrix * depthViewMatrix * depthModelMatrix;

		uniformBuffers.offscreen->MemCopy(&uboOffscreenVS, sizeof(uboOffscreenVS));

	}

	void BuildCommandBuffers()
	{
		VkCommandBufferBeginInfo cmdBufInfo{};
		cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		for (int32_t i = 0; i < drawCommandBuffers.size(); ++i)
		{
			VK_CHECK_RESULT(vkBeginCommandBuffer(drawCommandBuffers[i], &cmdBufInfo));

			/*
				First render pass: Generate shadow map by rendering the scene from light's POV
			*/
			{

				offscreenPass->Begin(drawCommandBuffers[i], 0);

				// Set depth bias (aka "Polygon offset")
				// Required to avoid shadow mapping artefacts
				vkCmdSetDepthBias(
					drawCommandBuffers[i],
					depthBiasConstant,
					0.0f,
					depthBiasSlope);

				offscreen_scenes[sceneIndex]->Draw(drawCommandBuffers[i]);

				offscreenPass->End(drawCommandBuffers[i]);
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
				mainRenderPass->Begin(drawCommandBuffers[i], i);

				// Visualize shadow map
				if (displayShadowMap) {

				}

				// 3D scene							
				scenes[sceneIndex]->Draw(drawCommandBuffers[i]);

				DrawUI(drawCommandBuffers[i]);

				mainRenderPass->End(drawCommandBuffers[i]);
			}

			VK_CHECK_RESULT(vkEndCommandBuffer(drawCommandBuffers[i]));
		}
	}

	void Prepare()
	{
		
		loadAssets();
		prepareOffscreenFramebuffer();
		prepareUniformBuffers();
		preparePipelines();
		setupDescriptorPool();
		PrepareUI();
		setupDescriptorSets();
		BuildCommandBuffers();
		prepared = true;
	}

	virtual void update(float dt)
	{
		if (!paused /*|| camera.updated*/)
		{
			updateLight();
			updateUniformBufferOffscreen();
			updateUniformBuffers();
		}
	}

	virtual void OnUpdateUIOverlay(engine::scene::UIOverlay *overlay)
	{
		if (overlay->header("Settings")) {
			if (overlay->comboBox("Scenes", &sceneIndex, sceneNames)) {
				BuildCommandBuffers();
			}
			if (overlay->checkBox("Display shadow render target", &displayShadowMap)) {
				BuildCommandBuffers();
			}
			if (overlay->comboBox("Technique", &uboFS.techniqueIndex, techniquesNames)) {
				//buildCommandBuffers();
				//uniformBufferFS->MemCopy(&uboFS, sizeof(uboFS));
			}
		}
	}
};

VULKAN_EXAMPLE_MAIN()
