
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

	render::VertexLayout* vertexLayout = nullptr;
		/*render::VertexLayout({
		render::VERTEX_COMPONENT_POSITION,
		render::VERTEX_COMPONENT_NORMAL,
		render::VERTEX_COMPONENT_UV
		}, {});*/

	render::DescriptorPool* descriptorPool;

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
		camera.SetPosition(glm::vec3(0.0f, 0.0f, -5.0f));
	}

	~VulkanExample()
	{
		// Clean up used Vulkan resources 
		// Note : Inherited destructor cleans up resources stored in base class
		
	}

	void setupGeometry()
	{
		vertexLayout = m_device->GetVertexLayout({
		render::VERTEX_COMPONENT_POSITION,
		render::VERTEX_COMPONENT_NORMAL,
		render::VERTEX_COMPONENT_UV
			}, {});

		//Geometry
		std::vector<render::MeshData*> bmd = basicModel.LoadGeometry(engine::tools::getAssetPath() + "models/sphere.obj", vertexLayout, 0.1f, 1);
		for (auto geo : bmd)
		{
			//geo->SetIndexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geo->m_indexCount * sizeof(uint32_t), geo->m_indices));
			//geo->SetVertexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geo->m_verticesSize * sizeof(float), geo->m_vertices));
			basicModel.AddGeometry(vulkanDevice->GetMesh(geo, vertexLayout, nullptr));
			delete geo;
		}
		std::vector<render::MeshData*> btmd = basicModelTextured.LoadGeometry(engine::tools::getAssetPath() + "models/sphere.obj", vertexLayout, 0.1f, 1);
		for (auto geo : btmd)
		{
			//geo->SetIndexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geo->m_indexCount * sizeof(uint32_t), geo->m_indices));
			//geo->SetVertexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geo->m_verticesSize * sizeof(float), geo->m_vertices));
			basicModelTextured.AddGeometry(vulkanDevice->GetMesh(geo, vertexLayout, nullptr));
			delete geo;
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
		/*colorMap = vulkanDevice->GetTexture(engine::tools::getAssetPath() + "textures/pbr/rusted_iron/albedo.png", VK_FORMAT_R8G8B8A8_UNORM, queue);
		roughnessMap = vulkanDevice->GetTexture(engine::tools::getAssetPath() + "textures/pbr/rusted_iron/roughness.png", VK_FORMAT_R8G8B8A8_UNORM, queue);
		metallicMap = vulkanDevice->GetTexture(engine::tools::getAssetPath() + "textures/pbr/rusted_iron/metallic.png", VK_FORMAT_R8G8B8A8_UNORM, queue);
		aoMap = vulkanDevice->GetTexture(engine::tools::getAssetPath() + "textures/pbr/rusted_iron/ao.png", VK_FORMAT_R8G8B8A8_UNORM, queue);*/
		render::Texture2DData data;
		data.LoadFromFile(engine::tools::getAssetPath() + "textures/pbr/rusted_iron/albedo.png", render::GfxFormat::R8G8B8A8_UNORM);
		colorMap = vulkanDevice->GetTexture(&data, queue);
		data.Destroy();

		data.LoadFromFile(engine::tools::getAssetPath() + "textures/pbr/rusted_iron/roughness.png", render::GfxFormat::R8G8B8A8_UNORM);
		roughnessMap = vulkanDevice->GetTexture(&data, queue);
		data.Destroy();

		data.LoadFromFile(engine::tools::getAssetPath() + "textures/pbr/rusted_iron/metallic.png", render::GfxFormat::R8G8B8A8_UNORM);
		metallicMap = vulkanDevice->GetTexture(&data, queue);
		data.Destroy();

		data.LoadFromFile(engine::tools::getAssetPath() + "textures/pbr/rusted_iron/ao.png", render::GfxFormat::R8G8B8A8_UNORM);
		aoMap = vulkanDevice->GetTexture(&data, queue);
		//data.Destroy();
	}

	void SetupUniforms()
	{
		//uniforms
		uniform_manager.SetDevice(vulkanDevice->logicalDevice);
		uniform_manager.SetEngineDevice(vulkanDevice);
		sceneVertexUniformBuffer = uniform_manager.GetGlobalUniformBuffer({ scene::UNIFORM_PROJECTION ,scene::UNIFORM_VIEW ,scene::UNIFORM_LIGHT0_POSITION, scene::UNIFORM_CAMERA_POSITION });

		modelFragmentUniformBuffer = vulkanDevice->GetUniformBuffer(sizeof(modelUniformFS), true, queue);
		modelFragmentUniformBuffer->Map();
		modelFragmentTexturedUniformBuffer = vulkanDevice->GetUniformBuffer(sizeof(modelUniformTexturedFS), true, queue);
		modelFragmentTexturedUniformBuffer->Map();
		

		updateUniformBuffers();
	}

	//here a descriptor pool will be created for the entire app. Now it contains 1 sampler because this is what the ui overlay needs
	void setupDescriptorPool()
	{
		/*std::vector<VkDescriptorPoolSize> poolSizes = {
			VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 6},
			VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 8}
		};
		descriptorPool = vulkanDevice->CreateDescriptorSetsPool(poolSizes, 4);*/
		descriptorPool = vulkanDevice->GetDescriptorPool({
			{ render::DescriptorType::UNIFORM_BUFFER, 6 },
			{render::DescriptorType::IMAGE_SAMPLER, 8 }
			}, 4);
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

		/*basicModel.AddDescriptor(vulkanDevice->GetDescriptorSet(descriptorPool, { &sceneVertexUniformBuffer->m_descriptor, &modelFragmentUniformBuffer->m_descriptor },
			{  },
			basicModel._descriptorLayout->m_descriptorSetLayout, basicModel._descriptorLayout->m_setLayoutBindings));*/
		basicModel.AddDescriptor(vulkanDevice->GetDescriptorSet(basicModel._descriptorLayout, descriptorPool, { sceneVertexUniformBuffer, modelFragmentUniformBuffer }, {  }));

		/*basicModelTextured.AddDescriptor(vulkanDevice->GetDescriptorSet(descriptorPool, { &sceneVertexUniformBuffer->m_descriptor, &modelFragmentTexturedUniformBuffer->m_descriptor },
			{ &colorMap->m_descriptor, &roughnessMap->m_descriptor, &metallicMap->m_descriptor, &aoMap->m_descriptor },
			basicModelTextured._descriptorLayout->m_descriptorSetLayout, basicModelTextured._descriptorLayout->m_setLayoutBindings));*/
		basicModelTextured.AddDescriptor(vulkanDevice->GetDescriptorSet(basicModelTextured._descriptorLayout, descriptorPool, { sceneVertexUniformBuffer, modelFragmentTexturedUniformBuffer },
			{ colorMap, roughnessMap, metallicMap, aoMap }));
	}

	void setupPipelines()
	{
		render::PipelineProperties props;
		/*basicModel.AddPipeline(vulkanDevice->GetPipeline(basicModel._descriptorLayout->m_descriptorSetLayout, vertexLayout.m_vertexInputBindings, vertexLayout.m_vertexInputAttributes,
			engine::tools::getAssetPath() + "shaders/pbr/pbr.vert.spv", engine::tools::getAssetPath() + "shaders/pbr/pbr.frag.spv", mainRenderPass->GetRenderPass(), pipelineCache, props));*/
		basicModel.AddPipeline(vulkanDevice->GetPipeline(
			engine::tools::getAssetPath() + "shaders/pbr/pbr.vert.spv","", engine::tools::getAssetPath() + "shaders/pbr/pbr.frag.spv","",
			vertexLayout, basicModel._descriptorLayout, props, mainRenderPass));

		/*basicModelTextured.AddPipeline(vulkanDevice->GetPipeline(basicModelTextured._descriptorLayout->m_descriptorSetLayout, vertexLayout.m_vertexInputBindings, vertexLayout.m_vertexInputAttributes,
			engine::tools::getAssetPath() + "shaders/pbr/pbr.vert.spv", engine::tools::getAssetPath() + "shaders/pbr/pbrtextured.frag.spv", mainRenderPass->GetRenderPass(), pipelineCache, props));
	*/
		basicModelTextured.AddPipeline(vulkanDevice->GetPipeline(
			engine::tools::getAssetPath() + "shaders/pbr/pbr.vert.spv","", engine::tools::getAssetPath() + "shaders/pbr/pbrtextured.frag.spv","",
			vertexLayout, basicModelTextured._descriptorLayout, props, mainRenderPass));

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

		for (int32_t i = 0; i < m_drawCommandBuffers.size(); ++i)
		{
			//VK_CHECK_RESULT(vkBeginCommandBuffer(drawCommandBuffers[i], &cmdBufInfo));
			m_drawCommandBuffers[i]->Begin();

			mainRenderPass->Begin(m_drawCommandBuffers[i], i);

			//draw here
			textured ? basicModelTextured.Draw(m_drawCommandBuffers[i]) : basicModel.Draw(m_drawCommandBuffers[i]);

			DrawUI(m_drawCommandBuffers[i]);

			mainRenderPass->End(m_drawCommandBuffers[i]);

			//VK_CHECK_RESULT(vkEndCommandBuffer(drawCommandBuffers[i]));
			m_drawCommandBuffers[i]->End();
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
		uniform_manager.UpdateGlobalParams(scene::UNIFORM_CAMERA_POSITION, &cucu, 0, sizeof(camera.GetPosition()));

		uniform_manager.Update(queue);

		modelUniformFS.cameraPosition = glm::vec4(cucu, 1.0f);
		modelUniformFS.lightPosition = light_pos;
		modelUniformTexturedFS.cameraPosition = glm::vec4(cucu, 1.0f);
		modelUniformTexturedFS.lightPosition = light_pos;

		modelFragmentUniformBuffer->MemCopy(&modelUniformFS, sizeof(modelUniformFS));
		modelFragmentTexturedUniformBuffer->MemCopy(&modelUniformTexturedFS, sizeof(modelUniformTexturedFS));

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
			if (ImGui::SliderFloat("Roughness", &modelUniformFS.roughness, 0.05f, 1.0f))
			{
				updateUniformBuffers();
			}
			if (ImGui::SliderFloat("Metallic", &modelUniformFS.metallic, 0.1f, 1.0f))
			{
				updateUniformBuffers();
			}
			if (overlay->checkBox("Textured", &textured)) {
				BuildCommandBuffers();
			}
		}
	}

};

VULKAN_EXAMPLE_MAIN()