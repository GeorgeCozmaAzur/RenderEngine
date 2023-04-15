
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

#include "vulkanexamplebase.h"
#include "scene/SimpleModel.h"
#include "scene/UniformBuffersManager.h"

using namespace engine;

class VulkanExample : public VulkanExampleBase
{
public:

	render::VertexLayout vertexLayout = render::VertexLayout({
		render::VERTEX_COMPONENT_POSITION,
		render::VERTEX_COMPONENT_NORMAL,
		render::VERTEX_COMPONENT_UV
		
		}, {});

	engine::scene::SimpleModel plane;
	engine::scene::SimpleModel trunk;
	engine::scene::SimpleModel leaves;
	render::VulkanTexture* colorMap;
	render::VulkanTexture* trunktex;
	render::VulkanTexture* leavestex;
	//render::VulkanTexture* perlinnoise;

	render::VulkanBuffer* sceneVertexUniformBuffer;
	scene::UniformBuffersManager uniform_manager;
	struct {
		glm::vec4 wind;
		float advance = 0.5f;
	} modelUniformVS;
	render::VulkanBuffer* modelVertexUniformBuffer;

	struct {
		float advance = 0.5f;
	} modelUniformFS;
	render::VulkanBuffer* modelFragmentUniformBuffer;

	glm::vec4 light_pos = glm::vec4(5.0f, -5.0f, 0.0f, 1.0f);

	VulkanExample() : VulkanExampleBase(true)
	{
		zoom = -3.75f;
		rotationSpeed = 0.5f;
		rotation = glm::vec3(15.0f, 0.f, 0.0f);
		title = "Render Engine Empty Scene";
		settings.overlay = true;
		camera.type = scene::Camera::CameraType::firstperson;
		camera.movementSpeed = 5.0f;
		camera.setPerspective(60.0f, (float)width / (float)height, 0.1f, 1024.0f);
		camera.setRotation(glm::vec3(0.0f, 0.0f, 0.0f));
		camera.setTranslation(glm::vec3(0.0f, 1.0f, -1.0f));
	}

	~VulkanExample()
	{
		// Clean up used Vulkan resources 
		// Note : Inherited destructor cleans up resources stored in base class
		
	}

