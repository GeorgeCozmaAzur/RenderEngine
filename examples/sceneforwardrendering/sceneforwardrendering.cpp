
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
#include "D3D12Application.h"
#include "scene/SceneLoaderGltf.h"
#include "render/vulkan/VulkanCommandBuffer.h"

#define FB_COLOR_HDR_FORMAT VK_FORMAT_R32G32B32A32_SFLOAT

using namespace engine;
using namespace engine::render;

class VulkanExample : public D3D12Application
{
public:

	scene::SceneLoaderGltf scene;
	std::vector<scene::RenderObject*> scene_render_objects;

	render::RenderPass* scenepass = nullptr;

	render::DescriptorPool* descriptorPoolPostEffects;
	render::DescriptorPool* descriptorPoolPostEffectsRTV;
	render::DescriptorPool* descriptorPoolPostEffectsDSV;

	render::Pipeline* blackandwhitepipeline = nullptr;

	render::DescriptorSet* pfdesc = nullptr;

	VulkanExample() : D3D12Application(true)
	{
		zoom = -3.75f;
		rotationSpeed = 0.5f;
		rotation = glm::vec3(15.0f, 0.f, 0.0f);
		title = "Vulkan Engine simple post effect";
		settings.overlay = true;
		camera.movementSpeed = 20.5f;
		camera.SetFlipY(false);
		camera.SetPerspective(60.0f, (float)width / (float)height, 0.1f, 1024.0f);
		camera.SetRotation(glm::vec3(0.0f, 0.0f, 0.0f));
		camera.SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));
		//settings.overlay = false;
	}

	~VulkanExample()
	{
		// Clean up used Vulkan resources 
		// Note : Inherited destructor cleans up resources stored in base class

	}

	void setupDescriptorPool()
	{
		/*std::vector<VkDescriptorPoolSize> poolSizes = {
			VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}
		};
		descriptorPoolPostEffects = vulkanDevice->CreateDescriptorSetsPool(poolSizes, 1);*/
		descriptorPoolPostEffects = m_device->GetDescriptorPool({ {render::DescriptorType::IMAGE_SAMPLER, 1} }, 1);
		descriptorPoolPostEffectsRTV = m_device->GetDescriptorPool({ {render::DescriptorType::RTV, 1} }, 1);
		descriptorPoolPostEffectsDSV = m_device->GetDescriptorPool({ {render::DescriptorType::DSV, 1} }, 1);
	}

	void init()
	{
		if (m_loadingCommandBuffer)
			m_loadingCommandBuffer->Begin();

		setupDescriptorPool();
		scene.SetCamera(&camera);
		scene.uniform_manager.SetEngineDevice(m_device);

		render::Texture* scenecolor = m_device->GetRenderTarget(width, height, render::GfxFormat::R8G8B8A8_UNORM, descriptorPoolPostEffects, descriptorPoolPostEffectsRTV, m_loadingCommandBuffer);
		render::Texture* scenedepth = m_device->GetDepthRenderTarget(width, height, render::GfxFormat::D32_FLOAT, descriptorPoolPostEffects, descriptorPoolPostEffectsDSV, m_loadingCommandBuffer, false, false);

		scenepass = m_device->GetRenderPass(width, height, { scenecolor }, scenedepth);

		/*scenepass = vulkanDevice->GetRenderPass({ scenecolor , scenedepth }, {});
		VulkanFrameBuffer* fb = vulkanDevice->GetFrameBuffer(scenepass->GetRenderPass(), width, height, { scenecolor->m_descriptor.imageView, scenedepth->m_descriptor.imageView }, { { 0.8f, 0.8f, 0.8f, 1.0f } });
		scenepass->AddFrameBuffer(fb);*/

		scene.m_loadingCommandBuffer = m_loadingCommandBuffer;
		scene.forwardShadersFolder = GetShadersPath() + "scene/";
		scene.lightingVS = "pbr" + GetVertexShadersExt();
		scene.lightingFS = "pbrtextured" + GetFragShadersExt();
		scene.normalmapVS = "pbrnormalmap" + GetVertexShadersExt();
		scene.normalmapFS = "pbrtexturednormalmap" + GetFragShadersExt();

		scene._device = m_device;
		//scene.CreateShadow();
		//scene.globalTextures.push_back(scene.shadowmap);

		//scene_render_objects = scene.LoadFromFile(engine::tools::getAssetPath() + "models/castle/", "modular_fort_01_4k.gltf", 10.0, vulkanDevice, queue, scenepass->GetRenderPass(), pipelineCache);
		scene_render_objects = scene.LoadFromFile(engine::tools::getAssetPath() + "models/tavern/", "tavern.gltf", 10.0, m_device, scenepass);
		//scene.light_pos = glm::vec4(0.0f, -3.0f, 0.0f, 1.0f);
		//scene.light_pos = glm::vec4(.0f, .0f, .0f, 1.0f);

		//scene.CreateShadowObjects(pipelineCache);

		/*std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> blurbindings
		{
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
		};
		VulkanDescriptorSetLayout* blur_layout = vulkanDevice->GetDescriptorSetLayout(blurbindings);*/
		render::DescriptorSetLayout* blur_layout = m_device->GetDescriptorSetLayout({
						{render::DescriptorType::IMAGE_SAMPLER,render::ShaderStage::FRAGMENT }
			});

		render::PipelineProperties props;
		/*blackandwhitepipeline = vulkanDevice->GetPipeline(blur_layout->m_descriptorSetLayout, {}, {},
			engine::tools::getAssetPath() + "shaders/posteffects/screenquad.vert.spv", engine::tools::getAssetPath() + "shaders/posteffects/blackandwhite.frag.spv",
			mainRenderPass->GetRenderPass(), pipelineCache, props);*/
		render::VertexLayout* emptylayout = m_device->GetVertexLayout({}, {});
		blackandwhitepipeline = m_device->GetPipeline(
			GetShadersPath() + "posteffects/screenquad" + GetVertexShadersExt(), "VSMain", GetShadersPath() + "posteffects/simpletexture" + GetFragShadersExt(), "PSMainTextured",
			emptylayout, blur_layout, props, m_mainRenderPass);

		//pfdesc = vulkanDevice->GetDescriptorSet(descriptorPoolPostEffects, {}, { &scenecolor->m_descriptor }, blur_layout->m_descriptorSetLayout, blur_layout->m_setLayoutBindings);
		pfdesc = m_device->GetDescriptorSet(blur_layout, descriptorPoolPostEffects, {  }, { scenecolor });

		PrepareUI();

		if (m_loadingCommandBuffer)
		{
			// Close the command list and execute it to begin the initial GPU setup.
			m_loadingCommandBuffer->End();
			SubmitOnQueue(m_loadingCommandBuffer);
		}

		WaitForDevice();

		m_device->FreeLoadStaggingBuffers();
	}

	void BuildCommandBuffers()
	{
		/*VkCommandBufferBeginInfo cmdBufInfo{};
		cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;*/

		for (int32_t i = 0; i < m_drawCommandBuffers.size(); ++i)
		{
			m_drawCommandBuffers[i]->Begin();
			//VK_CHECK_RESULT(vkBeginCommandBuffer(drawCommandBuffers[i], &cmdBufInfo));
			//scene.DrawShadowsInSeparatePass(drawCommandBuffers[i]);

			scenepass->Begin(m_drawCommandBuffers[i], 0);

			scene.descriptorPool->Draw(m_drawCommandBuffers[i]);
			for (int j = 0; j < scene_render_objects.size(); j++) {
				scene_render_objects[j]->Draw(m_drawCommandBuffers[i]);
			}

			scenepass->End(m_drawCommandBuffers[i]);


			m_mainRenderPass->Begin(m_drawCommandBuffers[i], i);

			descriptorPoolPostEffects->Draw(m_drawCommandBuffers[i]);
			blackandwhitepipeline->Draw(m_drawCommandBuffers[i]);
			//VkCommandBuffer vkbuffer = ((render::VulkanCommandBuffer*)m_drawCommandBuffers[i])->m_vkCommandBuffer;
			pfdesc->Draw(m_drawCommandBuffers[i], blackandwhitepipeline);
			//vkCmdDraw(drawCommandBuffers[i], 3, 1, 0, 0);
			DrawFullScreenQuad(m_drawCommandBuffers[i]);

			DrawUI(m_drawCommandBuffers[i]);

			m_mainRenderPass->End(m_drawCommandBuffers[i], i);

			//VK_CHECK_RESULT(vkEndCommandBuffer(drawCommandBuffers[i]));
			m_drawCommandBuffers[i]->End();
		}
	}

	void updateUniformBuffers()
	{
		scene.Update(timer * 0.05f);
	}

	void Prepare()
	{
		init();
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

		scene.UpdateLights(0, glm::vec4(offset, 1.0f), glm::vec4(flicker),dt);
		//scene.Update(dt, queue);
	}

	virtual void ViewChanged()
	{
		//updateUniformBuffers();
		scene.UpdateView(timer * 0.05f);
	}

	virtual void OnUpdateUIOverlay(engine::scene::UIOverlay* overlay)
	{
		if (overlay->header("Settings")) {
			for (int i = 0; i < scene.lightPositions.size(); i++)
			{
				std::string lx = std::string("Light position x ") + std::to_string(i);
				if (ImGui::SliderFloat(lx.c_str(), &scene.lightPositions[i].x, -50.0f, 50.0f))
				{
					scene.Update(0.0f);
				}
				std::string ly = std::string("Light position y ") + std::to_string(i);
				if (ImGui::SliderFloat(ly.c_str(), &scene.lightPositions[i].y, -50.0f, 50.0f))
				{
					scene.Update(0.0f);
				}
				std::string lz = std::string("Light position z ") + std::to_string(i);
				if (ImGui::SliderFloat(lz.c_str(), &scene.lightPositions[i].z, -50.0f, 50.0f))
				{
					scene.Update(0.0f);
				}
			}
		}
	}

};

VULKAN_EXAMPLE_MAIN()