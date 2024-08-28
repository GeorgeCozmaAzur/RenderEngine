
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <vector>
#include <time.h> 
#include <random>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>


#include "VulkanApplication.h"
#include "scene/SimpleModel.h"
#include "scene/UniformBuffersManager.h"
#include "threadpool.hpp"
#include "scene/SpacePartitionTree.h"
#include "scene/Timer.h"
#include "scene/DrawDebug.h"

using namespace engine;

float randomFloat() {
	return (float)rand() / ((float)RAND_MAX + 1);
}
float randomFloatRange(float min, float max)
{
	return randomFloat() * max + min;
}

class VulkanExample : public VulkanApplication
{
public:

	const float GRAVITY = 8.0f;

	render::VertexLayout vertexLayout = render::VertexLayout({
		render::VERTEX_COMPONENT_POSITION,
		render::VERTEX_COMPONENT_NORMAL,
		render::VERTEX_COMPONENT_UV
		}, {});

	render::VertexLayout vertexLayoutInstanced = render::VertexLayout({
		render::VERTEX_COMPONENT_POSITION,
		render::VERTEX_COMPONENT_UV,
		render::VERTEX_COMPONENT_COLOR,
		render::VERTEX_COMPONENT_NORMAL
		},
		{ render::VERTEX_COMPONENT_POSITION });

	struct ThreadData {
		VkCommandPool commandPool;
		// One command buffer per render object
		std::vector<VkCommandBuffer> commandBuffer;
		std::vector<engine::scene::SimpleModel*> objects;
	};
	std::vector<ThreadData> threadData;
	ThreadData threadUIData;
	engine::ThreadPool threadPool;
	// Number of animated objects to be renderer
	// by using threads and secondary command buffers
	uint32_t numObjectsPerThread;

	// Multi threaded stuff
	// Max. number of concurrent threads
	uint32_t numDrawThreads;

	std::vector<engine::scene::SimpleModel> objects;
	render::VulkanTexture* colorMap;

	render::VulkanBuffer* sceneVertexUniformBuffer;
	scene::UniformBuffersManager uniform_manager;

	glm::vec4 light_pos = glm::vec4(0.0f, -50.0f, 0.0f, 1.0f);

	struct UBOVS {
		glm::mat4 projection;
		glm::mat4 view;
		glm::mat4 model;
	};

	std::vector<render::VulkanBuffer*> vert_uniform_buffers;
	std::vector<UBOVS*> vert_ram_uniform_buffers;

	render::VulkanDescriptorSetLayout *objectslayout;
	render::VulkanPipeline* objectsPipeline;

	std::vector<scene::BoundingSphere*> balls;
	std::vector<glm::vec3> balls_positions;
	int objectsNo = 500;
	struct CubeLmitation
	{
		glm::vec3 plane_limit;
		glm::vec4 plane_eq;
		CubeLmitation(glm::vec3 limit, glm::vec4 eq) : plane_limit(limit), plane_eq(eq) {}

	};
	std::vector<CubeLmitation> cube_limitations;

	scene::SpacePartitionTree* tree = new scene::SpacePartitionTree;

	Timer timer;
	uint64_t timeadvance = 0;
	uint64_t timeupdate = 0;
	uint64_t timerender = 0;
	int visible_objects = 0;

