
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
		render::VERTEX_COMPONENT_UV,
		render::VERTEX_COMPONENT_TANGENT,
		render::VERTEX_COMPONENT_BITANGENT
		}, {});

	engine::scene::SimpleModel plane;
	render::VulkanTexture* colorMap;
	render::VulkanTexture* normalMap;
	render::VulkanTexture* dispMap;
	render::VulkanTexture* glossMap;

	render::VulkanBuffer* sceneVertexUniformBuffer;
	scene::UniformBuffersManager uniform_manager;

	struct {
		glm::mat4 model;
	} modelUniform;
	render::VulkanBuffer* modelVertexUniformBuffer;

	glm::vec4 light_pos = glm::vec4(5.0f, -5.0f, 0.0f, 1.0f);

	float ModelAngle = glm::radians(0.0f);

	VulkanExample() : VulkanExampleBase(true)
	{
		zoom = -3.75f;
		rotationSpeed = 0.5f;
		rotation = glm::vec3(15.0f, 0.f, 0.0f);
		title = "Render Engine Empty Scene";
		settings.overlay = true;
		camera.movementSpeed = 1.0f;
		camera.SetPerspective(60.0f, (float)width / (float)height, 0.1f, 1024.0f);
		camera.SetRotation(glm::vec3(0.0f, 0.0f, 0.0f));
		camera.SetPosition(glm::vec3(0.0f, 0.0f, -2.0f));
	}

	~VulkanExample()
	{
		// Clean up used Vulkan resources 
		// Note : Inherited destructor cleans up resources stored in base class
		
	}

	void setupGeometry()
	{
		//Geometry
		/*plane.LoadGeometry(engine::tools::getAssetPath() + "models/plane.obj", &vertexLayout, 1.0f, 1);
		for (auto geo : plane.m_geometries)
		{
			geo->SetIndexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geo->m_indexCount * sizeof(uint32_t), geo->m_indices));
			geo->SetVertexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geo->m_verticesSize * sizeof(float), geo->m_vertices));
		}*/
		struct Vertex {
			float pos[3];
			float normal[3];
			float uv[2];

			float tangent[3];
			float binormal[3];
		};
		scene::Geometry* geo = new scene::Geometry;
		std::vector<Vertex> vertices =
		{
			{ {  1.0f,  1.0f, 0.0f },  {0.0f,0.0f,1.0f},{ 1.0f, 1.0f }, {1.0f, 0.0f, 0.0f}, {0.0f,1.0f,0.0f} },
			{ { -1.0f,  1.0f, 0.0f }, {0.0f,0.0f,1.0f}, { 0.0f, 1.0f }, {1.0f, 0.0f, 0.0f}, {0.0f,1.0f,0.0f} },
			{ { -1.0f, -1.0f, 0.0f }, {0.0f,0.0f,1.0f}, { 0.0f, 0.0f }, {1.0f, 0.0f, 0.0f}, {0.0f,1.0f,0.0f} },
			{ {  1.0f, -1.0f, 0.0f }, {0.0f,0.0f,1.0f}, { 1.0f, 0.0f }, {1.0f, 0.0f, 0.0f}, {0.0f,1.0f,0.0f} }
		};
		/*std::vector<Vertex> vertices =
		{
			{ {  1.0f,  0.0f, 1.0f },  {0.0f,-1.0f,0.0f},{ 1.0f, 1.0f }, {1.0f, 0.0f, 0.0f}, {0.0f,0.0f,1.0f} },
			{ { -1.0f,  0.0f, 1.0f }, {0.0f,-1.0f,0.0f}, { 0.0f, 1.0f }, {1.0f, 0.0f, 0.0f}, {0.0f,0.0f,1.0f} },
			{ { -1.0f, 0.0f, -1.0f }, {0.0f,-1.0f,0.0f}, { 0.0f, 0.0f }, {1.0f, 0.0f, 0.0f}, {0.0f,0.0f,1.0f} },
			{ {  1.0f, 0.0f, -1.0f }, {0.0f,-1.0f,0.0f}, { 1.0f, 0.0f }, {1.0f, 0.0f, 0.0f}, {0.0f,0.0f,1.0f} }
		};*/
		geo->m_verticesSize = vertices.size();

		// Setup indices
		std::vector<uint32_t> indices = { 0,1,2, 2,3,0 };
		geo->m_indexCount = static_cast<uint32_t>(indices.size());

		geo->SetIndexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geo->m_indexCount * sizeof(uint32_t), indices.data()));
		geo->SetVertexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, vertices.size() * sizeof(Vertex), vertices.data()));
		plane._vertexLayout = &vertexLayout;
		plane.m_geometries.push_back(geo);
	}

	void SetupTextures()
	{
		colorMap = vulkanDevice->GetTexture(engine::tools::getAssetPath() + "textures/StoneBricksBeige015/StoneBricksBeige015_COL_4K.jpg", VK_FORMAT_R8G8B8A8_UNORM, queue);
		normalMap = vulkanDevice->GetTexture(engine::tools::getAssetPath() + "textures/StoneBricksBeige015/StoneBricksBeige015_NRM_4K.jpg", VK_FORMAT_R8G8B8A8_UNORM, queue);
		dispMap = vulkanDevice->GetTexture(engine::tools::getAssetPath() + "textures/StoneBricksBeige015/StoneBricksBeige015_DISP_4K.jpg", VK_FORMAT_R8G8B8A8_UNORM, queue);
		glossMap = vulkanDevice->GetTexture(engine::tools::getAssetPath() + "textures/StoneBricksBeige015/StoneBricksBeige015_GLOSS_4K.jpg", VK_FORMAT_R8G8B8A8_UNORM, queue);	
	}

	void SetupUniforms()
	{
		//uniforms
		uniform_manager.SetDevice(vulkanDevice->logicalDevice);
		uniform_manager.SetEngineDevice(vulkanDevice);
		sceneVertexUniformBuffer = uniform_manager.GetGlobalUniformBuffer({ scene::UNIFORM_PROJECTION ,scene::UNIFORM_VIEW ,scene::UNIFORM_LIGHT0_POSITION, scene::UNIFORM_CAMERA_POSITION });

		modelVertexUniformBuffer = vulkanDevice->GetUniformBuffer(sizeof(modelUniform), true, queue);
		modelVertexUniformBuffer->Map();

		updateUniformBuffers();
	}

	//here a descriptor pool will be created for the entire app. Now it contains 1 sampler because this is what the ui overlay needs
	void setupDescriptorPool()
	{
		std::vector<VkDescriptorPoolSize> poolSizes = {
			VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2},
			VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5}
		};
		vulkanDevice->CreateDescriptorSetsPool(poolSizes, 2);
	}

	void SetupDescriptors()
	{
		//descriptors
		std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> modelbindings
		{
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
		};
		plane.SetDescriptorSetLayout(vulkanDevice->GetDescriptorSetLayout(modelbindings));

		plane.AddDescriptor(vulkanDevice->GetDescriptorSet({ &sceneVertexUniformBuffer->m_descriptor, &modelVertexUniformBuffer->m_descriptor }, {&colorMap->m_descriptor, &normalMap->m_descriptor, &dispMap->m_descriptor, &glossMap->m_descriptor, },
			plane._descriptorLayout->m_descriptorSetLayout, plane._descriptorLayout->m_setLayoutBindings));
	}

	void setupPipelines()
	{
		plane.AddPipeline(vulkanDevice->GetPipeline(plane._descriptorLayout->m_descriptorSetLayout, vertexLayout.m_vertexInputBindings, vertexLayout.m_vertexInputAttributes,
			engine::tools::getAssetPath() + "shaders/normalparallax/normalparallaxmap.vert.spv", engine::tools::getAssetPath() + "shaders/normalparallax/normalparallaxmap.frag.spv", mainRenderPass->GetRenderPass(), pipelineCache));
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
			plane.Draw(drawCmdBuffers[i]);

			drawUI(drawCmdBuffers[i]);

			mainRenderPass->End(drawCmdBuffers[i]);

			VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
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

		uniform_manager.Update();

		modelUniform.model = glm::rotate(glm::mat4(1.0f), ModelAngle, glm::vec3(1.0, 0.0, 0.0));
		modelVertexUniformBuffer->MemCopy(&modelUniform, sizeof(modelUniform));
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
		light_pos.x = cos(glm::radians(timer * 360.0f)) * 50.0f;
		light_pos.z = sin(glm::radians(timer * 360.0f)) * 50.0f;
		updateUniformBuffers();
	}

	virtual void viewChanged()
	{
		updateUniformBuffers();
	}

	virtual void OnUpdateUIOverlay(engine::scene::UIOverlay *overlay)
	{
		if (overlay->header("Settings")) {
			ImGui::SliderAngle("Model Rotation Angle", &ModelAngle, -90.0f, 90.0f);
		}
	}

};

VULKAN_EXAMPLE_MAIN()