	void setupGeometry()
	{
		//Geometry
		plane.LoadGeometry(engine::tools::getAssetPath() + "models/plane.obj", &vertexLayout, 0.1f, 1);
		for (auto geo : plane.m_geometries)
		{
			geo->SetIndexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geo->m_indexCount * sizeof(uint32_t), geo->m_indices));
			geo->SetVertexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geo->m_verticesSize * sizeof(float), geo->m_vertices));
		}
		trunk.LoadGeometry(engine::tools::getAssetPath() + "models/oak_trunk.dae", &vertexLayout, 1.0f, 1);
		for (auto geo : trunk.m_geometries)
		{
			geo->SetIndexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geo->m_indexCount * sizeof(uint32_t), geo->m_indices));
			geo->SetVertexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geo->m_verticesSize * sizeof(float), geo->m_vertices));
		}
		leaves.LoadGeometry(engine::tools::getAssetPath() + "models/oak_leafs.dae", &vertexLayout, 1.0f, 1);
		for (auto geo : leaves.m_geometries)
		{
			geo->SetIndexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geo->m_indexCount * sizeof(uint32_t), geo->m_indices));
			geo->SetVertexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geo->m_verticesSize * sizeof(float), geo->m_vertices));
		}
	}

	void SetupTextures()
	{
		// Textures
		/*if (vulkanDevice->features.textureCompressionBC) {
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
		}*/

		colorMap = vulkanDevice->GetTexture(engine::tools::getAssetPath() + "textures/grass3K/GroundForest003_COL_VAR1_3K.jpg", VK_FORMAT_R8G8B8A8_UNORM, queue);
		

		trunktex = vulkanDevice->GetTexture(engine::tools::getAssetPath() + "textures/oak_bark.ktx", VK_FORMAT_R8G8B8A8_UNORM, queue);
		leavestex = vulkanDevice->GetTexture(engine::tools::getAssetPath() + "textures/oak_leafs.ktx", VK_FORMAT_R8G8B8A8_UNORM, queue);
		//perlinnoise = vulkanDevice->GetTexture(engine::tools::getAssetPath() + "textures/PerlinExample.png", VK_FORMAT_R8G8B8A8_UNORM, queue);
	}

	void SetupUniforms()
	{
		//uniforms
		uniform_manager.SetDevice(vulkanDevice->logicalDevice);
		uniform_manager.SetEngineDevice(vulkanDevice);
		sceneVertexUniformBuffer = uniform_manager.GetGlobalUniformBuffer({ scene::UNIFORM_PROJECTION ,scene::UNIFORM_VIEW ,scene::UNIFORM_LIGHT0_POSITION, scene::UNIFORM_CAMERA_POSITION });

		modelVertexUniformBuffer = vulkanDevice->GetUniformBuffer(sizeof(modelUniformVS), true, queue);
		modelVertexUniformBuffer->map();
		modelFragmentUniformBuffer = vulkanDevice->GetUniformBuffer(sizeof(modelUniformFS), true, queue);
		modelFragmentUniformBuffer->map();

		updateUniformBuffers();
	}

	//here a descriptor pool will be created for the entire app. Now it contains 1 sampler because this is what the ui overlay needs
	void setupDescriptorPool()
	{
		std::vector<VkDescriptorPoolSize> poolSizes = {
			engine::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 6),
			engine::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 6)
		};
		vulkanDevice->CreateDescriptorSetsPool(poolSizes, 4);
	}

	void SetupDescriptors()
	{
		//descriptors
		std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> modelbindings
		{
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
		};
		std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> treebindings
		{
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
		};
		std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> leavesbindings
		{
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
			//{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT}
		};
		plane.SetDescriptorSetLayout(vulkanDevice->GetDescriptorSetLayout(modelbindings));
		trunk.SetDescriptorSetLayout(vulkanDevice->GetDescriptorSetLayout(treebindings));
		leaves.SetDescriptorSetLayout(vulkanDevice->GetDescriptorSetLayout(leavesbindings));

		plane.AddDescriptor(vulkanDevice->GetDescriptorSet({ &sceneVertexUniformBuffer->m_descriptor }, {&colorMap->m_descriptor},
			plane._descriptorLayout->m_descriptorSetLayout, plane._descriptorLayout->m_setLayoutBindings));
		trunk.AddDescriptor(vulkanDevice->GetDescriptorSet({ &sceneVertexUniformBuffer->m_descriptor, &modelVertexUniformBuffer->m_descriptor }, { &trunktex->m_descriptor },
			trunk._descriptorLayout->m_descriptorSetLayout, trunk._descriptorLayout->m_setLayoutBindings));
		leaves.AddDescriptor(vulkanDevice->GetDescriptorSet({ &sceneVertexUniformBuffer->m_descriptor, &modelVertexUniformBuffer->m_descriptor, &modelFragmentUniformBuffer->m_descriptor }, { &leavestex->m_descriptor, /*&perlinnoise->m_descriptor*/ },
			leaves._descriptorLayout->m_descriptorSetLayout, leaves._descriptorLayout->m_setLayoutBindings));
	}

	void setupPipelines()
	{

		plane.AddPipeline(vulkanDevice->GetPipeline(plane._descriptorLayout->m_descriptorSetLayout, vertexLayout.m_vertexInputBindings, vertexLayout.m_vertexInputAttributes,
			engine::tools::getAssetPath() + "shaders/basic/phong.vert.spv", engine::tools::getAssetPath() + "shaders/basic/phongtextured.frag.spv", mainRenderPass->GetRenderPass(), pipelineCache));

		trunk.AddPipeline(vulkanDevice->GetPipeline(trunk._descriptorLayout->m_descriptorSetLayout, vertexLayout.m_vertexInputBindings, vertexLayout.m_vertexInputAttributes,
			engine::tools::getAssetPath() + "shaders/wind/tree.vert.spv", engine::tools::getAssetPath() + "shaders/wind/tree.frag.spv", mainRenderPass->GetRenderPass(), pipelineCache));

		leaves.AddPipeline(vulkanDevice->GetPipeline(leaves._descriptorLayout->m_descriptorSetLayout, vertexLayout.m_vertexInputBindings, vertexLayout.m_vertexInputAttributes,
			engine::tools::getAssetPath() + "shaders/wind/leaves.vert.spv", engine::tools::getAssetPath() + "shaders/wind/leaves.frag.spv", mainRenderPass->GetRenderPass(), pipelineCache));
	}

	void init()
	{	
		setupGeometry();
		SetupTextures();
		SetupUniforms();
		setupDescriptorPool();
		SetupDescriptors();
		setupPipelines();
	}

	

	void buildCommandBuffers()
	{
		VkCommandBufferBeginInfo cmdBufInfo = engine::initializers::commandBufferBeginInfo();

		for (int32_t i = 0; i < drawCmdBuffers.size(); ++i)
		{
			VK_CHECK_RESULT(vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufInfo));

			mainRenderPass->Begin(drawCmdBuffers[i], i);

			//draw here
			plane.Draw(drawCmdBuffers[i]);
			trunk.Draw(drawCmdBuffers[i]);
			leaves.Draw(drawCmdBuffers[i]);

			drawUI(drawCmdBuffers[i]);

			mainRenderPass->End(drawCmdBuffers[i]);

			VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
		}
	}

	void updateUniformBuffers()
	{
		uniform_manager.UpdateGlobalParams(scene::UNIFORM_PROJECTION, &camera.matrices.perspective, 0, sizeof(camera.matrices.perspective));
		uniform_manager.UpdateGlobalParams(scene::UNIFORM_VIEW, &camera.matrices.view, 0, sizeof(camera.matrices.view));
		uniform_manager.UpdateGlobalParams(scene::UNIFORM_LIGHT0_POSITION, &light_pos, 0, sizeof(light_pos));
		glm::vec3 cucu = camera.position;
		cucu.y = -cucu.y;
		uniform_manager.UpdateGlobalParams(scene::UNIFORM_CAMERA_POSITION, &cucu, 0, sizeof(camera.position));

		uniform_manager.Update();

		//modelUniformVS.wind = glm::vec4(0.0f, 0.0f, 1.0f, 0.5f);
		//modelVertexUniformBuffer->copyTo(&modelUniformVS, sizeof(modelUniformVS));
	}

	void draw()
	{
		VulkanExampleBase::prepareFrame();

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

		VulkanExampleBase::submitFrame();
	}

	void prepare()
	{
		VulkanExampleBase::prepare();
		init();
		
		prepareUI();
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
		modelUniformVS.advance += dt;
		modelUniformVS.wind = glm::vec4(1.0f, 0.0f, 0.0f, 0.1f);
		modelVertexUniformBuffer->copyTo(&modelUniformVS, sizeof(modelUniformVS));

		modelUniformFS.advance += dt;
		modelFragmentUniformBuffer->copyTo(&modelUniformFS, sizeof(modelUniformFS));
	}

	virtual void viewChanged()
	{
		updateUniformBuffers();
	}

	virtual void OnUpdateUIOverlay(engine::scene::UIOverlay *overlay)
	{
		if (overlay->header("Settings")) {

		}
	}

};

VULKAN_EXAMPLE_MAIN()