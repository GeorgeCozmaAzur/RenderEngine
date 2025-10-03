
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

#include "VulkanApplication.h"
#include "scene/SimpleModel.h"
#include "scene/UniformBuffersManager.h"

using namespace engine;

class VulkanExample : public VulkanApplication
{
public:

	render::VulkanVertexLayout vertexLayout = render::VulkanVertexLayout({
		render::VERTEX_COMPONENT_POSITION,
		render::VERTEX_COMPONENT_NORMAL,
		render::VERTEX_COMPONENT_UV
		}, {});

	VkDescriptorPool descriptorPool;

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

	VulkanExample() : VulkanApplication(true)
	{
		zoom = -3.75f;
		rotationSpeed = 0.5f;
		rotation = glm::vec3(15.0f, 0.f, 0.0f);
		title = "Render Engine Empty Scene";
		settings.overlay = true;
		camera.movementSpeed = 5.0f;
		camera.SetPerspective(60.0f, (float)width / (float)height, 0.1f, 1024.0f);
		camera.SetRotation(glm::vec3(0.0f, 0.0f, 0.0f));
		camera.SetPosition(glm::vec3(0.0f, 1.0f, -1.0f));
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
		/*if (vulkanDevice->m_enabledFeatures.textureCompressionBC) {
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
		}*/

		render::Texture2DData data;
		data.LoadFromFile(engine::tools::getAssetPath() + "textures/grass3K/GroundForest003_COL_VAR1_3K.jpg", render::GfxFormat::R8G8B8A8_UNORM);
		colorMap = vulkanDevice->GetTexture(&data, queue);
		data.Destroy();
		
		data.LoadFromFile(engine::tools::getAssetPath() + "textures/oak_bark.ktx", render::GfxFormat::R8G8B8A8_UNORM);
		trunktex = vulkanDevice->GetTexture(&data, queue);
		data.Destroy();

		data.LoadFromFile(engine::tools::getAssetPath() + "textures/oak_leafs.ktx", render::GfxFormat::R8G8B8A8_UNORM);
		leavestex = vulkanDevice->GetTexture(&data, queue);
		//data.Destroy();
		//perlinnoise = vulkanDevice->GetTexture(engine::tools::getAssetPath() + "textures/PerlinExample.png", VK_FORMAT_R8G8B8A8_UNORM, queue);
	}

	void SetupUniforms()
	{
		//uniforms
		uniform_manager.SetDevice(vulkanDevice->logicalDevice);
		uniform_manager.SetEngineDevice(vulkanDevice);
		sceneVertexUniformBuffer = uniform_manager.GetGlobalUniformBuffer({ scene::UNIFORM_PROJECTION ,scene::UNIFORM_VIEW ,scene::UNIFORM_LIGHT0_POSITION, scene::UNIFORM_CAMERA_POSITION });

		modelVertexUniformBuffer = vulkanDevice->GetUniformBuffer(sizeof(modelUniformVS), true, queue);
		modelVertexUniformBuffer->Map();
		modelFragmentUniformBuffer = vulkanDevice->GetUniformBuffer(sizeof(modelUniformFS), true, queue);
		modelFragmentUniformBuffer->Map();

		updateUniformBuffers();
	}