	scene::DrawDebugBBs dbgbb;

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
		camera.SetPosition(glm::vec3(0.0f, 5.0f, -10.0f));
	}

	~VulkanExample()
	{
		// Clean up used Vulkan resources 
		// Note : Inherited destructor cleans up resources stored in base class
		for (auto& thread : threadData) {
			vkFreeCommandBuffers(device, thread.commandPool, static_cast<uint32_t>(thread.commandBuffer.size()), thread.commandBuffer.data());
			vkDestroyCommandPool(device, thread.commandPool, nullptr);
		}
		
		vkFreeCommandBuffers(device, threadUIData.commandPool, static_cast<uint32_t>(threadUIData.commandBuffer.size()), threadUIData.commandBuffer.data());
		vkDestroyCommandPool(device, threadUIData.commandPool, nullptr);

		for (auto bla : vert_ram_uniform_buffers)
		{
			delete bla;
		}
		for (auto bla : balls)
		{
			delete bla;
		}
		delete tree;
	}

	void setupGeometry()
	{
		objects.resize(objectsNo);
		//Geometry
		for (int i = 0;i < objectsNo;i++)
		{
			objects[i].LoadGeometry(engine::tools::getAssetPath() + "models/sphere.obj", &vertexLayout, 0.01f * randomFloatRange(0.5, 1.0), 1);
			for (auto geo : objects[i].m_geometries)
			{
				geo->SetIndexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geo->m_indexCount * sizeof(uint32_t), geo->m_indices));
				geo->SetVertexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geo->m_verticesSize * sizeof(float), geo->m_vertices));
			}
		}

		std::default_random_engine rndGenerator((unsigned)time(nullptr));
		std::uniform_int_distribution<int> rndWidthIndex(-10, 10);
		std::uniform_int_distribution<int> rndLengthIndex(-10, 10);
		std::uniform_int_distribution<int> rndHeightIndex(-10, 10);

		balls.resize(objectsNo);

		balls_positions.resize(objectsNo);

		vert_ram_uniform_buffers.resize(objectsNo);

		for (int i = 0; i < objectsNo; i++)
		{
			int x = rndWidthIndex(rndGenerator);
			int y = rndLengthIndex(rndGenerator);
			int z = rndHeightIndex(rndGenerator);
			float radius = glm::length(objects[i].m_boundingBoxes[0]->GetPoint1());
			scene::BoundingSphere* ball = new scene::BoundingSphere(glm::vec3((float)x, (float)y, (float)z),
				glm::vec3(8.0 * randomFloat(),
					8.0 * randomFloat(),
					8.0 * randomFloat()),
				radius);

			balls_positions[i] = ball->GetCenter();
			balls[i] = ball;

			UBOVS* ubovs = new UBOVS;
			vert_ram_uniform_buffers[i] = ubovs;
		}
		cube_limitations.push_back(CubeLmitation(glm::vec3(0, 10, 0), glm::vec4(0, -1, 0, 10)));
		cube_limitations.push_back(CubeLmitation(glm::vec3(0, -10, 0), glm::vec4(0, 1, 0, 10)));
		cube_limitations.push_back(CubeLmitation(glm::vec3(0, -10, 0), glm::vec4(-1, 0, 0, 10)));
		cube_limitations.push_back(CubeLmitation(glm::vec3(0, -10, 0), glm::vec4(1, 0, 0, 10)));
		cube_limitations.push_back(CubeLmitation(glm::vec3(0, -10, 0), glm::vec4(0, 0, 1, 10)));
		cube_limitations.push_back(CubeLmitation(glm::vec3(0, -10, 0), glm::vec4(0, 0, -1, 10)));

		glm::vec3 bound1(-10.0f,-10.0f,-10.0f), bound2(10.0f,10.0f,10.0f);
		tree->m_boundries.push_back(bound1);
		tree->m_boundries.push_back(bound2);
		tree->CreateChildren();
		
		for (auto child : tree->m_children)
		{
			child->CreateChildren();
		}
		
	}

	void SetupTextures()
	{
		// Textures
		if (vulkanDevice->m_enabledFeatures.textureCompressionBC) {
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
		}
	}

	void SetupUniforms()
	{
		//uniforms
		uniform_manager.SetDevice(vulkanDevice->logicalDevice);
		uniform_manager.SetEngineDevice(vulkanDevice);
		sceneVertexUniformBuffer = uniform_manager.GetGlobalUniformBuffer({ scene::UNIFORM_PROJECTION ,scene::UNIFORM_VIEW ,scene::UNIFORM_LIGHT0_POSITION, scene::UNIFORM_CAMERA_POSITION });

		vert_uniform_buffers.resize(objectsNo);
		
		for (int i = 0;i < objectsNo;i++)
		{
			vert_uniform_buffers[i] = vulkanDevice->GetUniformBuffer(sizeof(UBOVS));
			vert_uniform_buffers[i]->Map();
		}
		updateglobalUniformBuffers();
		updateUniformBuffers();
	}

	//here a descriptor pool will be created for the entire app. Now it contains 1 sampler because this is what the ui overlay needs
	void setupDescriptorPool()
	{
		std::vector<VkDescriptorPoolSize> poolSizes = {
			VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2 * static_cast<uint32_t>(objectsNo) + 2},
			VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2 * static_cast<uint32_t>(objectsNo)}
		};
		vulkanDevice->CreateDescriptorSetsPool(poolSizes, 2 * objectsNo+2);
	}

	void SetupDescriptors()
	{
		//descriptors
		std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> modelbindings
		{
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
		};

		objectslayout = vulkanDevice->GetDescriptorSetLayout(modelbindings);
		for (int i = 0;i < objectsNo;i++)
		{
			objects[i].SetDescriptorSetLayout(objectslayout);
			objects[i].AddDescriptor(vulkanDevice->GetDescriptorSet({ &sceneVertexUniformBuffer->m_descriptor, &vert_uniform_buffers[i]->m_descriptor }, { &colorMap->m_descriptor },
				objects[i]._descriptorLayout->m_descriptorSetLayout, objects[i]._descriptorLayout->m_setLayoutBindings));
		}
	}

	void setupPipelines()
	{
		objectsPipeline = vulkanDevice->GetPipeline(objectslayout->m_descriptorSetLayout, vertexLayout.m_vertexInputBindings, vertexLayout.m_vertexInputAttributes,
			engine::tools::getAssetPath() + "shaders/multithreaded/phong.vert.spv", engine::tools::getAssetPath() + "shaders/multithreaded/phongtextured.frag.spv", mainRenderPass->GetRenderPass(), pipelineCache);

		for (int i = 0;i < objectsNo;i++)
		{
			objects[i].AddPipeline(objectsPipeline);
		}
	}

	std::vector<float> constants;

	void init()
	{	
		setupGeometry();
		SetupTextures();
		SetupUniforms();
		setupDescriptorPool();
		SetupDescriptors();
		setupPipelines();

		std::vector <std::vector<glm::vec3>> boundries;
		tree->GatherAllBoundries(boundries);
		dbgbb.Init(boundries, vulkanDevice, sceneVertexUniformBuffer,queue,mainRenderPass->GetRenderPass(), pipelineCache, sizeof(float));
		
		constants.resize(boundries.size());
		std::fill(constants.begin(), constants.end(), 1.0f);
		dbgbb.InitGeometriesPushConstants(sizeof(float), constants.size(), constants.data());
	}

	//just a testing function
	void BuildCommandBuffers()
	{
		VkCommandBufferBeginInfo cmdBufInfo{};
		cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		for (int32_t i = 0; i < drawCommandBuffers.size(); ++i)
		{
			VK_CHECK_RESULT(vkBeginCommandBuffer(drawCommandBuffers[i], &cmdBufInfo));

			mainRenderPass->Begin(drawCommandBuffers[i], i);

			//draw here
			for (int j = 0;j < objectsNo;j++)
			{
				objects[j].Draw(drawCommandBuffers[i]);
			}
			dbgbb.Draw(drawCommandBuffers[i]);

			DrawUI(drawCommandBuffers[i]);

			mainRenderPass->End(drawCommandBuffers[i]);

			VK_CHECK_RESULT(vkEndCommandBuffer(drawCommandBuffers[i]));
		}
	}

	// Create all threads and initialize shader push constants
	void prepareMultiThreadedRenderer()
	{
		numDrawThreads = 10;//std::thread::hardware_concurrency();
		assert(numDrawThreads > 0);
#if defined(__ANDROID__)
		LOGD("numThreads = %d", numThreads);
#else
		std::cout << "numThreads = " << numDrawThreads << std::endl;
#endif
		threadPool.setThreadCount(numDrawThreads + 2);
		numObjectsPerThread = 1;//512 / numThreads;

		threadData.resize(numDrawThreads);

		for (uint32_t i = 0; i < numDrawThreads; i++) {
			ThreadData* thread = &threadData[i];

			// Create one command pool for each thread
			VkCommandPoolCreateInfo cmdPoolCreateInfo{};
			cmdPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			cmdPoolCreateInfo.queueFamilyIndex = vulkanDevice->queueFamilyIndices.graphicsFamily;
			cmdPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			VK_CHECK_RESULT(vkCreateCommandPool(device, &cmdPoolCreateInfo, nullptr, &thread->commandPool));

			// One secondary command buffer per object that is updated by this thread
			thread->commandBuffer.resize(numObjectsPerThread);
			// Generate secondary command buffers for each thread
			VkCommandBufferAllocateInfo secondaryCmdBufAllocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO , nullptr, thread->commandPool, VK_COMMAND_BUFFER_LEVEL_SECONDARY, static_cast<uint32_t>(thread->commandBuffer.size()) };
			VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &secondaryCmdBufAllocateInfo, thread->commandBuffer.data()));

			thread->objects.reserve(objectsNo / numDrawThreads);
		}
		VkCommandPoolCreateInfo cmdPoolCreateInfo{};
		cmdPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPoolCreateInfo.queueFamilyIndex = vulkanDevice->queueFamilyIndices.graphicsFamily;
		cmdPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		VK_CHECK_RESULT(vkCreateCommandPool(device, &cmdPoolCreateInfo, nullptr, &threadUIData.commandPool));

		// One secondary command buffer per object that is updated by this thread
		threadUIData.commandBuffer.resize(numObjectsPerThread);
		// Generate secondary command buffers for each thread
		VkCommandBufferAllocateInfo secondaryCmdBufAllocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO , nullptr, threadUIData.commandPool, VK_COMMAND_BUFFER_LEVEL_SECONDARY, static_cast<uint32_t>(threadUIData.commandBuffer.size()) };

		VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &secondaryCmdBufAllocateInfo, threadUIData.commandBuffer.data()));

	}

	void threadRenderCode(uint32_t threadIndex, uint32_t cmdBufferIndex, VkCommandBufferInheritanceInfo inheritanceInfo)
	{
		ThreadData* thread = &threadData[threadIndex];

		VkCommandBufferBeginInfo commandBufferBeginInfo{};
		commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
		commandBufferBeginInfo.pInheritanceInfo = &inheritanceInfo;
		VkCommandBuffer cmdBuffer = thread->commandBuffer[cmdBufferIndex];

		VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &commandBufferBeginInfo));

		VkViewport viewport = { 0, 0, (float)width, (float)height, 0.0f, 1.0f };
		vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

		VkRect2D scissor = { VkOffset2D{0,0}, VkExtent2D{width, height} };
		vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

		for (auto object : thread->objects)
		{
			if(object)
				object->Draw(cmdBuffer);
		}

		VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuffer));
	}

	void threadRenderUICode(VkCommandBufferInheritanceInfo inheritanceInfo)
	{
		ThreadData* thread = &threadUIData;

		VkCommandBufferBeginInfo commandBufferBeginInfo{};
		commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
		commandBufferBeginInfo.pInheritanceInfo = &inheritanceInfo;
		VkCommandBuffer cmdBuffer = thread->commandBuffer[0];

		VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &commandBufferBeginInfo));

		DrawUI(cmdBuffer);

		VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuffer));
	}

	void updateCommandBuffers(int i)
	{
		VkCommandBufferBeginInfo commandBufferBeginInfo{};
		commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		VK_CHECK_RESULT(vkBeginCommandBuffer(drawCommandBuffers[0], &commandBufferBeginInfo));

		mainRenderPass->Begin(drawCommandBuffers[0], i, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

		// Contains the list of secondary command buffers to be submitted
		std::vector<VkCommandBuffer> commandBuffers;

		// Inheritance info for the secondary command buffers
		VkCommandBufferInheritanceInfo cmdBufferInheritanceInfo{};
		cmdBufferInheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
		cmdBufferInheritanceInfo.renderPass = mainRenderPass->GetRenderPass();
		// Secondary command buffer also use the currently active framebuffer
		cmdBufferInheritanceInfo.framebuffer = mainRenderPass->m_frameBuffers[i]->m_vkFrameBuffer;

		numObjectsPerThread = visible_objects / numDrawThreads;
		int restofjobs = visible_objects % numDrawThreads;

		for (uint32_t t = 0; t < numDrawThreads; t++)
		{
			threadData[t].objects.clear();
		}

		int thread_index = 0;

		int o = 0;
		for (o = 0; o<objects.size();o++ )
		{
			if (balls[o]->IsVisible())
			{
				if (thread_index == threadData.size())
					break;

				threadData[thread_index].objects.push_back(&objects[o]);

				if (threadData[thread_index].objects.size() >= numObjectsPerThread)
				{
					thread_index++;
				}
			}
		}

		thread_index = 0;
		//distribute the remaining jobs
		for (int oo = o; oo < objects.size();oo++)
		{
			if (thread_index == threadData.size())
				thread_index = 0;
			threadData[thread_index].objects.push_back(&objects[oo]);
			thread_index++;
		}

		for (uint32_t t = 0; t < numDrawThreads; t++)
		{
			if(threadData[t].objects.size() != 0)
			{
				threadPool.threads[t]->addJob([=] { threadRenderCode(t, 0, cmdBufferInheritanceInfo); });
			}
		}

		auto lastpos = threadPool.threads.size() - 1;
		threadPool.threads[lastpos-1]->addJob([=] { threadRenderUICode(cmdBufferInheritanceInfo); });
		threadPool.threads[lastpos]->addJob([=] { updateUniformBuffers(); });

		threadPool.wait();

		for (uint32_t t = 0; t < numDrawThreads; t++)
		{
			//for (uint32_t i = 0; i < numObjectsPerThread; i++)
			{
				if (threadData[t].objects.size() != 0)
				{
					commandBuffers.push_back(threadData[t].commandBuffer[0]);
				}
			}
		}
		commandBuffers.push_back(threadUIData.commandBuffer[0]);

		// Execute render commands from the secondary command buffer
		vkCmdExecuteCommands(drawCommandBuffers[0], static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

		mainRenderPass->End(drawCommandBuffers[0]);

		VK_CHECK_RESULT(vkEndCommandBuffer(drawCommandBuffers[0]));
	}
	void updateglobalUniformBuffers()
	{
		glm::mat4 perspectiveMatrix = camera.GetPerspectiveMatrix();
		glm::mat4 viewMatrix = camera.GetViewMatrix();

		uniform_manager.UpdateGlobalParams(scene::UNIFORM_PROJECTION, &perspectiveMatrix, 0, sizeof(perspectiveMatrix));
		uniform_manager.UpdateGlobalParams(scene::UNIFORM_VIEW, &viewMatrix, 0, sizeof(viewMatrix));
		uniform_manager.UpdateGlobalParams(scene::UNIFORM_LIGHT0_POSITION, &light_pos, 0, sizeof(light_pos));
		glm::vec3 cucu = -camera.GetPosition();
		uniform_manager.UpdateGlobalParams(scene::UNIFORM_CAMERA_POSITION, &cucu, 0, sizeof(camera.GetPosition()));

		uniform_manager.Update();
	}

	void updateUniformBuffers()
	{	
		glm::mat4 perspectiveMatrix = camera.GetPerspectiveMatrix();
		glm::mat4 viewMatrix = camera.GetViewMatrix();

		for (int i = 0; i < vert_uniform_buffers.size(); i++)
		{
			vert_ram_uniform_buffers[i]->projection = perspectiveMatrix;
			vert_ram_uniform_buffers[i]->view = viewMatrix;
			vert_ram_uniform_buffers[i]->model = glm::translate(glm::mat4(1.0f), balls_positions[i]);
			vert_uniform_buffers[i]->MemCopy(vert_ram_uniform_buffers[i], sizeof(UBOVS));
		}
		
		for (int i = 0;i < objectsNo;i++)
		{		
			vert_uniform_buffers[i]->MemCopy(&vert_ram_uniform_buffers[i]->model,sizeof(vert_ram_uniform_buffers[i]->model));
		}
	}
	bool multithreaded = true;
	void draw()
	{
		VulkanApplication::PrepareFrame();

		//TODO add fences
		timer.start();
		if(multithreaded)
			updateCommandBuffers(currentBuffer);
		else
		{
			BuildCommandBuffers();
			updateUniformBuffers();
		}
		memcpy(dbgbb._geometriesPushConstants, constants.data(), constants.size()*sizeof(float));
		
		timer.stop();
		timerender = timer.elapsedMicroseconds();

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &drawCommandBuffers[multithreaded? 0 : currentBuffer];
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

		VulkanApplication::PresentFrame();
	}

	void Prepare()
	{
		
		init();
		
		PrepareUI();
		if (multithreaded)
			prepareMultiThreadedRenderer();
		else
			BuildCommandBuffers();
		
		prepared = true;
	}

	virtual void Render()
	{
		if (!prepared)
			return;
		draw();
	}

	void SetTreeColors(scene::SpacePartitionTree* root, int &index)
	{
		if (root->m_objects.size() > 0)
		{
			constants[index] = 1.0f;
		}
		else
			constants[index] = 0.0f;
		index++;
		for (int i = 0; i < root->m_children.size(); i++)
		{
			SetTreeColors(root->m_children[i], index);
		}
	}

	virtual void update(float dt)
	{
		timer.start();

		for (int i = 0; i < objectsNo; i++)
		{
			balls[i]->AddToVelocity(glm::vec3(0, GRAVITY * dt * 0.5f, 0));
			balls[i]->AddToPosition(balls[i]->GetVelocity() * dt * 0.5f);
			balls_positions[i] = balls[i]->GetCenter();

			for (auto climit : cube_limitations)
			{
				glm::vec3 plane_normal = climit.plane_eq;
				float res = balls[i]->GetCenter().x * climit.plane_eq.x + balls[i]->GetCenter().y * climit.plane_eq.y + balls[i]->GetCenter().z * climit.plane_eq.z + climit.plane_eq.w;

				if (res <= 0.0f)
				{
					plane_normal *= 2 * glm::dot(balls[i]->GetVelocity(), plane_normal);
					balls[i]->AddToVelocity(-plane_normal);
				}
				int q = 2;
				
			}
			tree->AdvanceObject(balls[i]);		
		}

		timer.stop();
		timeadvance = timer.elapsedMicroseconds();
		timer.start();

		int ind = 0;
		SetTreeColors(tree, ind);

		tree->TestCollisions();

		/*for (int i = 0; i < objectsNo - 1; i++)
		{
			for (int j = i + 1; j < objectsNo; j++)
			{
				if (balls[i]->IsClose(balls[j]))
				{
					balls[i]->Collide(balls[j]);
					balls[j]->Collide(balls[i]);
				}
			}
		}*/
		visible_objects = 0;
		glm::vec4 frustumPlanes[6];
		memcpy(frustumPlanes, camera.GetFrustum()->m_planes.data(), sizeof(glm::vec4) * 6);
		tree->ResetVisibility();
		tree->TestVisibility(frustumPlanes);

		for (int i = 0; i < objectsNo; i++)
		{
			visible_objects += balls[i]->IsVisible();
		}

		timer.stop();
		timeupdate = timer.elapsedMicroseconds();
	}

	virtual void ViewChanged()
	{
		updateglobalUniformBuffers();
	}

	virtual void OnUpdateUIOverlay(engine::scene::UIOverlay *overlay)
	{
		if (overlay->header("Settings")) {
			ImGui::Text("%ld time advance", timeadvance);
			ImGui::Text("%ld time update", timeupdate);
			ImGui::Text("%ld time render", timerender);
			ImGui::Text("%.2d visible objects", visible_objects);
		}
	}

};

VULKAN_EXAMPLE_MAIN()