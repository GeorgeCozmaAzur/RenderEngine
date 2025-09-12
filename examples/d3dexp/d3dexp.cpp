
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <vector>

#include "D3D12Application.h"

using namespace engine;

class VulkanExample : public D3D12Application
{
public:

	VulkanExample() : D3D12Application(true)
	{
		zoom = -3.75f;
		rotationSpeed = 0.5f;
		rotation = glm::vec3(15.0f, 0.f, 0.0f);
		title = "Render Engine Empty Scene";
		settings.overlay = true;
		camera.movementSpeed = 20.5f;
		camera.SetPerspective(60.0f, (float)width / (float)height, 0.1f, 1024.0f);
		camera.SetRotation(glm::vec3(0.0f, 0.0f, 0.0f));
		camera.SetPosition(glm::vec3(0.0f, -5.0f, -5.0f));
	}

	~VulkanExample()
	{
		// Clean up used Vulkan resources 
		// Note : Inherited destructor cleans up resources stored in base class
		
	}

	void init()
	{	

	}

	//here a descriptor pool will be created for the entire app. Now it contains 1 sampler because this is what the ui overlay needs
	void setupDescriptorPool()
	{
		/*std::vector<VkDescriptorPoolSize> poolSizes = {
			VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}
		};
		vulkanDevice->CreateDescriptorSetsPool(poolSizes, 1);*/
	}

	//void BuildCommandBuffers()
	//{
		//VkCommandBufferBeginInfo cmdBufInfo{};
		//cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		//for (int32_t i = 0; i < drawCommandBuffers.size(); ++i)
		//{
		//	VK_CHECK_RESULT(vkBeginCommandBuffer(drawCommandBuffers[i], &cmdBufInfo));

		//	mainRenderPass->Begin(drawCommandBuffers[i], i);

		//	//draw here

		//	DrawUI(drawCommandBuffers[i]);

		//	mainRenderPass->End(drawCommandBuffers[i]);

		//	VK_CHECK_RESULT(vkEndCommandBuffer(drawCommandBuffers[i]));
		//}
	//}

	void updateUniformBuffers()
	{
		
	}

	void Prepare()
	{
		init();
		setupDescriptorPool();
		PrepareUI();
		//BuildCommandBuffers();
		prepared = true;
	}

	virtual void update(float dt)
	{

	}

	virtual void ViewChanged()
	{
		updateUniformBuffers();
	}

};

VULKAN_EXAMPLE_MAIN()