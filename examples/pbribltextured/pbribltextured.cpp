
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
#include "scene/DrawDebug.h"
#include "scene/UniformBuffersManager.h"
#include "render/vulkan/VulkanCommandBuffer.h"

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

	render::VertexLayout* simpleVertexLayout = nullptr;
		/*render::VertexLayout({
		render::VERTEX_COMPONENT_POSITION
		}, {});*/

	render::DescriptorPool* descriptorPool;

	engine::scene::SimpleModel plane;
	
	render::VulkanTexture* colorMap;
	render::VulkanTexture* normalMap;
	render::VulkanTexture* roughnessMap;
	render::VulkanTexture* metallicMap;
	render::VulkanTexture* aoMap;

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

	VulkanExample() : VulkanApplication(true)
	{
		zoom = -3.75f;
		rotationSpeed = 0.5f;
		rotation = glm::vec3(15.0f, 0.f, 0.0f);
		title = "Render Engine Empty Scene";
		settings.overlay = true;
		camera.movementSpeed = 5.0f;
		camera.SetPerspective(60.0f, (float)width / (float)height, 0.1f, 1024.0f);
		/*camera.setRotation(glm::vec3(0.0f, 0.0f, 0.0f));
		camera.setTranslation(glm::vec3(0.0f, 5.0f, 0.0f));*/
		camera.SetRotation({ -3.75f, 180.0f, 0.0f });
		camera.SetPosition({ 0.55f, 0.85f, 12.0f });
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
		render::VERTEX_COMPONENT_UV,
		render::VERTEX_COMPONENT_TANGENT,
		render::VERTEX_COMPONENT_BITANGENT
			}, {});
		simpleVertexLayout = m_device->GetVertexLayout({
		render::VERTEX_COMPONENT_POSITION
			}, {});
		//Geometry
		plane.LoadGeometry(engine::tools::getAssetPath() + "models/geosphere.obj", vertexLayout, 0.1f, 1);
		skybox.LoadGeometry(engine::tools::getAssetPath() + "models/cube.obj", simpleVertexLayout, 20.0f, 1);
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
		render::Texture2DData data;
		data.LoadFromFile(engine::tools::getAssetPath() + "textures/pbr/rusted_iron/albedo.png", render::GfxFormat::R8G8B8A8_UNORM);
		colorMap = vulkanDevice->GetTexture(&data, queue);
		data.Destroy();

		data.LoadFromFile(engine::tools::getAssetPath() + "textures/pbr/rusted_iron/normal.png", render::GfxFormat::R8G8B8A8_UNORM);
		normalMap = vulkanDevice->GetTexture(&data, queue);
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

		/*colorMap = vulkanDevice->GetTexture(engine::tools::getAssetPath() + "textures/pbr/rusted_iron/albedo.png", VK_FORMAT_R8G8B8A8_UNORM, queue);
		normalMap = vulkanDevice->GetTexture(engine::tools::getAssetPath() + "textures/pbr/rusted_iron/normal.png", VK_FORMAT_R8G8B8A8_UNORM, queue);
		roughnessMap = vulkanDevice->GetTexture(engine::tools::getAssetPath() + "textures/pbr/rusted_iron/roughness.png", VK_FORMAT_R8G8B8A8_UNORM, queue);
		metallicMap = vulkanDevice->GetTexture(engine::tools::getAssetPath() + "textures/pbr/rusted_iron/metallic.png", VK_FORMAT_R8G8B8A8_UNORM, queue);
		aoMap = vulkanDevice->GetTexture(engine::tools::getAssetPath() + "textures/pbr/rusted_iron/ao.png", VK_FORMAT_R8G8B8A8_UNORM, queue);*/

		render::TextureCubeMapData cdata;
		cdata.LoadFromFile(engine::tools::getAssetPath() + "textures/hdr/pisa_cube.ktx", render::GfxFormat::R16G16B16A16_SFLOAT);
		envMap = vulkanDevice->GetTexture(&cdata, queue);
		//cdata.Destroy();
		//envMap = vulkanDevice->GetTextureCubeMap(engine::tools::getAssetPath() + "textures/hdr/pisa_cube.ktx", VK_FORMAT_R16G16B16A16_SFLOAT, queue);
	}

	void SetupUniforms()
	{
		//uniforms
		uniform_manager.SetDevice(vulkanDevice->logicalDevice);
		uniform_manager.SetEngineDevice(vulkanDevice);
		sceneVertexUniformBuffer = uniform_manager.GetGlobalUniformBuffer({ scene::UNIFORM_PROJECTION ,scene::UNIFORM_VIEW ,scene::UNIFORM_LIGHT0_POSITION, scene::UNIFORM_CAMERA_POSITION });

		modelFragmentUniformBuffer = vulkanDevice->GetUniformBuffer(sizeof(modelUniformFS), true, queue);
		modelFragmentUniformBuffer->Map();
		
		modelSBVertexUniformBuffer = vulkanDevice->GetUniformBuffer(sizeof(modelUniformVS), true, queue);
		modelSBVertexUniformBuffer->Map();

		modelSBFragmentUniformBuffer = vulkanDevice->GetUniformBuffer(sizeof(modelSBUniformFS), true, queue);
		modelSBFragmentUniformBuffer->Map();

		updateUniformBuffers();
	}

	void updateUniformBuffers()
	{
		glm::mat4 perspectiveMatrix = camera.GetPerspectiveMatrix();
		glm::mat4 viewMatrix = camera.GetViewMatrix();

		uniform_manager.UpdateGlobalParams(scene::UNIFORM_PROJECTION, &perspectiveMatrix, 0, sizeof(perspectiveMatrix));
		uniform_manager.UpdateGlobalParams(scene::UNIFORM_VIEW, &viewMatrix, 0, sizeof(viewMatrix));
		uniform_manager.UpdateGlobalParams(scene::UNIFORM_LIGHT0_POSITION, &light_pos, 0, sizeof(light_pos));
		glm::vec3 cucu = -camera.GetPosition();
		//cucu.y = -cucu.y;
		uniform_manager.UpdateGlobalParams(scene::UNIFORM_CAMERA_POSITION, &cucu, 0, sizeof(camera.GetPosition()));

		uniform_manager.Update(queue);

		modelFragmentUniformBuffer->MemCopy(&modelUniformFS, sizeof(modelUniformFS));

		/*modelUniformVS.model = 
			glm::mat4(1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f);*/

		modelUniformVS.model = glm::mat4(glm::mat3(viewMatrix));
		modelSBVertexUniformBuffer->MemCopy(&modelUniformVS, sizeof(modelUniformVS));

		modelSBFragmentUniformBuffer->MemCopy(&modelSBUniformFS, sizeof(modelSBUniformFS));

		//dbgtex.UpdateUniformBuffers(perspectiveMatrix, viewMatrix, 1.0);
	}

	//here a descriptor pool will be created for the entire app. Now it contains 1 sampler because this is what the ui overlay needs
	void setupDescriptorPool()
	{
		/*std::vector<VkDescriptorPoolSize> poolSizes = {
			VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 6},
			VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 13}
		};
		descriptorPool = vulkanDevice->CreateDescriptorSetsPool(poolSizes, 6);*/
		descriptorPool = vulkanDevice->GetDescriptorPool({
			{ render::DescriptorType::UNIFORM_BUFFER, 6 },
			{render::DescriptorType::IMAGE_SAMPLER, 13}
			}, 6);
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
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
		};
		plane.SetDescriptorSetLayout(vulkanDevice->GetDescriptorSetLayout(modelbindings));

		/*plane.AddDescriptor(vulkanDevice->GetDescriptorSet(descriptorPool, { &sceneVertexUniformBuffer->m_descriptor, &modelFragmentUniformBuffer->m_descriptor },
			{ &BRDFLUTMap->m_descriptor, &irradianceMap->m_descriptor, &prefilterMap->m_descriptor, &colorMap->m_descriptor, &normalMap->m_descriptor, &roughnessMap->m_descriptor, &metallicMap->m_descriptor, &aoMap->m_descriptor },
			plane._descriptorLayout->m_descriptorSetLayout, plane._descriptorLayout->m_setLayoutBindings));*/
		plane.AddDescriptor(vulkanDevice->GetDescriptorSet(plane._descriptorLayout, descriptorPool, { sceneVertexUniformBuffer, modelFragmentUniformBuffer },
			{ BRDFLUTMap, irradianceMap, prefilterMap, colorMap, normalMap, roughnessMap, metallicMap, aoMap })); 

		std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> skyboxbindings
		{
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
		};
		skybox.SetDescriptorSetLayout(vulkanDevice->GetDescriptorSetLayout(skyboxbindings));

		/*skybox.AddDescriptor(vulkanDevice->GetDescriptorSet(descriptorPool, { &sceneVertexUniformBuffer->m_descriptor, &modelSBVertexUniformBuffer->m_descriptor, &modelSBFragmentUniformBuffer->m_descriptor },
			{ &envMap->m_descriptor },
			skybox._descriptorLayout->m_descriptorSetLayout, skybox._descriptorLayout->m_setLayoutBindings));*/
		skybox.AddDescriptor(vulkanDevice->GetDescriptorSet(skybox._descriptorLayout, descriptorPool, { sceneVertexUniformBuffer, modelSBVertexUniformBuffer, modelSBFragmentUniformBuffer },
			{ envMap }));
	}

	void setupPipelines()
	{
		render::PipelineProperties props;
		/*plane.AddPipeline(vulkanDevice->GetPipeline(plane._descriptorLayout->m_descriptorSetLayout, vertexLayout.m_vertexInputBindings, vertexLayout.m_vertexInputAttributes,
			engine::tools::getAssetPath() + "shaders/pbr/pbribltextured.vert.spv", engine::tools::getAssetPath() + "shaders/pbr/pbribltextured.frag.spv", mainRenderPass->GetRenderPass(), pipelineCache, props));
		*/plane.AddPipeline(vulkanDevice->GetPipeline(
			engine::tools::getAssetPath() + "shaders/pbr/pbribltextured.vert.spv","", engine::tools::getAssetPath() + "shaders/pbr/pbribltextured.frag.spv","",
			vertexLayout, plane._descriptorLayout,props, mainRenderPass));

		//render::PipelineProperties props;
		props.cullMode = render::CullMode::FRONT;
		/*skybox.AddPipeline(vulkanDevice->GetPipeline(skybox._descriptorLayout->m_descriptorSetLayout, simpleVertexLayout.m_vertexInputBindings, simpleVertexLayout.m_vertexInputAttributes,
			engine::tools::getAssetPath() + "shaders/pbr/skybox.vert.spv", engine::tools::getAssetPath() + "shaders/pbr/skybox.frag.spv", mainRenderPass->GetRenderPass(), pipelineCache, props));
		*/skybox.AddPipeline(vulkanDevice->GetPipeline(
			engine::tools::getAssetPath() + "shaders/pbr/skybox.vert.spv","", engine::tools::getAssetPath() + "shaders/pbr/skybox.frag.spv","",
			simpleVertexLayout, skybox._descriptorLayout, props, mainRenderPass));
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

		render::PipelineProperties props;
		render::VulkanPipeline* pipeline = vulkanDevice->GetPipeline(layout->m_descriptorSetLayout, {}, {},
			engine::tools::getAssetPath() + "shaders/pbr/genbrdflut.vert.spv", engine::tools::getAssetPath() + "shaders/pbr/genbrdflut.frag.spv", offscreenRenderPass->GetRenderPass(), pipelineCache, props);

		VkCommandBuffer cmdBuf = vulkanDevice->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
		offscreenRenderPass->Begin(cmdBuf,0);
		pipeline->Draw(cmdBuf);
		vkCmdDraw(cmdBuf, 3, 1, 0, 0);
		offscreenRenderPass->End(cmdBuf);
		vulkanDevice->FlushCommandBuffer(cmdBuf, queue);
		vkQueueWaitIdle(queue);

		//dbgtex.Init(vulkanDevice, BRDFLUTMap, queue, mainRenderPass->GetRenderPass(), pipelineCache);
	}

	void generateIrradianceCube()
	{	
		uint32_t dim = 64;
		const VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT;
		//irradianceMap = vulkanDevice->GetTextureCubeMap(dim, format, queue);

		render::TextureCubeMapData data;
		data.isCubeMap = true;
		data.m_height = data.m_width = dim;
		data.m_layers_no = 6;
		data.m_format = render::GfxFormat::R32G32B32A32_SFLOAT;
		irradianceMap = vulkanDevice->GetTexture(&data,queue, 6U, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, true);
		data.Destroy();

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
		/*obj.AddDescriptor(vulkanDevice->GetDescriptorSet(descriptorPool, {},
			{ &envMap->m_descriptor },
			obj._descriptorLayout->m_descriptorSetLayout, obj._descriptorLayout->m_setLayoutBindings));*/
		obj.AddDescriptor(vulkanDevice->GetDescriptorSet(obj._descriptorLayout, descriptorPool, {}, { envMap }));

		struct PushBlock {
			glm::mat4 mvp;
			// Sampling deltas
			float deltaPhi = (2.0f * float(M_PI)) / 180.0f;
			float deltaTheta = (0.5f * float(M_PI)) / 64.0f;
		} pushBlock;

		obj.SetVertexLayout(simpleVertexLayout);

		render::PipelineProperties props;
		props.cullMode = render::CullMode::FRONT;
		props.vertexConstantBlockSize = sizeof(pushBlock);
		/*obj.AddPipeline(vulkanDevice->GetPipeline(obj._descriptorLayout->m_descriptorSetLayout, obj._vertexLayout->m_vertexInputBindings, obj._vertexLayout->m_vertexInputAttributes,
			engine::tools::getAssetPath() + "shaders/pbr/filtercube.vert.spv", engine::tools::getAssetPath() + "shaders/pbr/irradiancecube.frag.spv", offscreenRenderPass->GetRenderPass(), pipelineCache, props));*/
		obj.AddPipeline(vulkanDevice->GetPipeline(
			engine::tools::getAssetPath() + "shaders/pbr/filtercube.vert.spv","", engine::tools::getAssetPath() + "shaders/pbr/irradiancecube.frag.spv","",
			obj._vertexLayout, obj._descriptorLayout, props, offscreenRenderPass));

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

		//VkCommandBuffer cmdBuf = vulkanDevice->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
		render::CommandBuffer* commandBuffer = m_device->GetCommandBuffer();
		VkCommandBuffer cmdBuf = ((render::VulkanCommandBuffer*)commandBuffer)->m_vkCommandBuffer;
		commandBuffer->Begin();
		// Change image layout for all cubemap faces to transfer destination
		irradianceMap->ChangeLayout(cmdBuf, 
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		VkViewport viewport = { 0, 0, (float)dim, (float)dim, 0.0f, 1.0f };
		for (uint32_t m = 0; m < irradianceMap->m_mipLevelsCount; m++) {
			for (uint32_t f = 0; f < 6; f++) {
				viewport.width = static_cast<float>(dim * std::pow(0.5f, m));
				viewport.height = static_cast<float>(dim * std::pow(0.5f, m));
				offscreenRenderPass->Begin(cmdBuf,0);
				vkCmdSetViewport(cmdBuf, 0, 1, &viewport);

				pushBlock.mvp = glm::perspective((float)(M_PI / 2.0), 1.0f, 0.1f, 512.0f) * matrices[f];

				//vkCmdPushConstants(cmdBuf, obj._pipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushBlock), &pushBlock);
				obj._pipeline->PushConstants(commandBuffer, &pushBlock);
				obj._pipeline->Draw(commandBuffer);
				obj.m_descriptorSets[0]->Draw(commandBuffer, obj._pipeline);

				render::VulkanVertexLayout* vjlayout = static_cast<render::VulkanVertexLayout*>(obj._vertexLayout);
				uint32_t vip = vjlayout->GetVertexInputBinding(VK_VERTEX_INPUT_RATE_VERTEX);//TODO can we add this info for each geometry?
				uint32_t iip = vjlayout->GetVertexInputBinding(VK_VERTEX_INPUT_RATE_INSTANCE);
				skybox.m_geometries[0]->Draw(&cmdBuf, vip, iip);
				offscreenRenderPass->End(cmdBuf);

				tempTex->ChangeLayout(cmdBuf,
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
				tempTex->ChangeLayout(cmdBuf,
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

			}
		}

		irradianceMap->ChangeLayout(cmdBuf,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		vulkanDevice->FlushCommandBuffer(cmdBuf, queue, false);
		vulkanDevice->DestroyDrawCommandBuffer(commandBuffer);
	}

	void generatePrefilteredCube()
	{
		uint32_t dim = 512;
		const VkFormat format = VK_FORMAT_R16G16B16A16_SFLOAT;
		//prefilterMap = vulkanDevice->GetTextureCubeMap(dim, format, queue);
		render::TextureCubeMapData data;
		data.isCubeMap = true;
		data.m_height = data.m_width = dim;
		data.m_layers_no = 6;
		data.m_format = render::GfxFormat::R16G16B16A16_SFLOAT;
		prefilterMap = vulkanDevice->GetTexture(&data, queue, 6U, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, true);
		data.Destroy();

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
		/*obj.AddDescriptor(vulkanDevice->GetDescriptorSet(descriptorPool, {},
			{ &envMap->m_descriptor },
			obj._descriptorLayout->m_descriptorSetLayout, obj._descriptorLayout->m_setLayoutBindings));*/
		obj.AddDescriptor(vulkanDevice->GetDescriptorSet(obj._descriptorLayout, descriptorPool, {}, { envMap }));

		struct PushBlock {
			glm::mat4 mvp;
			float roughness;
			uint32_t numSamples = 32u;
		} pushBlock;

		obj.SetVertexLayout(simpleVertexLayout);

		render::PipelineProperties props;
		props.cullMode = render::CullMode::FRONT;
		props.vertexConstantBlockSize = sizeof(pushBlock);
		/*obj.AddPipeline(vulkanDevice->GetPipeline(obj._descriptorLayout->m_descriptorSetLayout, obj._vertexLayout->m_vertexInputBindings, obj._vertexLayout->m_vertexInputAttributes,
			engine::tools::getAssetPath() + "shaders/pbr/filtercube.vert.spv", engine::tools::getAssetPath() + "shaders/pbr/prefilterenvmap.frag.spv", offscreenRenderPass->GetRenderPass(), pipelineCache, props));*/
		obj.AddPipeline(vulkanDevice->GetPipeline(
			engine::tools::getAssetPath() + "shaders/pbr/filtercube.vert.spv","", engine::tools::getAssetPath() + "shaders/pbr/prefilterenvmap.frag.spv","",
			obj._vertexLayout, obj._descriptorLayout, props, offscreenRenderPass));

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

		//VkCommandBuffer cmdBuf = vulkanDevice->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
		render::CommandBuffer* commandBuffer = m_device->GetCommandBuffer();
		VkCommandBuffer cmdBuf = ((render::VulkanCommandBuffer*)commandBuffer)->m_vkCommandBuffer;
		commandBuffer->Begin();

		VkViewport viewport = { 0, 0, (float)dim, (float)dim, 0.0f, 1.0f };

		prefilterMap->ChangeLayout(cmdBuf,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		for (uint32_t m = 0; m < prefilterMap->m_mipLevelsCount; m++) {
			pushBlock.roughness = (float)m / (float)(prefilterMap->m_mipLevelsCount - 1);
			for (uint32_t f = 0; f < 6; f++) {
				viewport.width = static_cast<float>(dim * std::pow(0.5f, m));
				viewport.height = static_cast<float>(dim * std::pow(0.5f, m));

				offscreenRenderPass->Begin(cmdBuf, 0);
				vkCmdSetViewport(cmdBuf, 0, 1, &viewport);

				pushBlock.mvp = glm::perspective((float)(M_PI / 2.0), 1.0f, 0.1f, 512.0f) * matrices[f];

				obj._pipeline->PushConstants(commandBuffer, &pushBlock);
				//vkCmdPushConstants(cmdBuf, obj._pipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushBlock), &pushBlock);
				obj._pipeline->Draw(commandBuffer);
				obj.m_descriptorSets[0]->Draw(commandBuffer, obj._pipeline);

				render::VulkanVertexLayout* vjlayout = static_cast<render::VulkanVertexLayout*>(obj._vertexLayout);
				uint32_t vip = vjlayout->GetVertexInputBinding(VK_VERTEX_INPUT_RATE_VERTEX);//TODO can we add this info for each geometry?
				uint32_t iip = vjlayout->GetVertexInputBinding(VK_VERTEX_INPUT_RATE_INSTANCE);
				skybox.m_geometries[0]->Draw(&cmdBuf, vip, iip);
				offscreenRenderPass->End(cmdBuf);

				tempTex->ChangeLayout(cmdBuf,
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
				tempTex->ChangeLayout(cmdBuf,
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
			}
		}

		prefilterMap->ChangeLayout(cmdBuf,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		vulkanDevice->FlushCommandBuffer(cmdBuf, queue, false);
		vulkanDevice->DestroyDrawCommandBuffer(commandBuffer);
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
			skybox.Draw(m_drawCommandBuffers[i]);
			plane.Draw(m_drawCommandBuffers[i]);
			//dbgtex.Draw(drawCmdBuffers[i]);

			DrawUI(m_drawCommandBuffers[i]);

			mainRenderPass->End(m_drawCommandBuffers[i]);

			//VK_CHECK_RESULT(vkEndCommandBuffer(drawCommandBuffers[i]));
			m_drawCommandBuffers[i]->End();
		}
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