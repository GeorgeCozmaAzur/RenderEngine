
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
#include "scene/DeferredLights.h"

using namespace engine;

#define LIGHTS_NO 500

class VulkanExample : public VulkanApplication
{
public:

	// Vertex layout for the models
	render::VertexLayout* vertexLayout = nullptr;
		/*render::VulkanVertexLayout({
		render::VERTEX_COMPONENT_POSITION,
		render::VERTEX_COMPONENT_UV,
		render::VERTEX_COMPONENT_COLOR,
		render::VERTEX_COMPONENT_NORMAL
		}, {});*/

	render::VertexLayout* vertexLayoutInstanced = nullptr;
		/*render::VulkanVertexLayout({
		render::VERTEX_COMPONENT_POSITION,
		render::VERTEX_COMPONENT_UV,
		render::VERTEX_COMPONENT_COLOR,
		render::VERTEX_COMPONENT_NORMAL
		},
		{ render::VERTEX_COMPONENT_POSITION });*/

	render::DescriptorPool* descriptorPool;

	struct {
		scene::SimpleModel example;
		scene::SimpleModel quad;
		scene::SimpleModel plane;
	} models;
	scene::DeferredLights deferredLights;

	struct UBO {
		glm::mat4 projection;
		glm::mat4 view;
		glm::mat4 model;
	} uboShared;

