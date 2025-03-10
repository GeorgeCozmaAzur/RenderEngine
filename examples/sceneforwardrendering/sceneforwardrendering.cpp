
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
#include "scene/SceneLoaderGltf.h"

#define FB_COLOR_HDR_FORMAT VK_FORMAT_R32G32B32A32_SFLOAT

using namespace engine;
using namespace engine::render;

class VulkanExample : public VulkanApplication
{
public:

	scene::SceneLoaderGltf scene;
	std::vector<scene::RenderObject*> scene_render_objects;

	render::VulkanRenderPass* scenepass = nullptr;

	render::VulkanPipeline* blackandwhitepipeline = nullptr;

	VulkanDescriptorSet* pfdesc = nullptr;

	VulkanExample() : VulkanApplication(true)
	{
		zoom = -3.75f;
		rotationSpeed = 0.5f;
		rotation = glm::vec3(15.0f, 0.f, 0.0f);
		title = "Vulkan Engine simple post effect";
		settings.overlay = true;
		camera.movementSpeed = 20.5f;
		camera.SetFlipY(true);
		camera.SetPerspective(60.0f, (float)width / (float)height, 0.1f, 1024.0f);
		camera.SetRotation(glm::vec3(0.0f, 0.0f, 0.0f));
		camera.SetPosition(glm::vec3(0.0f, 3.0f, -5.0f));
		//settings.overlay = false;
	}

	~VulkanExample()
	{
		// Clean up used Vulkan resources 
		// Note : Inherited destructor cleans up resources stored in base class

	}

