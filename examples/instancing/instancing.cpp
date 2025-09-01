
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
#define LIGHTS_NO 18000
	/*render::VertexLayout vertexLayout = render::VertexLayout({
		render::VERTEX_COMPONENT_POSITION,
		render::VERTEX_COMPONENT_NORMAL,
		render::VERTEX_COMPONENT_UV
		
		}, {});*/


	render::VertexLayout vertexLayoutInstanced = render::VertexLayout({
		render::VERTEX_COMPONENT_POSITION,
		render::VERTEX_COMPONENT_NORMAL,
		render::VERTEX_COMPONENT_UV
		},
		{ render::VERTEX_COMPONENT_POSITION });

	engine::scene::SimpleModel plane;
	render::VulkanTexture* colorMap;

	render::VulkanBuffer* sceneVertexUniformBuffer;
	scene::UniformBuffersManager uniform_manager;
	VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

	glm::vec4 light_pos = glm::vec4(0.0f, -5.0f, 0.0f, 1.0f);

	std::vector<glm::vec3> models_positions;

	VulkanExample() : VulkanApplication(true)
	{
		zoom = -3.75f;
		rotationSpeed = 0.5f;
		rotation = glm::vec3(15.0f, 0.f, 0.0f);
		title = "Render Engine Empty Scene";
		settings.overlay = true;
		camera.movementSpeed = 20.5f;
		camera.SetPerspective(60.0f, (float)width / (float)height, 0.1f, 1024.0f);
		camera.SetRotation(glm::vec3(0.0f, 0.0f, 0.0f));
		camera.SetPosition(glm::vec3(0.0f, 5.0f, 0.0f));
	}

	~VulkanExample()
	{
		// Clean up used Vulkan resources 
		// Note : Inherited destructor cleans up resources stored in base class
		
	}

	float randomFloat() {
		return (float)rand() / ((float)RAND_MAX + 1);
	}
	void setupGeometry()
	{
		models_positions.resize(LIGHTS_NO);
		for (int i = 0; i < LIGHTS_NO; i++)
		{
			float x = randomFloat() * 300 - 200;
			float z = randomFloat() * 300 - 200;

			models_positions[i] = glm::vec3(x, 0, z);
		}

		//Geometry
		plane.LoadGeometry(engine::tools::getAssetPath() + "models/rock01.dae", &vertexLayoutInstanced, 1.0f, LIGHTS_NO);
		for (auto geo : plane.m_geometries)
		{
			geo->SetIndexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geo->m_indexCount * sizeof(uint32_t), geo->m_indices));
			geo->SetVertexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geo->m_verticesSize * sizeof(float), geo->m_vertices));
			geo->SetInstanceBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				queue, models_positions.size() * sizeof(glm::vec3), models_positions.data(),
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));
		}
	}

	void SetupTextures()
	{
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
	}

	void SetupUniforms()
	{
		//uniforms
		uniform_manager.SetDevice(vulkanDevice->logicalDevice);
		uniform_manager.SetEngineDevice(vulkanDevice);
		sceneVertexUniformBuffer = uniform_manager.GetGlobalUniformBuffer({ scene::UNIFORM_PROJECTION ,scene::UNIFORM_VIEW ,scene::UNIFORM_LIGHT0_POSITION, scene::UNIFORM_CAMERA_POSITION });
		updateUniformBuffers();
	}

	//here a descriptor pool will be created for the entire app. Now it contains 1 sampler because this is what the ui overlay needs
	void setupDescriptorPool()
	{
		std::vector<VkDescriptorPoolSize> poolSizes = {
			VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
			VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2}
		};
		descriptorPool = vulkanDevice->CreateDescriptorSetsPool(poolSizes, 2);
	}

	void SetupDescriptors()
	{
		//descriptors
		std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> modelbindings
		{
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
		};
		plane.SetDescriptorSetLayout(vulkanDevice->GetDescriptorSetLayout(modelbindings));

		plane.AddDescriptor(vulkanDevice->GetDescriptorSet(descriptorPool, { &sceneVertexUniformBuffer->m_descriptor }, {&colorMap->m_descriptor},
			plane._descriptorLayout->m_descriptorSetLayout, plane._descriptorLayout->m_setLayoutBindings));
	}

	void setupPipelines()
	{

		plane.AddPipeline(vulkanDevice->GetPipeline(plane._descriptorLayout->m_descriptorSetLayout, vertexLayoutInstanced.m_vertexInputBindings, vertexLayoutInstanced.m_vertexInputAttributes,
			engine::tools::getAssetPath() + "shaders/instancing/phong.vert.spv", engine::tools::getAssetPath() + "shaders/instancing/phongtextured.frag.spv", mainRenderPass->GetRenderPass(), pipelineCache));
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
		glm::vec3 cucu = -camera.GetPosition();
		uniform_manager.UpdateGlobalParams(scene::UNIFORM_CAMERA_POSITION, &cucu, 0, sizeof(cucu));

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

	}

	virtual void ViewChanged()
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