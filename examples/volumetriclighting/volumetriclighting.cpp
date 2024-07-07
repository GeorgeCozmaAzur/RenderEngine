
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
#include "vulkanexamplebase.h"
#include "render/VulkanTexture.h"
#include "scene/DrawDebug.h"
#include "scene/SceneLoader.h"

#define TEX_WIDTH 160
#define TEX_HEIGHT 80
#define TEX_DEPTH 128
#define COMPUTE_GROUP_SIZE 8
#define COMPUTE_GROUP_SIZE_Z 1

#define CAMERA_NEAR_PLANE 0.1f
#define CAMERA_FAR_PLANE 256.0f

#define NUM_BLUE_NOISE_TEXTURES 16

using namespace engine;

class VulkanExample : public VulkanExampleBase
{
public:

	scene::SceneLoader scene;
	std::vector<scene::RenderObject*> scene_render_objects;

	render::VulkanDescriptorSetLayout* lightinjectiondescriptorSetLayout;
	render::VulkanDescriptorSetLayout* raymarchdescriptorSetLayout;

	render::VulkanDescriptorSet* lightinjectiondescriptorSet;
	render::VulkanDescriptorSet* raymarchdescriptorSet;

	render::VulkanPipeline* lightinjectionpipeline;
	render::VulkanPipeline* raymarchpipeline;

	render::VulkanTexture* textureColorMap;
	render::VulkanTexture* textureCompute3dTargets[2];
	render::VulkanTexture* textureCompute3dTargetRaymarch;
	//std::vector<render::VulkanTexture*> texturesBlueNoise;
	std::vector<std::string> textureBlueNoiseFilenames;
	render::VulkanTexture* textureBlueNoise;

	struct UBOCompute
	{
		glm::mat4 view;
		glm::mat4 projection;
		glm::mat4 inv_view_proj;
		glm::mat4 prev_view_proj;
		glm::mat4 light_view_proj;
		glm::vec4 camera_position;
		glm::vec4 bias_near_far_pow;
		glm::vec4 light_color;
		float time;
		int noise_index = 1;
	} uboCompute;

	render::VulkanBuffer* computeUniformBuffer;

	glm::mat4 previous_view_proj;

	struct {
		glm::mat4 view_proj;
		glm::vec4 bias_near_far_pow;
	} uboFSscene;
	//render::VulkanBuffer* globalSceneFragmentUniformBuffer = nullptr;

	scene::DrawDebugTexture dbgtex;
	float depth = 0.0f;

	float m_ct_ping_pong = false;
	int m_currentNoiseIndex = 0;

	float lightAngle = glm::radians(-45.0f);

	std::vector<VkCommandBuffer> drawShadowCmdBuffers;
	std::vector<VkCommandBuffer> drawComputeCmdBuffers;

	VulkanExample() : VulkanExampleBase(true)
	{
		zoom = -3.75f;
		rotationSpeed = 0.5f;
		rotation = glm::vec3(15.0f, 0.f, 0.0f);
		title = "Vulkan Engine Empty Scene";
		settings.overlay = true;
		camera.type = scene::Camera::CameraType::firstperson;
		camera.movementSpeed = 20.0f;
		camera.setPerspective(60.0f, (float)width / (float)height, CAMERA_NEAR_PLANE, CAMERA_FAR_PLANE);
		camera.setRotation(glm::vec3(0.0f, 0.0f, 0.0f));
		camera.setTranslation(glm::vec3(0.0f, -5.0f, 0.0f));
		//camera.setTranslation(glm::vec3(0.0f, 0.0f, 0.0f));

		previous_view_proj = camera.matrices.perspective * camera.matrices.view;
	}

	~VulkanExample()
	{
	}

