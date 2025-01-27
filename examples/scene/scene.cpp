
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
#include "scene/SceneLoader.h"

#define FB_COLOR_HDR_FORMAT VK_FORMAT_R32G32B32A32_SFLOAT

using namespace engine;
using namespace engine::render;

class VulkanExample : public VulkanApplication
{
public:

	scene::SceneLoader scene;
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
		camera.SetPerspective(60.0f, (float)width / (float)height, 0.1f, 1024.0f);
		camera.SetRotation(glm::vec3(0.0f, 0.0f, 0.0f));
		camera.SetPosition(glm::vec3(0.0f, 10.0f, -5.0f));
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

		render::VulkanTexture* scenecolor = vulkanDevice->GetColorRenderTarget(width, height, FB_COLOR_HDR_FORMAT);
		render::VulkanTexture* scenedepth = vulkanDevice->GetDepthRenderTarget(width, height, false);

		scenepass = vulkanDevice->GetRenderPass({ scenecolor , scenedepth }, {});
		VulkanFrameBuffer* fb = vulkanDevice->GetFrameBuffer(scenepass->GetRenderPass(), width, height, { scenecolor->m_descriptor.imageView, scenedepth->m_descriptor.imageView }, { { 30.8f, 100.95f, 300.f, 1.0f } });
		scenepass->AddFrameBuffer(fb);

		scene.normalmapVS = "normalmapshadowmap.vert.spv";
		scene.normalmapFS = "normalmapshadowmap.frag.spv";
		scene.lightingVS = "phongshadowmap.vert.spv";
		scene.lightingFS = "phongshadowmap.frag.spv";
		scene.lightingTexturedFS = "phongtexturedshadowmap.frag.spv";

		scene._device = vulkanDevice;
		scene.CreateShadow(queue);
		scene.globalTextures.push_back(scene.shadowmap);

		scene_render_objects = scene.LoadFromFile(engine::tools::getAssetPath() + "models/", "tavern.obj", 10.0, vulkanDevice, queue, scenepass->GetRenderPass(), pipelineCache);
		scene.light_pos = glm::vec4(34.0f, -50.0f, 190.0f, 1.0f);
		//scene.light_pos = glm::vec4(.0f, .0f, .0f, 1.0f);

		scene.CreateShadowObjects(pipelineCache);

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
			scene.DrawShadowsInSeparatePass(drawCommandBuffers[i]);

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
		scene.Update(timer * 0.05f);
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

	virtual void update(float dt)
	{

	}

	virtual void ViewChanged()
	{
		//updateUniformBuffers();
		scene.UpdateView(timer * 0.05f);
	}

	virtual void OnUpdateUIOverlay(engine::scene::UIOverlay *overlay)
	{
		if (overlay->header("Settings")) {
			if (ImGui::SliderFloat("Light position x", &scene.light_pos.x, -200.0f, 200.0f))
			{
				scene.Update(0.0f);
			}
			if (ImGui::SliderFloat("Light position y", &scene.light_pos.y, -200.0f, 200.0f))
			{
				scene.Update(0.0f);
			}
			if (ImGui::SliderFloat("Light position z", &scene.light_pos.z, -200.0f, 200.0f))
			{
				scene.Update(0.0f);
			}
		}
	}

};

VULKAN_EXAMPLE_MAIN()