	struct UBOLights {
		glm::mat4 projection;
		glm::mat4 view;
	} uboSharedLights;

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
		render::VulkanBuffer* vsModelLights;
		render::VulkanBuffer* fsdeferred;
	} uniformBuffers;

	render::VulkanTexture* colorMap;
	render::VulkanTexture* colorMap2;
	render::VulkanTexture* colorTex;
	render::VulkanTexture* depthTex;
	render::VulkanTexture* scenecolor;
	render::VulkanTexture* scenepositions;
	render::VulkanTexture* scenenormals;
	render::VulkanTexture* sceneLightscolor;
	render::VulkanTexture* scenedepth;

	struct {
		render::VulkanDescriptorSetLayout* model;
		render::VulkanDescriptorSetLayout* deferred;
		render::VulkanDescriptorSetLayout* simpletexture;
	} layouts;

	struct {
		render::Pipeline* plane;
		render::Pipeline* model;
		render::Pipeline* deferred;
		render::Pipeline* simpletexture;
	} pipelines;

	struct {
		render::DescriptorSet* plane;
		render::DescriptorSet* model;
		render::DescriptorSet* deferred;
		render::DescriptorSet* simpletexture;
	} descriptorSets;

	glm::vec3 meshPos = glm::vec3(0.0f, -1.5f, 0.0f);
	glm::vec3 meshRot = glm::vec3(0.0f);

	render::VulkanRenderPass* scenepass = nullptr;

	glm::vec3 light_positions[LIGHTS_NO];
	glm::vec3 light_colors[LIGHTS_NO];
	std::vector<glm::vec3> models_positions;

	VulkanExample() : VulkanApplication(true)
	{
		zoom = -60.75f;
		rotationSpeed = 0.5f;
		rotation = glm::vec3(0.0f, 0.f, 0.0f);
		title = "Deferred lighting example";
		settings.overlay = true;

		camera.movementSpeed = 20.5f;
		camera.SetPerspective(60.0f, (float)width / (float)height, 0.1f, 1024.0f);
		camera.SetRotation(glm::vec3(0.0f, 0.0f, 0.0f));
		camera.SetPosition(glm::vec3(0.0f, 100.0f,-5.0f));
		rotation.x = -20.0;
		rotation.y = -10.0;
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
		vertexLayout = m_device->GetVertexLayout(
			{
				render::VERTEX_COMPONENT_POSITION,
				render::VERTEX_COMPONENT_UV,
				render::VERTEX_COMPONENT_COLOR,
				render::VERTEX_COMPONENT_NORMAL	
			}, {});
		vertexLayoutInstanced = m_device->GetVertexLayout(
			{
				render::VERTEX_COMPONENT_POSITION,
				render::VERTEX_COMPONENT_UV,
				render::VERTEX_COMPONENT_COLOR,
				render::VERTEX_COMPONENT_NORMAL
			}, 
			{ render::VERTEX_COMPONENT_POSITION });

		scenecolor = vulkanDevice->GetRenderTarget(width, height, FB_COLOR_FORMAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
		scenepositions = vulkanDevice->GetRenderTarget(width, height, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		scenenormals = vulkanDevice->GetRenderTarget(width, height, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		sceneLightscolor = vulkanDevice->GetRenderTarget(width, height, FB_COLOR_FORMAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		scenedepth = vulkanDevice->GetDepthRenderTarget(width, height, false);

		scenepass = vulkanDevice->GetRenderPass({ scenecolor ,scenepositions, scenenormals, sceneLightscolor, scenedepth }, { render::RenderSubpass({}, {0,1,2,4}), render::RenderSubpass({1,2}, {3}) } );
		render::VulkanFrameBuffer* fb = vulkanDevice->GetFrameBuffer(scenepass->GetRenderPass(), width, height, 
			{ scenecolor->m_descriptor.imageView, scenepositions->m_descriptor.imageView, scenenormals->m_descriptor.imageView, sceneLightscolor->m_descriptor.imageView, scenedepth->m_descriptor.imageView },
			{ { 0.95f, 0.95f, 0.95f, 1.0f } });
		scenepass->AddFrameBuffer(fb);
		scenepass->SetClearColor({ 0.0f, 0.0f, 0.0f, 0.0f }, 1);
		scenepass->SetClearColor({ 0.0f, 0.0f, 0.0f, 0.0f }, 2);
		scenepass->SetClearColor({ 0.0f, 0.0f, 0.0f, 1.0f }, 3);

		models.plane.LoadGeometry(engine::tools::getAssetPath() + "models/plane.obj", vertexLayout, 10.0f, 1, glm::vec3(0.0,1.5,0.0));
		//models.example.LoadGeometry(engine::tools::getAssetPath() + "models/chinesedragon.dae", &vertexLayoutInstanced, 0.3f, LIGHTS_NO, glm::vec3(0.0, 0.0, 0.0));
		models.example.LoadGeometry(engine::tools::getAssetPath() + "models/armor/armor.dae", vertexLayoutInstanced, 0.3f, LIGHTS_NO, glm::vec3(0.0, 0.0, 0.0), glm::vec3(1.0, 1.0, 1.0), glm::vec2(1.0,-1.0));
		//models.example.LoadGeometry(engine::tools::getAssetPath() + "models/medieval/Medieval_House.obj", &vertexLayoutInstanced, 0.01f, MODELS_NO, glm::vec3(0.0, 2.0, 0.0));

		render::Texture2DData data;
		if (vulkanDevice->m_enabledFeatures.textureCompressionBC) {
			data.LoadFromFile(engine::tools::getAssetPath() + "textures/darkmetal_bc3_unorm.ktx", render::GfxFormat::BC3_UNORM_BLOCK);
			colorMap = vulkanDevice->GetTexture(&data, queue);
			data.Destroy();
			data.LoadFromFile(engine::tools::getAssetPath() + "models/armor/color_bc3_unorm.ktx", render::GfxFormat::BC3_UNORM_BLOCK);
			colorMap2 = vulkanDevice->GetTexture(&data, queue);
		}
		else if (vulkanDevice->m_enabledFeatures.textureCompressionASTC_LDR) {
			data.LoadFromFile(engine::tools::getAssetPath() + "textures/darkmetal_astc_8x8_unorm.ktx", render::GfxFormat::ASTC_8x8_UNORM_BLOCK);
			colorMap = vulkanDevice->GetTexture(&data, queue);
			data.Destroy();
			data.LoadFromFile(engine::tools::getAssetPath() + "textures/armor/color_astc_8x8_unorm.ktx", render::GfxFormat::ASTC_8x8_UNORM_BLOCK);
			colorMap2 = vulkanDevice->GetTexture(&data, queue);
		}
		else if (vulkanDevice->m_enabledFeatures.textureCompressionETC2) {
			data.LoadFromFile(engine::tools::getAssetPath() + "textures/darkmetal_etc2_unorm.ktx", render::GfxFormat::ETC2_R8G8B8_UNORM_BLOCK);
			colorMap = vulkanDevice->GetTexture(&data, queue);
			data.Destroy();
			data.LoadFromFile(engine::tools::getAssetPath() + "textures/armor/color_etc2_unorm.ktx", render::GfxFormat::ETC2_R8G8B8_UNORM_BLOCK);
			colorMap2 = vulkanDevice->GetTexture(&data, queue);
		}
		else {
			engine::tools::exitFatal("Device does not support any compressed texture format!", VK_ERROR_FEATURE_NOT_PRESENT);
		}
		//data.Destroy();
				
		models_positions.resize(LIGHTS_NO);

		for (int i = 0; i < LIGHTS_NO; i++)
		{
			float x = randomFloat() * 100 - 50;
			float z = randomFloat() * 100 - 50;
			light_positions[i].x = x;
			light_positions[i].z = z;
			light_positions[i].y = 0.0f;

			models_positions[i] = glm::vec3(x, 0, z);

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
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
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

		std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> simpletexurebinding
		{
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
		};
		layouts.simpletexture = vulkanDevice->GetDescriptorSetLayout(simpletexurebinding);
		
	}

	void setupDescriptorPool()
	{
		// Example uses three ubos and two image samplers
		/*std::vector<VkDescriptorPoolSize> poolSizes = {
			VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 8},
			VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10},
			VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 4}
		};
		descriptorPool = vulkanDevice->CreateDescriptorSetsPool(poolSizes, 6);*/
		descriptorPool = vulkanDevice->GetDescriptorPool(
			{ 
			{render::DescriptorType::UNIFORM_BUFFER, 8},
			{render::DescriptorType::IMAGE_SAMPLER, 10},
			{render::DescriptorType::INPUT_ATTACHMENT, 4}
			}, 6);
	}

	void prepareUniformBuffers()
	{
		uniformBuffers.vsModel = vulkanDevice->GetBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, sizeof(uboShared));

		uniformBuffers.vsModelLights = vulkanDevice->GetBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, sizeof(uboSharedLights));

		uniformBuffers.fsdeferred = vulkanDevice->GetBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, sizeof(uboDeferred));


		// Map persistent
		VK_CHECK_RESULT(uniformBuffers.vsModel->Map());
		VK_CHECK_RESULT(uniformBuffers.vsModelLights->Map());
		VK_CHECK_RESULT(uniformBuffers.fsdeferred->Map());
		updateUniformBuffers();

		deferredLights.Init(uniformBuffers.vsModelLights,vulkanDevice, descriptorPool, queue, scenepass, pipelineCache, LIGHTS_NO, scenepositions, scenenormals);
	}

	void updateUniformBuffers()
	{
		// Mesh
		uboShared.projection = uboSharedLights.projection = glm::perspective(glm::radians(60.0f), (float)width / (float)height, 0.1f, 256.0f);
		glm::mat4 viewMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, zoom));

		uboShared.view = viewMatrix * glm::translate(glm::mat4(1.0f), cameraPos);
		uboShared.view = glm::rotate(uboShared.view, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		uboShared.view = glm::rotate(uboShared.view, glm::radians(rotation.y + meshRot.y), glm::vec3(0.0f, 1.0f, 0.0f));
		uboShared.view = glm::rotate(uboShared.view, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
		uboSharedLights.view = uboShared.view;

		uboShared.model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
		uboShared.model = glm::translate(uboShared.model, meshPos);

		uniformBuffers.vsModel->MemCopy(&uboShared, sizeof(uboShared));
		uniformBuffers.vsModelLights->MemCopy(&uboSharedLights, sizeof(uboSharedLights));

		for (int i = 0; i < LIGHTS_NO; i++)
		{
			uboDeferred.lights[i].radius = 0.25f;
		}

		uniformBuffers.fsdeferred->MemCopy(&uboDeferred, sizeof(uboDeferred));
	}

	void setupDescriptors()
	{
		descriptorSets.plane = vulkanDevice->GetDescriptorSet(layouts.model, descriptorPool, { uniformBuffers.vsModel }, { colorMap });

		descriptorSets.model = vulkanDevice->GetDescriptorSet(layouts.model, descriptorPool, { uniformBuffers.vsModel }, { colorMap2 });
			//layouts.model->m_descriptorSetLayout, layouts.model->m_setLayoutBindings);

		models.plane.AddDescriptor(descriptorSets.plane);
		models.example.AddDescriptor(descriptorSets.model);	

		descriptorSets.deferred = vulkanDevice->GetDescriptorSet(layouts.deferred, descriptorPool, { uniformBuffers.fsdeferred },
			{ scenecolor , scenepositions, scenenormals });

		descriptorSets.simpletexture = vulkanDevice->GetDescriptorSet(layouts.simpletexture, descriptorPool, {}, { scenecolor , sceneLightscolor });
	}

	void setupPipelines()
	{
		/*VkPipelineColorBlendAttachmentState opaqueState{};
		opaqueState.blendEnable = VK_FALSE;
		opaqueState.colorWriteMask = 0xf;
		std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStates{ opaqueState ,opaqueState, opaqueState };*/
		render::BlendAttachmentState opaqueState{ false };
		std::vector <render::BlendAttachmentState> blendAttachmentStates{ opaqueState ,opaqueState, opaqueState };

		render::PipelineProperties props;
		props.attachmentCount = static_cast<uint32_t>(blendAttachmentStates.size());
		props.pAttachments = blendAttachmentStates.data();

		pipelines.plane = vulkanDevice->GetPipeline(//layouts.model->m_descriptorSetLayout, vertexLayout.m_vertexInputBindings, vertexLayout.m_vertexInputAttributes,
			engine::tools::getAssetPath() + "shaders/basicdeferred/basictexturedcolored.vert.spv","", engine::tools::getAssetPath() + "shaders/basicdeferred/basictexturedcolored.frag.spv","",
			vertexLayout, layouts.model, props, scenepass);
		pipelines.model = vulkanDevice->GetPipeline(//layouts.model->m_descriptorSetLayout, vertexLayoutInstanced.m_vertexInputBindings, vertexLayoutInstanced.m_vertexInputAttributes,
			engine::tools::getAssetPath() + "shaders/basicdeferred/basictexturedcoloredinstanced.vert.spv","", engine::tools::getAssetPath() + "shaders/basicdeferred/basictexturedcolored.frag.spv","",
			vertexLayoutInstanced, layouts.model, props, scenepass);
			//scenepass->GetRenderPass(), pipelineCache, props);
		models.example.AddPipeline(pipelines.model);
		models.plane.AddPipeline(pipelines.plane);

		render::PipelineProperties props1;
		props1.depthTestEnable = false;

		pipelines.deferred = vulkanDevice->GetPipeline(layouts.deferred->m_descriptorSetLayout, {}, {},
			engine::tools::getAssetPath() + "shaders/posteffects/screenquad.vert.spv", engine::tools::getAssetPath() + "shaders/basicdeferred/deferredlighting.frag.spv",
			mainRenderPass->GetRenderPass(), pipelineCache, props1);

		render::PipelineProperties props2;
		pipelines.simpletexture = vulkanDevice->GetPipeline(layouts.simpletexture->m_descriptorSetLayout, {}, {},
			engine::tools::getAssetPath() + "shaders/posteffects/screenquad.vert.spv", engine::tools::getAssetPath() + "shaders/posteffects/doubletexture.frag.spv",
			mainRenderPass->GetRenderPass(), pipelineCache, props2);
	}

	void BuildCommandBuffers()
	{
		VkCommandBufferBeginInfo cmdBufInfo{};
		cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		//cmdBufInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

		for (int32_t i = 0; i < m_drawCommandBuffers.size(); ++i)
		{
			//VK_CHECK_RESULT(vkBeginCommandBuffer(drawCommandBuffers[i], &cmdBufInfo));
			m_drawCommandBuffers[i]->Begin();

			scenepass->Begin(m_drawCommandBuffers[i], 0);
			models.example.Draw(m_drawCommandBuffers[i]);
			models.plane.Draw(m_drawCommandBuffers[i]);

			//vkCmdNextSubpass(drawCommandBuffers[i], VK_SUBPASS_CONTENTS_INLINE);
			scenepass->NextSubPass(m_drawCommandBuffers[i]);
			//vkCmdSetDepthTestEnable(drawCmdBuffers[i],true);
			deferredLights.Draw(m_drawCommandBuffers[i]);

			scenepass->End(m_drawCommandBuffers[i]);

			mainRenderPass->Begin(m_drawCommandBuffers[i], i);
			
			//draw here
			pipelines.simpletexture->Draw(m_drawCommandBuffers[i]);
			descriptorSets.simpletexture->Draw(m_drawCommandBuffers[i], pipelines.simpletexture);//pipelines.simpletexture->getPipelineLayout(), 0);
			//vkCmdDraw(drawCommandBuffers[i], 3, 1, 0, 0);
			DrawFullScreenQuad(m_drawCommandBuffers[i]);

			//deferredLights.Draw(drawCmdBuffers[i]);

			DrawUI(m_drawCommandBuffers[i]);

			mainRenderPass->End(m_drawCommandBuffers[i]);

			//VK_CHECK_RESULT(vkEndCommandBuffer(drawCommandBuffers[i]));
			m_drawCommandBuffers[i]->End();
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
		int j = 0;
		for (int i = 0; i < deferredLights.m_pointLights.size(); i++)
		{
			//deferredLights.m_pointLights[i] = glm::vec4();
			deferredLights.m_pointLights[i].x = light_positions[j].x + cos(glm::radians(timer * 360.0f)) * 1.0f;
			deferredLights.m_pointLights[i].z = light_positions[j].z + sin(glm::radians(timer * 360.0f)) * 1.0f;
			deferredLights.m_pointLights[i].y = -1.0f;
			deferredLights.m_pointLights[i].w = 7.0f;

			deferredLights.m_pointLights[++i].x = light_colors[j].x;
			deferredLights.m_pointLights[i].y = light_colors[j].y;
			deferredLights.m_pointLights[i].z = light_colors[j].z;
			j++;
		}

		deferredLights.Update();
		updateUniformBuffers();
	
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