	void initComputeObjects()
	{
		for (int i = 0; i < NUM_BLUE_NOISE_TEXTURES; i++)
			textureBlueNoiseFilenames.push_back(engine::tools::getAssetPath() + "textures/blue_noise/LDR_LLL1_" + std::to_string(i) + ".png");
		textureBlueNoise = vulkanDevice->GetTextureArray(textureBlueNoiseFilenames, VK_FORMAT_R8G8B8A8_UNORM, queue);

		computeUniformBuffer = vulkanDevice->GetUniformBuffer(sizeof(uboCompute));
		VK_CHECK_RESULT(computeUniformBuffer->map());

		std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> computebindings
		{
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT},
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},
			{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT}
		};
		lightinjectiondescriptorSetLayout = vulkanDevice->GetDescriptorSetLayout(computebindings);
		lightinjectiondescriptorSet = vulkanDevice->GetDescriptorSet({ &computeUniformBuffer->m_descriptor }, { &scene.shadowmap->m_descriptor,&scene.shadowmapColor->m_descriptor, &textureBlueNoise->m_descriptor,&textureCompute3dTargets[0]->m_descriptor, &textureCompute3dTargets[1]->m_descriptor },
			lightinjectiondescriptorSetLayout->m_descriptorSetLayout, lightinjectiondescriptorSetLayout->m_setLayoutBindings);

		std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> rmbindings
		{
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT},
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},
			{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT}
		};
		raymarchdescriptorSetLayout = vulkanDevice->GetDescriptorSetLayout(rmbindings);
		raymarchdescriptorSet = vulkanDevice->GetDescriptorSet({ &computeUniformBuffer->m_descriptor }, { &textureCompute3dTargets[0]->m_descriptor, &textureCompute3dTargetRaymarch->m_descriptor },
			raymarchdescriptorSetLayout->m_descriptorSetLayout, raymarchdescriptorSetLayout->m_setLayoutBindings);

		std::string fileName = engine::tools::getAssetPath() + "shaders/computeshader/" + "lightinjection" + ".comp.spv";
		lightinjectionpipeline = vulkanDevice->GetComputePipeline(fileName, device, lightinjectiondescriptorSetLayout->m_descriptorSetLayout, pipelineCache);

		fileName = engine::tools::getAssetPath() + "shaders/computeshader/" + "raymarch" + ".comp.spv";
		raymarchpipeline = vulkanDevice->GetComputePipeline(fileName, device, raymarchdescriptorSetLayout->m_descriptorSetLayout, pipelineCache);
	}

	void init()
	{	
		textureCompute3dTargets[0] = vulkanDevice->GetTextureStorage({ TEX_WIDTH, TEX_HEIGHT, TEX_DEPTH }, VK_FORMAT_R16G16B16A16_SFLOAT, queue, VK_IMAGE_VIEW_TYPE_3D);
		textureCompute3dTargets[1] = vulkanDevice->GetTextureStorage({ TEX_WIDTH, TEX_HEIGHT, TEX_DEPTH }, VK_FORMAT_R16G16B16A16_SFLOAT, queue, VK_IMAGE_VIEW_TYPE_3D);
		textureCompute3dTargetRaymarch = vulkanDevice->GetTextureStorage({ TEX_WIDTH, TEX_HEIGHT, TEX_DEPTH }, VK_FORMAT_R16G16B16A16_SFLOAT, queue, VK_IMAGE_VIEW_TYPE_3D);

		scene.globalTextures.push_back(textureCompute3dTargetRaymarch);
		scene._device = vulkanDevice;
		scene.CreateShadow(queue);
		scene.globalTextures.push_back(scene.shadowmap);
		scene.globalTextures.push_back(scene.shadowmapColor);

		scene.SetCamera(&camera);
		scene.uniform_manager.SetEngineDevice(vulkanDevice);

		scene.lightingVS = "phongshadowmap.vert.spv";
		scene.lightingFS = "phongshadowmap_vf.frag.spv";
		scene.lightingTexturedFS = "phongtexturedshadowmap_vf.frag.spv";
		scene.normalmapVS = "normalmapshadowmap.vert.spv";
		scene.normalmapFS = "normalmapshadowmap_vf.frag.spv";
		scene.sceneFragmentUniformBuffer = vulkanDevice->GetUniformBuffer(sizeof(uboFSscene));
		VK_CHECK_RESULT(scene.sceneFragmentUniformBuffer->map());
		/*scene_render_objects = scene.LoadFromFile(engine::tools::getAssetPath() + "models/sponza/", "crytek-sponza-huge-vray.obj", 0.01, vulkanDevice, queue, mainRenderPass->GetRenderPass(), pipelineCache);
		scene.light_pos = glm::vec4(34.0f, -90.0f, 40.0f, 1.0f);*/
		scene::ModelCreateInfo2 modelCreateInfo(1.0, 1.0f, glm::vec3(0.0,0,0.0));
		scene_render_objects = scene.LoadFromFile2(engine::tools::getAssetPath() + "models/sibenik/", "sibenik.dae", &modelCreateInfo, vulkanDevice, queue, mainRenderPass->GetRenderPass(), pipelineCache);
		scene.light_pos = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

		scene.CreateShadowObjects(pipelineCache);

		textureColorMap = vulkanDevice->GetTexture(engine::tools::getAssetPath() + "textures/vulkan_11_rgba.ktx", VK_FORMAT_R8G8B8A8_UNORM, queue, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(vulkanDevice->physicalDevice, VK_FORMAT_R16G16B16A16_SFLOAT, &formatProperties);
		// Check if requested image format supports image storage operations
		assert(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT);
		
		scene.Update(timer * 0.05f);

		dbgtex.Init(vulkanDevice, scene.shadowmapColor, queue, mainRenderPass->GetRenderPass(), pipelineCache);

		if (swapChain.queueNodeIndex != UINT32_MAX)
		{
			drawShadowCmdBuffers = vulkanDevice->createdrawCommandBuffers(swapChain.imageCount, swapChain.queueNodeIndex);
			drawComputeCmdBuffers = vulkanDevice->createdrawCommandBuffers(swapChain.imageCount, swapChain.queueNodeIndex);
		}
		initComputeObjects();
	}

	void buildShadowCommanBuffers()
	{
		VkCommandBufferBeginInfo cmdBufInfo{};
		cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		for (int32_t i = 0; i < drawShadowCmdBuffers.size(); ++i)
		{

			VK_CHECK_RESULT(vkBeginCommandBuffer(drawShadowCmdBuffers[i], &cmdBufInfo));

			scene.DrawShadowsInSeparatePass(drawShadowCmdBuffers[i]);

			VK_CHECK_RESULT(vkEndCommandBuffer(drawShadowCmdBuffers[i]));
		}
	}

	void buildComputeCommandBuffers()
	{
		VkCommandBufferBeginInfo cmdBufInfo{};
		cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		uint32_t read_idx = static_cast<uint32_t>(m_ct_ping_pong);
		uint32_t write_idx = static_cast<uint32_t>(!m_ct_ping_pong);

		for (int32_t i = 0; i < drawComputeCmdBuffers.size(); ++i)
		{
			VK_CHECK_RESULT(vkBeginCommandBuffer(drawComputeCmdBuffers[i], &cmdBufInfo));

			// Image memory barrier to make sure that compute shader writes are finished before sampling from the texture
			VkImageMemoryBarrier imageMemoryBarrier = {};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			// We won't be changing the layout of the image
			imageMemoryBarrier.oldLayout = scene.shadowmap->m_descriptor.imageLayout;
			imageMemoryBarrier.newLayout = scene.shadowmap->m_descriptor.imageLayout;
			imageMemoryBarrier.image = scene.shadowmap->m_vkImage;
			imageMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 };
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			vkCmdPipelineBarrier(
				drawComputeCmdBuffers[i],
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_FLAGS_NONE,
				0, nullptr,
				0, nullptr,
				1, &imageMemoryBarrier);

			lightinjectionpipeline->Draw(drawComputeCmdBuffers[i], VK_PIPELINE_BIND_POINT_COMPUTE);
			lightinjectiondescriptorSet->Draw(drawComputeCmdBuffers[i], lightinjectionpipeline->getPipelineLayout(), 0, VK_PIPELINE_BIND_POINT_COMPUTE);
			vkCmdDispatch(drawComputeCmdBuffers[i], TEX_WIDTH / COMPUTE_GROUP_SIZE, TEX_HEIGHT / COMPUTE_GROUP_SIZE, TEX_DEPTH / COMPUTE_GROUP_SIZE_Z);


			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
			imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
			imageMemoryBarrier.image = textureCompute3dTargets[write_idx]->m_vkImage;
			imageMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			vkCmdPipelineBarrier(
				drawComputeCmdBuffers[i],
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_FLAGS_NONE,
				0, nullptr,
				0, nullptr,
				1, &imageMemoryBarrier);

			raymarchpipeline->Draw(drawComputeCmdBuffers[i], VK_PIPELINE_BIND_POINT_COMPUTE);
			raymarchdescriptorSet->Draw(drawComputeCmdBuffers[i], raymarchpipeline->getPipelineLayout(), 0, VK_PIPELINE_BIND_POINT_COMPUTE);
			vkCmdDispatch(drawComputeCmdBuffers[i], TEX_WIDTH / COMPUTE_GROUP_SIZE, TEX_HEIGHT / COMPUTE_GROUP_SIZE, COMPUTE_GROUP_SIZE_Z);

			VK_CHECK_RESULT(vkEndCommandBuffer(drawComputeCmdBuffers[i]));
		}
	}

	void buildCommandBuffers()
	{
		VkCommandBufferBeginInfo cmdBufInfo{};
		cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		for (int32_t i = 0; i < drawCmdBuffers.size(); ++i)
		{
			VK_CHECK_RESULT(vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufInfo));

			// Image memory barrier to make sure that compute shader writes are finished before sampling from the texture
			VkImageMemoryBarrier imageMemoryBarrier = {};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			// We won't be changing the layout of the image
			imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
			imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
			imageMemoryBarrier.image = textureCompute3dTargetRaymarch->m_vkImage;
			imageMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			vkCmdPipelineBarrier(
				drawCmdBuffers[i],
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				VK_FLAGS_NONE,
				0, nullptr,
				0, nullptr,
				1, &imageMemoryBarrier);

			mainRenderPass->Begin(drawCmdBuffers[i], i);

			for (int j = 0; j < scene_render_objects.size(); j++) {
				if(scene_render_objects[j])
				scene_render_objects[j]->Draw(drawCmdBuffers[i]);
			}

			dbgtex.Draw(drawCmdBuffers[i]);

			drawUI(drawCmdBuffers[i]);

			mainRenderPass->End(drawCmdBuffers[i]);

			VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
		}
	}

	void updateUniformBuffers()
	{
		scene.Update(timer * 0.05f);		
	}

	void draw()
	{
		VulkanExampleBase::prepareFrame();

		std::vector<VkCommandBuffer> cmdBuffers = { drawShadowCmdBuffers[currentBuffer], drawComputeCmdBuffers[currentBuffer], drawCmdBuffers[currentBuffer] };

		submitInfo.commandBufferCount = cmdBuffers.size();
		submitInfo.pCommandBuffers = cmdBuffers.data();
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

		VulkanExampleBase::submitFrame();
	}

	void prepare()
	{
		VulkanExampleBase::prepare();
		init();
		prepareUI();
		buildShadowCommanBuffers();
		buildComputeCommandBuffers();
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
		
		depth += frameTimer * 0.15f;
		if (depth > 1.0f)
			depth = depth - 1.0f;

		m_currentNoiseIndex++;
		if (m_currentNoiseIndex > 15)
			m_currentNoiseIndex = 0;

		//scene.light_pos = glm::vec4(0.0f, sin(lightAngle) * 150, cos(lightAngle) * 150, 1.0f);
		scene.light_pos = glm::vec4(sin(lightAngle) * 80, -20.0f, cos(lightAngle) * 80, 1.0f);

		uboCompute.view = camera.matrices.view;
		uboCompute.projection = camera.matrices.perspective;
		uboCompute.inv_view_proj = glm::inverse(camera.matrices.perspective * camera.matrices.view);
		uboCompute.prev_view_proj = previous_view_proj;
		uboCompute.bias_near_far_pow = glm::vec4(0.002f, CAMERA_NEAR_PLANE, CAMERA_FAR_PLANE, 1.0f);
		uboCompute.light_view_proj = scene.uboShadowOffscreenVS.depthMVP;
		uboCompute.camera_position = glm::vec4(-camera.position, 1.0f);
		uboCompute.time = depth;
		uboCompute.noise_index = m_currentNoiseIndex;
		memcpy(computeUniformBuffer->m_mapped, &uboCompute, sizeof(uboCompute));

		previous_view_proj = camera.matrices.perspective * camera.matrices.view;

		uboFSscene.view_proj = scene.m_camera->matrices.perspective * scene.m_camera->matrices.view;
		uboFSscene.bias_near_far_pow = glm::vec4(0.002f, scene.m_camera->getNearClip(), scene.m_camera->getFarClip(), 1.0f);
		memcpy(scene.sceneFragmentUniformBuffer->m_mapped, &uboFSscene, sizeof(uboFSscene));

		dbgtex.UpdateUniformBuffers(camera.matrices.perspective, camera.matrices.view, depth);

		uint32_t read_idx = static_cast<uint32_t>(m_ct_ping_pong);
		uint32_t write_idx = static_cast<uint32_t>(!m_ct_ping_pong);

		lightinjectiondescriptorSet->Update(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr, &textureCompute3dTargets[read_idx]->m_descriptor);
		lightinjectiondescriptorSet->Update(5, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, nullptr, &textureCompute3dTargets[write_idx]->m_descriptor);
		raymarchdescriptorSet->Update(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr, &textureCompute3dTargets[write_idx]->m_descriptor);
		buildComputeCommandBuffers();
		m_ct_ping_pong = !m_ct_ping_pong;

		updateUniformBuffers();
	}

	virtual void viewChanged()
	{		
		//updateUniformBuffers();		
	}

	virtual void OnUpdateUIOverlay(engine::scene::UIOverlay *overlay)
	{
		if (overlay->header("Settings")) {
			ImGui::SliderAngle("Sun Angle", &lightAngle, 0.0f, -180.0f);
		}
	}

};

VULKAN_EXAMPLE_MAIN()