	//here a descriptor pool will be created for the entire app. Now it contains 1 sampler because this is what the ui overlay needs
	void setupDescriptorPool()
	{
		std::vector<VkDescriptorPoolSize> poolSizes = {
			VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 6},
			VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 6}
		};
		descriptorPool = vulkanDevice->CreateDescriptorSetsPool(poolSizes, 4);
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

		plane.AddDescriptor(vulkanDevice->GetDescriptorSet(descriptorPool, { &sceneVertexUniformBuffer->m_descriptor }, {&colorMap->m_descriptor},
			plane._descriptorLayout->m_descriptorSetLayout, plane._descriptorLayout->m_setLayoutBindings));
		trunk.AddDescriptor(vulkanDevice->GetDescriptorSet(descriptorPool, { &sceneVertexUniformBuffer->m_descriptor, &modelVertexUniformBuffer->m_descriptor }, { &trunktex->m_descriptor },
			trunk._descriptorLayout->m_descriptorSetLayout, trunk._descriptorLayout->m_setLayoutBindings));
		leaves.AddDescriptor(vulkanDevice->GetDescriptorSet(descriptorPool, { &sceneVertexUniformBuffer->m_descriptor, &modelVertexUniformBuffer->m_descriptor, &modelFragmentUniformBuffer->m_descriptor }, { &leavestex->m_descriptor, /*&perlinnoise->m_descriptor*/ },
			leaves._descriptorLayout->m_descriptorSetLayout, leaves._descriptorLayout->m_setLayoutBindings));
	}

	void setupPipelines()
	{
		render::PipelineProperties props;
		plane.AddPipeline(vulkanDevice->GetPipeline(plane._descriptorLayout->m_descriptorSetLayout, vertexLayout.m_vertexInputBindings, vertexLayout.m_vertexInputAttributes,
			engine::tools::getAssetPath() + "shaders/basic/phong.vert.spv", engine::tools::getAssetPath() + "shaders/basic/phongtextured.frag.spv", mainRenderPass->GetRenderPass(), pipelineCache, props));

		trunk.AddPipeline(vulkanDevice->GetPipeline(trunk._descriptorLayout->m_descriptorSetLayout, vertexLayout.m_vertexInputBindings, vertexLayout.m_vertexInputAttributes,
			engine::tools::getAssetPath() + "shaders/wind/tree.vert.spv", engine::tools::getAssetPath() + "shaders/wind/tree.frag.spv", mainRenderPass->GetRenderPass(), pipelineCache, props));

		leaves.AddPipeline(vulkanDevice->GetPipeline(leaves._descriptorLayout->m_descriptorSetLayout, vertexLayout.m_vertexInputBindings, vertexLayout.m_vertexInputAttributes,
			engine::tools::getAssetPath() + "shaders/wind/leaves.vert.spv", engine::tools::getAssetPath() + "shaders/wind/leaves.frag.spv", mainRenderPass->GetRenderPass(), pipelineCache, props));
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

	

	void BuildCommandBuffers()
	{
		VkCommandBufferBeginInfo cmdBufInfo{};
		cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		for (int32_t i = 0; i < drawCommandBuffers.size(); ++i)
		{
			VK_CHECK_RESULT(vkBeginCommandBuffer(drawCommandBuffers[i], &cmdBufInfo));

			mainRenderPass->Begin(drawCommandBuffers[i], i);

			//draw here
			plane.Draw(drawCommandBuffers[i]);
			trunk.Draw(drawCommandBuffers[i]);
			leaves.Draw(drawCommandBuffers[i]);

			DrawUI(drawCommandBuffers[i]);

			mainRenderPass->End(drawCommandBuffers[i]);

			VK_CHECK_RESULT(vkEndCommandBuffer(drawCommandBuffers[i]));
		}
	}

	void updateUniformBuffers()
	{
		glm::mat4 perspectiveMatrix = camera.GetPerspectiveMatrix();
		glm::mat4 viewMatrix = camera.GetViewMatrix();

		uniform_manager.UpdateGlobalParams(scene::UNIFORM_PROJECTION, &perspectiveMatrix, 0, sizeof(perspectiveMatrix));
		uniform_manager.UpdateGlobalParams(scene::UNIFORM_VIEW, &viewMatrix, 0, sizeof(viewMatrix));
		uniform_manager.UpdateGlobalParams(scene::UNIFORM_LIGHT0_POSITION, &light_pos, 0, sizeof(light_pos));
		glm::vec3 cucu = camera.GetPosition();
		cucu.y = -cucu.y;
		uniform_manager.UpdateGlobalParams(scene::UNIFORM_CAMERA_POSITION, &cucu, 0, sizeof(camera.GetPosition()));

		modelUniformVS.wind = glm::vec4(1.0f, 0.0f, 0.0f, 0.5f);

		uniform_manager.Update(queue);
	}

	void Prepare()
	{	
		init();
		PrepareUI();
		BuildCommandBuffers();
		prepared = true;
	}

	virtual void update(float dt)
	{
		modelUniformVS.advance += dt;
		modelVertexUniformBuffer->MemCopy(&modelUniformVS, sizeof(modelUniformVS));

		modelUniformFS.advance += dt;
		modelFragmentUniformBuffer->MemCopy(&modelUniformFS, sizeof(modelUniformFS));
	}

	virtual void ViewChanged()
	{
		updateUniformBuffers();
	}

	virtual void OnUpdateUIOverlay(engine::scene::UIOverlay *overlay)
	{
		float v[3] = { modelUniformVS.wind.x, modelUniformVS.wind.y, modelUniformVS.wind.z};
		if (overlay->header("Settings")) {
			if (ImGui::SliderFloat3("Wind direction", v, 0.0f, 1.0f))
			{
				modelUniformVS.wind.x = v[0];
				modelUniformVS.wind.y = v[1];
				modelUniformVS.wind.z = v[2];
			}
			if (ImGui::SliderFloat("Wind intensity", &modelUniformVS.wind.w, 0.0f, 1.0f))
			{

			}
		}
	}

};

VULKAN_EXAMPLE_MAIN()