	void init()
	{
		scene.SetCamera(&camera);
		scene.uniform_manager.SetEngineDevice(vulkanDevice);

		render::VulkanTexture* scenecolor = vulkanDevice->GetColorRenderTarget(width, height, swapChain.m_surfaceFormat.format);
		render::VulkanTexture* scenedepth = vulkanDevice->GetDepthRenderTarget(width, height, false);

		scenepass = vulkanDevice->GetRenderPass({ scenecolor , scenedepth }, {});
		VulkanFrameBuffer* fb = vulkanDevice->GetFrameBuffer(scenepass->GetRenderPass(), width, height, { scenecolor->m_descriptor.imageView, scenedepth->m_descriptor.imageView }, { { 0.8f, 0.8f, 0.8f, 1.0f } });
		scenepass->AddFrameBuffer(fb);

		/*scene.normalmapVS = "normalmapshadowmap.vert.spv";
		scene.normalmapFS = "normalmapshadowmap.frag.spv";
		scene.lightingVS = "phongshadowmap.vert.spv";
		scene.lightingFS = "phongshadowmap.frag.spv";*/

		scene._device = vulkanDevice;
		scene.CreateShadow(queue);
		scene.globalTextures.push_back(scene.shadowmap);

		//scene_render_objects = scene.LoadFromFile(engine::tools::getAssetPath() + "models/castle/", "modular_fort_01_4k.gltf", 10.0, vulkanDevice, queue, scenepass->GetRenderPass(), pipelineCache);
		scene_render_objects = scene.LoadFromFile(engine::tools::getAssetPath() + "models/tavern2/", "tavern.gltf", 10.0, vulkanDevice, queue, scenepass->GetRenderPass(), pipelineCache);
		//scene.light_pos = glm::vec4(0.0f, -3.0f, 0.0f, 1.0f);
		//scene.light_pos = glm::vec4(.0f, .0f, .0f, 1.0f);

		//scene.CreateShadowObjects(pipelineCache);

		std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> blurbindings
		{
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
		};
		VulkanDescriptorSetLayout* blur_layout = vulkanDevice->GetDescriptorSetLayout(blurbindings);

		blackandwhitepipeline = vulkanDevice->GetPipeline(blur_layout->m_descriptorSetLayout, {}, {},
			engine::tools::getAssetPath() + "shaders/posteffects/screenquad.vert.spv", engine::tools::getAssetPath() + "shaders/posteffects/simpletexture.frag.spv",
			mainRenderPass->GetRenderPass(), pipelineCache, false, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

		pfdesc = vulkanDevice->GetDescriptorSet({}, { &scenecolor->m_descriptor }, blur_layout->m_descriptorSetLayout, blur_layout->m_setLayoutBindings);
	}

	void BuildCommandBuffers()
	{
		VkCommandBufferBeginInfo cmdBufInfo{};
		cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		for (int32_t i = 0; i < drawCommandBuffers.size(); ++i)
		{
			VK_CHECK_RESULT(vkBeginCommandBuffer(drawCommandBuffers[i], &cmdBufInfo));
			//scene.DrawShadowsInSeparatePass(drawCommandBuffers[i]);

			scenepass->Begin(drawCommandBuffers[i], 0);

			for (int j = 0; j < scene_render_objects.size(); j++) {
				scene_render_objects[j]->Draw(drawCommandBuffers[i]);
			}

			scenepass->End(drawCommandBuffers[i]);


			mainRenderPass->Begin(drawCommandBuffers[i], i);

			blackandwhitepipeline->Draw(drawCommandBuffers[i]);
			pfdesc->Draw(drawCommandBuffers[i], blackandwhitepipeline->getPipelineLayout(), 0);
			vkCmdDraw(drawCommandBuffers[i], 3, 1, 0, 0);

			DrawUI(drawCommandBuffers[i]);

			mainRenderPass->End(drawCommandBuffers[i]);

			VK_CHECK_RESULT(vkEndCommandBuffer(drawCommandBuffers[i]));
		}
	}

	void updateUniformBuffers()
	{
		scene.Update(timer * 0.05f, queue);
	}

	void Prepare()
	{

		init();
		PrepareUI();
		BuildCommandBuffers();
		//scene.uniform_manager.UpdateGlobalParams(scene::UNIFORM_PROJECTION, &perspectiveMatrix, 0, sizeof(perspectiveMatrix));
		updateUniformBuffers();
		prepared = true;
	}
	float time = 0.0f;
	virtual void update(float dt)
	{
		time += dt;
		float flicker = 8 + 2 * sin(3.0 * time) + 1 * glm::fract(sin(time * 12.9898) * 43758.5453);

		glm::vec3 offset = glm::vec3(
			0.1 * sin(time * 2.0) + 0.05 * glm::fract(sin(time * 5.0) * 100.0),
			0.1 * sin(time * 3.5) + 0.05 * glm::fract(sin(time * 7.0) * 100.0),
			0.1 * sin(time * 1.8) + 0.05 * glm::fract(sin(time * 6.0) * 100.0)
		);

		scene.UpdateLights(0, glm::vec4(offset, 1.0f), glm::vec4(flicker),dt,queue);
		//scene.Update(dt, queue);
	}

	virtual void ViewChanged()
	{
		//updateUniformBuffers();
		scene.UpdateView(timer * 0.05f, queue);
	}

	virtual void OnUpdateUIOverlay(engine::scene::UIOverlay* overlay)
	{
		if (overlay->header("Settings")) {
			for (int i = 0; i < scene.lightPositions.size(); i++)
			{
				std::string lx = std::string("Light position x ") + std::to_string(i);
				if (ImGui::SliderFloat(lx.c_str(), &scene.lightPositions[i].x, -50.0f, 50.0f))
				{
					scene.Update(0.0f, queue);
				}
				std::string ly = std::string("Light position y ") + std::to_string(i);
				if (ImGui::SliderFloat(ly.c_str(), &scene.lightPositions[i].y, -50.0f, 50.0f))
				{
					scene.Update(0.0f, queue);
				}
				std::string lz = std::string("Light position z ") + std::to_string(i);
				if (ImGui::SliderFloat(lz.c_str(), &scene.lightPositions[i].z, -50.0f, 50.0f))
				{
					scene.Update(0.0f, queue);
				}
			}
		}
	}

};

VULKAN_EXAMPLE_MAIN()