
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
#include "scene/DrawDebug.h"
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

	render::VertexLayout simpleVertexLayout = render::VertexLayout({
		render::VERTEX_COMPONENT_POSITION
		}, {});

	engine::scene::SimpleModel plane;

	render::VulkanTexture* envMap;
	render::VulkanTexture* BRDFLUTMap;
	render::VulkanTexture* irradianceMap;
	render::VulkanTexture* prefilterMap;
	
	
	struct {
		glm::vec3 albedo = glm::vec3(1.0,0.0,0.0);
		float roughness = 0.1f;
		float metallic = 0.9f;
		float ao = 1.0;
	} modelUniformFS;
	render::VulkanBuffer* modelFragmentUniformBuffer;

	render::VulkanBuffer* sceneVertexUniformBuffer;
	scene::UniformBuffersManager uniform_manager;

	struct {
		glm::mat4 model;
	} modelUniformVS;
	render::VulkanBuffer* modelSBVertexUniformBuffer;
	struct {
		glm::vec4 lights[4];
		float exposure = 4.5f;
		float gamma = 2.2f;
	} modelSBUniformFS;
	render::VulkanBuffer* modelSBFragmentUniformBuffer;

	scene::SimpleModel skybox;

	glm::vec4 light_pos = glm::vec4(5.0f, -5.0f, 0.0f, 1.0f);

	render::VulkanRenderPass* offscreenRenderPass;

	//scene::DrawDebugTexture dbgtex;

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
		/*camera.setRotation(glm::vec3(0.0f, 0.0f, 0.0f));
		camera.setTranslation(glm::vec3(0.0f, 5.0f, 0.0f));*/
		camera.setRotation({ -3.75f, 180.0f, 0.0f });
		camera.setPosition({ 0.55f, 0.85f, 12.0f });
	}

	~VulkanExample()
	{
		// Clean up used Vulkan resources 
		// Note : Inherited destructor cleans up resources stored in base class
		
	}

	void setupGeometry()
	{
		//Geometry
		plane.LoadGeometry(engine::tools::getAssetPath() + "models/geosphere.obj", &vertexLayout, 0.1f, 1);
		skybox.LoadGeometry(engine::tools::getAssetPath() + "models/cube.obj", &simpleVertexLayout, 20.0f, 1);
		for (auto geo : plane.m_geometries)
		{
			geo->SetIndexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geo->m_indexCount * sizeof(uint32_t), geo->m_indices));
			geo->SetVertexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geo->m_verticesSize * sizeof(float), geo->m_vertices));
		}
		for (auto geo : skybox.m_geometries)
		{
			geo->SetIndexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geo->m_indexCount * sizeof(uint32_t), geo->m_indices));
			geo->SetVertexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geo->m_verticesSize * sizeof(float), geo->m_vertices));
		}
		
	}

	void SetupTextures()
	{
		envMap = vulkanDevice->GetTextureCubeMap(engine::tools::getAssetPath() + "textures/hdr/pisa_cube.ktx", VK_FORMAT_R16G16B16A16_SFLOAT, queue);
	}

	void SetupUniforms()
	{
		//uniforms
		uniform_manager.SetDevice(vulkanDevice->logicalDevice);
		uniform_manager.SetEngineDevice(vulkanDevice);
		sceneVertexUniformBuffer = uniform_manager.GetGlobalUniformBuffer({ scene::UNIFORM_PROJECTION ,scene::UNIFORM_VIEW ,scene::UNIFORM_LIGHT0_POSITION, scene::UNIFORM_CAMERA_POSITION });

		modelFragmentUniformBuffer = vulkanDevice->GetUniformBuffer(sizeof(modelUniformFS), true, queue);
		modelFragmentUniformBuffer->map();
		
		modelSBVertexUniformBuffer = vulkanDevice->GetUniformBuffer(sizeof(modelUniformVS), true, queue);
		modelSBVertexUniformBuffer->map();

		modelSBFragmentUniformBuffer = vulkanDevice->GetUniformBuffer(sizeof(modelSBUniformFS), true, queue);
		modelSBFragmentUniformBuffer->map();

		updateUniformBuffers();
	}

	void updateUniformBuffers()
	{
		uniform_manager.UpdateGlobalParams(scene::UNIFORM_PROJECTION, &camera.matrices.perspective, 0, sizeof(camera.matrices.perspective));
		uniform_manager.UpdateGlobalParams(scene::UNIFORM_VIEW, &camera.matrices.view, 0, sizeof(camera.matrices.view));
		uniform_manager.UpdateGlobalParams(scene::UNIFORM_LIGHT0_POSITION, &light_pos, 0, sizeof(light_pos));
		glm::vec3 cucu = -camera.position;
		//cucu.y = -cucu.y;
		uniform_manager.UpdateGlobalParams(scene::UNIFORM_CAMERA_POSITION, &cucu, 0, sizeof(camera.position));

		uniform_manager.Update();

		modelFragmentUniformBuffer->copyTo(&modelUniformFS, sizeof(modelUniformFS));

		/*modelUniformVS.model = 
			glm::mat4(1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f);*/

		modelUniformVS.model = glm::mat4(glm::mat3(camera.matrices.view));
		modelSBVertexUniformBuffer->copyTo(&modelUniformVS, sizeof(modelUniformVS));

		modelSBFragmentUniformBuffer->copyTo(&modelSBUniformFS, sizeof(modelSBUniformFS));

		//dbgtex.UpdateUniformBuffers(camera.matrices.perspective, camera.matrices.view, 1.0);
	}

	//here a descriptor pool will be created for the entire app. Now it contains 1 sampler because this is what the ui overlay needs
	void setupDescriptorPool()
	{
		std::vector<VkDescriptorPoolSize> poolSizes = {
			VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 6},
			VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 8}
		};
		vulkanDevice->CreateDescriptorSetsPool(poolSizes, 6);
	}

	void SetupDescriptors()
	{
		//descriptors
		std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> modelbindings
		{
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
		};
		plane.SetDescriptorSetLayout(vulkanDevice->GetDescriptorSetLayout(modelbindings));

		plane.AddDescriptor(vulkanDevice->GetDescriptorSet({ &sceneVertexUniformBuffer->m_descriptor, &modelFragmentUniformBuffer->m_descriptor }, 
			{ &BRDFLUTMap->m_descriptor, &irradianceMap->m_descriptor, &prefilterMap->m_descriptor },
			plane._descriptorLayout->m_descriptorSetLayout, plane._descriptorLayout->m_setLayoutBindings));

		std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> skyboxbindings
		{
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
		};
		skybox.SetDescriptorSetLayout(vulkanDevice->GetDescriptorSetLayout(skyboxbindings));

		skybox.AddDescriptor(vulkanDevice->GetDescriptorSet({ &sceneVertexUniformBuffer->m_descriptor, &modelSBVertexUniformBuffer->m_descriptor, &modelSBFragmentUniformBuffer->m_descriptor },
			{ &envMap->m_descriptor },
			skybox._descriptorLayout->m_descriptorSetLayout, skybox._descriptorLayout->m_setLayoutBindings));
	}

	void setupPipelines()
	{
		plane.AddPipeline(vulkanDevice->GetPipeline(plane._descriptorLayout->m_descriptorSetLayout, vertexLayout.m_vertexInputBindings, vertexLayout.m_vertexInputAttributes,
			engine::tools::getAssetPath() + "shaders/pbr/pbribl.vert.spv", engine::tools::getAssetPath() + "shaders/pbr/pbribl.frag.spv", mainRenderPass->GetRenderPass(), pipelineCache));

		render::PipelineProperties props;
		props.cullMode = VK_CULL_MODE_FRONT_BIT;
		skybox.AddPipeline(vulkanDevice->GetPipeline(skybox._descriptorLayout->m_descriptorSetLayout, simpleVertexLayout.m_vertexInputBindings, simpleVertexLayout.m_vertexInputAttributes,
			engine::tools::getAssetPath() + "shaders/pbr/skybox.vert.spv", engine::tools::getAssetPath() + "shaders/pbr/skybox.frag.spv", mainRenderPass->GetRenderPass(), pipelineCache, props));
	}

	void generateBRDFLUTTexture()
	{
		const VkFormat format = VK_FORMAT_R16G16_SFLOAT;	// R16G16 is supported pretty much everywhere
		const int32_t dim = 512;

		BRDFLUTMap = vulkanDevice->GetRenderTarget(dim, dim, format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		offscreenRenderPass = vulkanDevice->GetRenderPass({ { BRDFLUTMap->m_format, BRDFLUTMap->m_descriptor.imageLayout } });
		render::VulkanFrameBuffer* fb = vulkanDevice->GetFrameBuffer(offscreenRenderPass->GetRenderPass(), dim, dim, { BRDFLUTMap->m_descriptor.imageView });
		offscreenRenderPass->AddFrameBuffer(fb);

		std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> modelbindings
		{
		};

		render::VulkanDescriptorSetLayout* layout = vulkanDevice->GetDescriptorSetLayout(modelbindings);
		/*render::VulkanDescriptorSet* set = vulkanDevice->GetDescriptorSet({  },
			{ &envMap->m_descriptor },
			layout->m_descriptorSetLayout, layout->m_setLayoutBindings);*/

		render::VulkanPipeline* pipeline = vulkanDevice->GetPipeline(layout->m_descriptorSetLayout, {}, {},
			engine::tools::getAssetPath() + "shaders/pbr/genbrdflut.vert.spv", engine::tools::getAssetPath() + "shaders/pbr/genbrdflut.frag.spv", offscreenRenderPass->GetRenderPass(), pipelineCache, false, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

		VkCommandBuffer cmdBuf = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
		offscreenRenderPass->Begin(cmdBuf,0);
		pipeline->Draw(cmdBuf);
		vkCmdDraw(cmdBuf, 3, 1, 0, 0);
		offscreenRenderPass->End(cmdBuf);
		vulkanDevice->flushCommandBuffer(cmdBuf, queue);
		vkQueueWaitIdle(queue);

		//dbgtex.Init(vulkanDevice, BRDFLUTMap, queue, mainRenderPass->GetRenderPass(), pipelineCache);
	}

	void generateIrradianceCube()
	{	
		uint32_t dim = 64;
		const VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT;
		irradianceMap = vulkanDevice->GetTextureCubeMap(dim, format, queue);

		render::VulkanTexture *tempTex = vulkanDevice->GetRenderTarget(dim, dim, format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		offscreenRenderPass = vulkanDevice->GetRenderPass({ { tempTex->m_format, tempTex->m_descriptor.imageLayout } });
		render::VulkanFrameBuffer* fb = vulkanDevice->GetFrameBuffer(offscreenRenderPass->GetRenderPass(), dim, dim, { tempTex->m_descriptor.imageView });
		offscreenRenderPass->AddFrameBuffer(fb);

		std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> modelbindings
		{
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
		};
		scene::RenderObject obj;
		obj.SetDescriptorSetLayout(vulkanDevice->GetDescriptorSetLayout(modelbindings));
		obj.AddDescriptor(vulkanDevice->GetDescriptorSet({  },
			{ &envMap->m_descriptor },
			obj._descriptorLayout->m_descriptorSetLayout, obj._descriptorLayout->m_setLayoutBindings));

		struct PushBlock {
			glm::mat4 mvp;
			// Sampling deltas
			float deltaPhi = (2.0f * float(M_PI)) / 180.0f;
			float deltaTheta = (0.5f * float(M_PI)) / 64.0f;
		} pushBlock;

		obj.SetVertexLayout(&simpleVertexLayout);

		render::PipelineProperties props;
		props.cullMode = VK_CULL_MODE_FRONT_BIT;
		props.vertexConstantBlockSize = sizeof(pushBlock);
		obj.AddPipeline(vulkanDevice->GetPipeline(obj._descriptorLayout->m_descriptorSetLayout, obj._vertexLayout->m_vertexInputBindings, obj._vertexLayout->m_vertexInputAttributes,
			engine::tools::getAssetPath() + "shaders/pbr/filtercube.vert.spv", engine::tools::getAssetPath() + "shaders/pbr/irradiancecube.frag.spv", offscreenRenderPass->GetRenderPass(), pipelineCache, props));

		std::vector<glm::mat4> matrices = {
			// POSITIVE_X
			glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
			// NEGATIVE_X
			glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
			// POSITIVE_Y
			glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
			// NEGATIVE_Y
			glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
			// POSITIVE_Z
			glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
			// NEGATIVE_Z
			glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
		};

		VkCommandBuffer cmdBuf = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.baseMipLevel = 0;
		subresourceRange.levelCount = irradianceMap->m_mipLevelsCount;
		subresourceRange.layerCount = 6;
		// Change image layout for all cubemap faces to transfer destination
		engine::tools::setImageLayout(
			cmdBuf,
			irradianceMap->m_vkImage,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			subresourceRange);

		VkViewport viewport = { 0, 0, (float)dim, (float)dim, 0.0f, 1.0f };
		for (uint32_t m = 0; m < irradianceMap->m_mipLevelsCount; m++) {
			for (uint32_t f = 0; f < 6; f++) {
				viewport.width = static_cast<float>(dim * std::pow(0.5f, m));
				viewport.height = static_cast<float>(dim * std::pow(0.5f, m));
				offscreenRenderPass->Begin(cmdBuf,0);
				vkCmdSetViewport(cmdBuf, 0, 1, &viewport);

				pushBlock.mvp = glm::perspective((float)(M_PI / 2.0), 1.0f, 0.1f, 512.0f) * matrices[f];

				vkCmdPushConstants(cmdBuf, obj._pipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushBlock), &pushBlock);
				obj._pipeline->Draw(cmdBuf);
				obj.m_descriptorSets[0]->Draw(cmdBuf, obj._pipeline->getPipelineLayout(),0);

				uint32_t vip = obj._vertexLayout->GetVertexInputBinding(VK_VERTEX_INPUT_RATE_VERTEX);//TODO can we add this info for each geometry?
				uint32_t iip = obj._vertexLayout->GetVertexInputBinding(VK_VERTEX_INPUT_RATE_INSTANCE);
				skybox.m_geometries[0]->Draw(&cmdBuf, vip, iip);
				offscreenRenderPass->End(cmdBuf);

				engine::tools::setImageLayout(
					cmdBuf,
					tempTex->m_vkImage,
					VK_IMAGE_ASPECT_COLOR_BIT,
					VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

				// Copy region for transfer from framebuffer to cube face
				VkImageCopy copyRegion = {};

				copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				copyRegion.srcSubresource.baseArrayLayer = 0;
				copyRegion.srcSubresource.mipLevel = 0;
				copyRegion.srcSubresource.layerCount = 1;
				copyRegion.srcOffset = { 0, 0, 0 };

				copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				copyRegion.dstSubresource.baseArrayLayer = f;
				copyRegion.dstSubresource.mipLevel = m;
				copyRegion.dstSubresource.layerCount = 1;
				copyRegion.dstOffset = { 0, 0, 0 };

				copyRegion.extent.width = static_cast<uint32_t>(viewport.width);
				copyRegion.extent.height = static_cast<uint32_t>(viewport.height);
				copyRegion.extent.depth = 1;

				vkCmdCopyImage(
					cmdBuf,
					tempTex->m_vkImage,
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					irradianceMap->m_vkImage,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					1,
					&copyRegion);

				// Transform framebuffer color attachment back
				engine::tools::setImageLayout(
					cmdBuf,
					tempTex->m_vkImage,
					VK_IMAGE_ASPECT_COLOR_BIT,
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

			}
		}

		engine::tools::setImageLayout(
			cmdBuf,
			irradianceMap->m_vkImage,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			subresourceRange);

		vulkanDevice->flushCommandBuffer(cmdBuf, queue);
	}

	void generatePrefilteredCube()
	{
		uint32_t dim = 512;
		const VkFormat format = VK_FORMAT_R16G16B16A16_SFLOAT;
		prefilterMap = vulkanDevice->GetTextureCubeMap(dim, format, queue);
		render::VulkanTexture* tempTex = vulkanDevice->GetRenderTarget(dim, dim, format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		offscreenRenderPass = vulkanDevice->GetRenderPass({ { tempTex->m_format, tempTex->m_descriptor.imageLayout } });
		render::VulkanFrameBuffer* fb = vulkanDevice->GetFrameBuffer(offscreenRenderPass->GetRenderPass(), dim, dim, { tempTex->m_descriptor.imageView });
		offscreenRenderPass->AddFrameBuffer(fb);
		std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> modelbindings
		{
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
		};
		scene::RenderObject obj;
		obj.SetDescriptorSetLayout(vulkanDevice->GetDescriptorSetLayout(modelbindings));
		obj.AddDescriptor(vulkanDevice->GetDescriptorSet({  },
			{ &envMap->m_descriptor },
			obj._descriptorLayout->m_descriptorSetLayout, obj._descriptorLayout->m_setLayoutBindings));

		struct PushBlock {
			glm::mat4 mvp;
			float roughness;
			uint32_t numSamples = 32u;
		} pushBlock;

		obj.SetVertexLayout(&simpleVertexLayout);

		render::PipelineProperties props;
		props.cullMode = VK_CULL_MODE_FRONT_BIT;
		props.vertexConstantBlockSize = sizeof(pushBlock);
		obj.AddPipeline(vulkanDevice->GetPipeline(obj._descriptorLayout->m_descriptorSetLayout, obj._vertexLayout->m_vertexInputBindings, obj._vertexLayout->m_vertexInputAttributes,
			engine::tools::getAssetPath() + "shaders/pbr/filtercube.vert.spv", engine::tools::getAssetPath() + "shaders/pbr/prefilterenvmap.frag.spv", offscreenRenderPass->GetRenderPass(), pipelineCache, props));

		std::vector<glm::mat4> matrices = {
			// POSITIVE_X
			glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
			// NEGATIVE_X
			glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
			// POSITIVE_Y
			glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
			// NEGATIVE_Y
			glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
			// POSITIVE_Z
			glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
			// NEGATIVE_Z
			glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
		};

		VkCommandBuffer cmdBuf = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

		VkViewport viewport = { 0, 0, (float)dim, (float)dim, 0.0f, 1.0f };

		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.baseMipLevel = 0;
		subresourceRange.levelCount = prefilterMap->m_mipLevelsCount;
		subresourceRange.layerCount = 6;
		// Change image layout for all cubemap faces to transfer destination
		engine::tools::setImageLayout(
			cmdBuf,
			prefilterMap->m_vkImage,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			subresourceRange);

		for (uint32_t m = 0; m < prefilterMap->m_mipLevelsCount; m++) {
			pushBlock.roughness = (float)m / (float)(prefilterMap->m_mipLevelsCount - 1);
			for (uint32_t f = 0; f < 6; f++) {
				viewport.width = static_cast<float>(dim * std::pow(0.5f, m));
				viewport.height = static_cast<float>(dim * std::pow(0.5f, m));

				offscreenRenderPass->Begin(cmdBuf, 0);
				vkCmdSetViewport(cmdBuf, 0, 1, &viewport);

				pushBlock.mvp = glm::perspective((float)(M_PI / 2.0), 1.0f, 0.1f, 512.0f) * matrices[f];

				vkCmdPushConstants(cmdBuf, obj._pipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushBlock), &pushBlock);
				obj._pipeline->Draw(cmdBuf);
				obj.m_descriptorSets[0]->Draw(cmdBuf, obj._pipeline->getPipelineLayout(), 0);

				uint32_t vip = obj._vertexLayout->GetVertexInputBinding(VK_VERTEX_INPUT_RATE_VERTEX);//TODO can we add this info for each geometry?
				uint32_t iip = obj._vertexLayout->GetVertexInputBinding(VK_VERTEX_INPUT_RATE_INSTANCE);
				skybox.m_geometries[0]->Draw(&cmdBuf, vip, iip);
				offscreenRenderPass->End(cmdBuf);

				engine::tools::setImageLayout(
					cmdBuf,
					tempTex->m_vkImage,
					VK_IMAGE_ASPECT_COLOR_BIT,
					VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

				// Copy region for transfer from framebuffer to cube face
				VkImageCopy copyRegion = {};

				copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				copyRegion.srcSubresource.baseArrayLayer = 0;
				copyRegion.srcSubresource.mipLevel = 0;
				copyRegion.srcSubresource.layerCount = 1;
				copyRegion.srcOffset = { 0, 0, 0 };

				copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				copyRegion.dstSubresource.baseArrayLayer = f;
				copyRegion.dstSubresource.mipLevel = m;
				copyRegion.dstSubresource.layerCount = 1;
				copyRegion.dstOffset = { 0, 0, 0 };

				copyRegion.extent.width = static_cast<uint32_t>(viewport.width);
				copyRegion.extent.height = static_cast<uint32_t>(viewport.height);
				copyRegion.extent.depth = 1;

				vkCmdCopyImage(
					cmdBuf,
					tempTex->m_vkImage,
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					prefilterMap->m_vkImage,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					1,
					&copyRegion);

				// Transform framebuffer color attachment back
				engine::tools::setImageLayout(
					cmdBuf,
					tempTex->m_vkImage,
					VK_IMAGE_ASPECT_COLOR_BIT,
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
			}
		}
		engine::tools::setImageLayout(
			cmdBuf,
			prefilterMap->m_vkImage,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			subresourceRange);

		vulkanDevice->flushCommandBuffer(cmdBuf, queue);
	}

	void init()
	{	
		setupGeometry();
		SetupTextures();
		SetupUniforms();
		setupDescriptorPool();
		generateBRDFLUTTexture();
		generateIrradianceCube();
		generatePrefilteredCube();
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
			skybox.Draw(drawCmdBuffers[i]);
			plane.Draw(drawCmdBuffers[i]);
			//dbgtex.Draw(drawCmdBuffers[i]);

			drawUI(drawCmdBuffers[i]);

			mainRenderPass->End(drawCmdBuffers[i]);

			VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
		}
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
		}
	}

};

VULKAN_EXAMPLE_MAIN()