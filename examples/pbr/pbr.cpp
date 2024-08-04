
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

	engine::scene::SimpleModel basicModel;
	engine::scene::SimpleModel basicModelTextured;
	
	render::VulkanTexture* colorMap;
	render::VulkanTexture* roughnessMap;
	render::VulkanTexture* metallicMap;
	render::VulkanTexture* aoMap;
	
	struct {
		glm::vec4 albedo = glm::vec4(1.0,0.0,0.0,1.0);
		glm::vec4 lightPosition;
		glm::vec4 cameraPosition;
		float roughness = 0.5f;
		float metallic = 0.5f;
		float ao = 0.0;
	} modelUniformFS;

	struct {
		glm::vec4 lightPosition;
		glm::vec4 cameraPosition;
	} modelUniformTexturedFS;

	render::VulkanBuffer* modelFragmentUniformBuffer;
	render::VulkanBuffer* modelFragmentTexturedUniformBuffer;

	render::VulkanBuffer* sceneVertexUniformBuffer;
	scene::UniformBuffersManager uniform_manager;

	glm::vec4 light_pos = glm::vec4(0.0f, -5.0f, 0.0f, 1.0f);

	bool textured = false;

	VulkanExample() : VulkanExampleBase(true)
	{
		zoom = -3.75f;
		rotationSpeed = 0.5f;
		rotation = glm::vec3(15.0f, 0.f, 0.0f);
		title = "Render Engine Empty Scene";
		settings.overlay = true;
		camera.type = scene::Camera::CameraType::firstperson;
		camera.movementSpeed = 20.5f;
		camera.setPerspective(60.0f, (float)width / (float)height, 0.1f, 1024.0f);
		camera.setRotation(glm::vec3(0.0f, 0.0f, 0.0f));
		camera.setTranslation(glm::vec3(0.0f, 0.0f, -5.0f));
	}

	~VulkanExample()
	{
		// Clean up used Vulkan resources 
		// Note : Inherited destructor cleans up resources stored in base class
		
	}

	void setupGeometry()
	{
		//Geometry
		basicModel.LoadGeometry(engine::tools::getAssetPath() + "models/sphere.obj", &vertexLayout, 0.1f, 1);
		for (auto geo : basicModel.m_geometries)
		{
			geo->SetIndexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geo->m_indexCount * sizeof(uint32_t), geo->m_indices));
			geo->SetVertexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geo->m_verticesSize * sizeof(float), geo->m_vertices));
		}
		basicModelTextured.LoadGeometry(engine::tools::getAssetPath() + "models/sphere.obj", &vertexLayout, 0.1f, 1);
		for (auto geo : basicModelTextured.m_geometries)
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
		colorMap = vulkanDevice->GetTexture(engine::tools::getAssetPath() + "textures/pbr/rusted_iron/albedo.png", VK_FORMAT_R8G8B8A8_UNORM, queue);
		roughnessMap = vulkanDevice->GetTexture(engine::tools::getAssetPath() + "textures/pbr/rusted_iron/roughness.png", VK_FORMAT_R8G8B8A8_UNORM, queue);
		metallicMap = vulkanDevice->GetTexture(engine::tools::getAssetPath() + "textures/pbr/rusted_iron/metallic.png", VK_FORMAT_R8G8B8A8_UNORM, queue);
		aoMap = vulkanDevice->GetTexture(engine::tools::getAssetPath() + "textures/pbr/rusted_iron/ao.png", VK_FORMAT_R8G8B8A8_UNORM, queue);
	}

	void SetupUniforms()
	{
		//uniforms
		uniform_manager.SetDevice(vulkanDevice->logicalDevice);
		uniform_manager.SetEngineDevice(vulkanDevice);
		sceneVertexUniformBuffer = uniform_manager.GetGlobalUniformBuffer({ scene::UNIFORM_PROJECTION ,scene::UNIFORM_VIEW ,scene::UNIFORM_LIGHT0_POSITION, scene::UNIFORM_CAMERA_POSITION });

		modelFragmentUniformBuffer = vulkanDevice->GetUniformBuffer(sizeof(modelUniformFS), true, queue);
		modelFragmentUniformBuffer->map();
		modelFragmentTexturedUniformBuffer = vulkanDevice->GetUniformBuffer(sizeof(modelUniformTexturedFS), true, queue);
		modelFragmentTexturedUniformBuffer->map();
		

		updateUniformBuffers();
	}

	//here a descriptor pool will be created for the entire app. Now it contains 1 sampler because this is what the ui overlay needs
	void setupDescriptorPool()
	{
		std::vector<VkDescriptorPoolSize> poolSizes = {
			VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 6},
			VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 8}
		};
		vulkanDevice->CreateDescriptorSetsPool(poolSizes, 4);
	}

	void SetupDescriptors()
	{
		//descriptors
		std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> modelbindings
		{
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT}
		};
		//descriptors
		std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> modelbindingstextured
		{
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
		};
		basicModel.SetDescriptorSetLayout(vulkanDevice->GetDescriptorSetLayout(modelbindings));
		basicModelTextured.SetDescriptorSetLayout(vulkanDevice->GetDescriptorSetLayout(modelbindingstextured));

		basicModel.AddDescriptor(vulkanDevice->GetDescriptorSet({ &sceneVertexUniformBuffer->m_descriptor, &modelFragmentUniformBuffer->m_descriptor }, 
			{  },
			basicModel._descriptorLayout->m_descriptorSetLayout, basicModel._descriptorLayout->m_setLayoutBindings));

		basicModelTextured.AddDescriptor(vulkanDevice->GetDescriptorSet({ &sceneVertexUniformBuffer->m_descriptor, &modelFragmentTexturedUniformBuffer->m_descriptor },
			{ &colorMap->m_descriptor, &roughnessMap->m_descriptor, &metallicMap->m_descriptor, &aoMap->m_descriptor },
			basicModelTextured._descriptorLayout->m_descriptorSetLayout, basicModelTextured._descriptorLayout->m_setLayoutBindings));
	}

	void setupPipelines()
	{

		basicModel.AddPipeline(vulkanDevice->GetPipeline(basicModel._descriptorLayout->m_descriptorSetLayout, vertexLayout.m_vertexInputBindings, vertexLayout.m_vertexInputAttributes,
			engine::tools::getAssetPath() + "shaders/pbr/pbr.vert.spv", engine::tools::getAssetPath() + "shaders/pbr/pbr.frag.spv", mainRenderPass->GetRenderPass(), pipelineCache));

		basicModelTextured.AddPipeline(vulkanDevice->GetPipeline(basicModelTextured._descriptorLayout->m_descriptorSetLayout, vertexLayout.m_vertexInputBindings, vertexLayout.m_vertexInputAttributes,
			engine::tools::getAssetPath() + "shaders/pbr/pbr.vert.spv", engine::tools::getAssetPath() + "shaders/pbr/pbrtextured.frag.spv", mainRenderPass->GetRenderPass(), pipelineCache));
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
		VkCommandBufferBeginInfo cmdBufInfo{};
		cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		for (int32_t i = 0; i < drawCmdBuffers.size(); ++i)
		{
			VK_CHECK_RESULT(vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufInfo));

			mainRenderPass->Begin(drawCmdBuffers[i], i);

			//draw here
			textured ? basicModelTextured.Draw(drawCmdBuffers[i]) : basicModel.Draw(drawCmdBuffers[i]);

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
		glm::vec3 cucu = -camera.position;
		uniform_manager.UpdateGlobalParams(scene::UNIFORM_CAMERA_POSITION, &cucu, 0, sizeof(camera.position));

		uniform_manager.Update();

		modelUniformFS.cameraPosition = glm::vec4(cucu, 1.0f);
		modelUniformFS.lightPosition = light_pos;
		modelUniformTexturedFS.cameraPosition = glm::vec4(cucu, 1.0f);
		modelUniformTexturedFS.lightPosition = light_pos;

		modelFragmentUniformBuffer->copyTo(&modelUniformFS, sizeof(modelUniformFS));
		modelFragmentTexturedUniformBuffer->copyTo(&modelUniformTexturedFS, sizeof(modelUniformTexturedFS));

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

	}

	virtual void viewChanged()
	{
		updateUniformBuffers();
	}

	virtual void OnUpdateUIOverlay(engine::scene::UIOverlay *overlay)
	{
		if (overlay->header("Settings")) {
			if (ImGui::SliderFloat("Roughness", &modelUniformFS.roughness, 0.05f, 1.0f))
			{
				updateUniformBuffers();
			}
			if (ImGui::SliderFloat("Metallic", &modelUniformFS.metallic, 0.1f, 1.0f))
			{
				updateUniformBuffers();
			}
			if (overlay->checkBox("Textured", &textured)) {
				buildCommandBuffers();
			}
		}
	}

};

VULKAN_EXAMPLE_MAIN()