
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


#include "vulkanexamplebase.h"
#include "scene/SimpleModel.h"
#include "scene/UniformBuffersManager.h"
#include "threadpool.hpp"
#include "scene/SpacePartitionTree.h"
#include "scene/Timer.h"

using namespace engine;

float randomFloat() {
	return (float)rand() / ((float)RAND_MAX + 1);
}
float randomFloatRange(float min, float max)
{
	return randomFloat() * max + min;
}

class VulkanExample : public VulkanExampleBase
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
	float timeupdate = 0.0f;
	float timerender = 0.0f;
	int visible_objects = 0;

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
		camera.setRotation(glm::vec3(0.0f, 0.0f, 0.0f));
		camera.setTranslation(glm::vec3(0.0f, 5.0f, -10.0f));
	}

	~VulkanExample()
	{
		// Clean up used Vulkan resources 
		// Note : Inherited destructor cleans up resources stored in base class
		for (auto& thread : threadData) {
			vkFreeCommandBuffers(device, thread.commandPool, thread.commandBuffer.size(), thread.commandBuffer.data());
			vkDestroyCommandPool(device, thread.commandPool, nullptr);
		}
		
		vkFreeCommandBuffers(device, threadUIData.commandPool, threadUIData.commandBuffer.size(), threadUIData.commandBuffer.data());
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

	scene::Geometry* CreateGeometrycubestuff(bool walls, bool wireframe, glm::vec3& bound1, glm::vec3& bound2)
	{
		scene::Geometry* geometry = new scene::Geometry();
		geometry->_device = device;
		geometry->m_instanceNo = 1;


		std::vector<glm::vec3> vertices;
		const int k_noVerts = 8;
		vertices.reserve(k_noVerts * 2);

		std::vector<uint32_t> ext_indices = { 0, 3, 2, 2, 1, 0, 4, 5, 6,6,7,4 };
		std::vector<uint32_t> wire_indices = { 0,1,1,2,2,3,3,0, 0,4,3,7,2,6,1,5,4,5,5,6,6,7,7,4 };

		geometry->m_indices = walls ? wire_indices.data() : ext_indices.data();

		vertices.push_back(glm::vec3(-10.0, -10.0, -10.0));
		vertices.push_back(glm::vec3(0.0, 1.0, 0.0));
		vertices.push_back(glm::vec3(10.0, -10.0, -10.0));
		vertices.push_back(glm::vec3(0.0, 1.0, 0.0));
		vertices.push_back(glm::vec3(10.0, -10.0, 10.0));
		vertices.push_back(glm::vec3(0.0, 1.0, 0.0));
		vertices.push_back(glm::vec3(-10.0, -10.0, 10.0));
		vertices.push_back(glm::vec3(0.0, 1.0, 0.0));

		vertices.push_back(glm::vec3(-10.0, 10.0, -10.0));
		vertices.push_back(glm::vec3(0.0, 0.0, 1.0));
		vertices.push_back(glm::vec3(10.0, 10.0, -10.0));
		vertices.push_back(glm::vec3(0.0, 0.0, 1.0));
		vertices.push_back(glm::vec3(10.0, 10.0, 10.0));
		vertices.push_back(glm::vec3(0.0, 0.0, 1.0));
		vertices.push_back(glm::vec3(-10.0, 10.0, 10.0));
		vertices.push_back(glm::vec3(0.0, 0.0, 1.0));

		geometry->m_vertexCount = k_noVerts;
		geometry->m_indexCount = static_cast<uint32_t>(walls ? wire_indices.size() : ext_indices.size());
		uint32_t vBufferSize = static_cast<uint32_t>(vertices.size()) * sizeof(glm::vec3);
		uint32_t iBufferSize = geometry->m_indexCount * sizeof(uint32_t);

		geometry->m_verticesSize = 16 * 3;
		geometry->m_vertices = new float[geometry->m_verticesSize];

		int vi = 0;
		for (int i = 0; i < vertices.size(); i++)
		{
			vi = i * 3;
			geometry->m_vertices[vi] = vertices[i].x;
			geometry->m_vertices[vi + 1] = vertices[i].y;
			geometry->m_vertices[vi + 2] = vertices[i].z;
		}

		bound1 = vertices[0];
		bound2 = vertices[2 * 6];

		return geometry;
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

		glm::vec3 bound1, bound2;
		tree->SetDebugGeometry(CreateGeometrycubestuff(true, true, bound1, bound2));
		tree->CreateChildren();
		tree->m_boundries.push_back(bound1);
		tree->m_boundries.push_back(bound2);
		for (auto child : tree->m_children)
		{
			child->CreateChildren();
		}
		std::vector<scene::Geometry*> geos;
		tree->GatherAllGeometries(geos);
	}

	void SetupTextures()
	{
		// Textures
		if (vulkanDevice->features.textureCompressionBC) {
			colorMap = vulkanDevice->GetTexture(engine::tools::getAssetPath() + "textures/darkmetal_bc3_unorm.ktx", VK_FORMAT_BC3_UNORM_BLOCK, queue);
		}
		else if (vulkanDevice->features.textureCompressionASTC_LDR) {
			colorMap = vulkanDevice->GetTexture(engine::tools::getAssetPath() + "textures/darkmetal_astc_8x8_unorm.ktx", VK_FORMAT_ASTC_8x8_UNORM_BLOCK, queue);
		}
		else if (vulkanDevice->features.textureCompressionETC2) {
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
			vert_uniform_buffers[i]->map();
		}
		updateglobalUniformBuffers();
		updateUniformBuffers();
	}

	//here a descriptor pool will be created for the entire app. Now it contains 1 sampler because this is what the ui overlay needs
	void setupDescriptorPool()
	{
		std::vector<VkDescriptorPoolSize> poolSizes = {
			VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2 * static_cast<uint32_t>(objectsNo)},
			VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2 * static_cast<uint32_t>(objectsNo)}
		};
		vulkanDevice->CreateDescriptorSetsPool(poolSizes, 2 * objectsNo);
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

	void init()
	{	
		setupGeometry();
		SetupTextures();
		SetupUniforms();
		setupDescriptorPool();
		SetupDescriptors();
		setupPipelines();
	}

	//just a testing function
	void buildCommandBuffers()
	{
		VkCommandBufferBeginInfo cmdBufInfo{};
		cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		for (int32_t i = 0; i < drawCmdBuffers.size(); ++i)
		{
			VK_CHECK_RESULT(vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufInfo));

			mainRenderPass->Begin(drawCmdBuffers[i], i);

			//draw here
			for (int j = 0;j < objectsNo;j++)
			{
				objects[j].Draw(drawCmdBuffers[i]);
			}

			drawUI(drawCmdBuffers[i]);

			mainRenderPass->End(drawCmdBuffers[i]);

			VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
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
			cmdPoolCreateInfo.queueFamilyIndex = swapChain.queueNodeIndex;
			cmdPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
			VK_CHECK_RESULT(vkCreateCommandPool(device, &cmdPoolCreateInfo, nullptr, &thread->commandPool));

			// One secondary command buffer per object that is updated by this thread
			thread->commandBuffer.resize(numObjectsPerThread);
			// Generate secondary command buffers for each thread
			VkCommandBufferAllocateInfo secondaryCmdBufAllocateInfo =
				engine::render::VulkanDevice::commandBufferAllocateInfo(
					thread->commandPool,
					VK_COMMAND_BUFFER_LEVEL_SECONDARY,
					thread->commandBuffer.size());
			VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &secondaryCmdBufAllocateInfo, thread->commandBuffer.data()));

			thread->objects.reserve(objectsNo / numDrawThreads);
		}
		VkCommandPoolCreateInfo cmdPoolCreateInfo{};
		cmdPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPoolCreateInfo.queueFamilyIndex = swapChain.queueNodeIndex;
		cmdPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		VK_CHECK_RESULT(vkCreateCommandPool(device, &cmdPoolCreateInfo, nullptr, &threadUIData.commandPool));

		// One secondary command buffer per object that is updated by this thread
		threadUIData.commandBuffer.resize(numObjectsPerThread);
		// Generate secondary command buffers for each thread
		VkCommandBufferAllocateInfo secondaryCmdBufAllocateInfo =
			engine::render::VulkanDevice::commandBufferAllocateInfo(
				threadUIData.commandPool,
				VK_COMMAND_BUFFER_LEVEL_SECONDARY,
				threadUIData.commandBuffer.size());
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

		drawUI(cmdBuffer);

		VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuffer));
	}

	void updateCommandBuffers(int i)
	{
		VkCommandBufferBeginInfo commandBufferBeginInfo{};
		commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		VK_CHECK_RESULT(vkBeginCommandBuffer(drawCmdBuffers[0], &commandBufferBeginInfo));

		mainRenderPass->Begin(drawCmdBuffers[0], i, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

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

		int lastpos = threadPool.threads.size() - 1;
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
		vkCmdExecuteCommands(drawCmdBuffers[0], commandBuffers.size(), commandBuffers.data());

		mainRenderPass->End(drawCmdBuffers[0]);

		VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[0]));
	}
	void updateglobalUniformBuffers()
	{
		uniform_manager.UpdateGlobalParams(scene::UNIFORM_PROJECTION, &camera.matrices.perspective, 0, sizeof(camera.matrices.perspective));
		uniform_manager.UpdateGlobalParams(scene::UNIFORM_VIEW, &camera.matrices.view, 0, sizeof(camera.matrices.view));
		uniform_manager.UpdateGlobalParams(scene::UNIFORM_LIGHT0_POSITION, &light_pos, 0, sizeof(light_pos));
		glm::vec3 cucu = -camera.position;
		uniform_manager.UpdateGlobalParams(scene::UNIFORM_CAMERA_POSITION, &cucu, 0, sizeof(camera.position));

		uniform_manager.Update();
	}

	void updateUniformBuffers()
	{	
		for (int i = 0; i < vert_uniform_buffers.size(); i++)
		{
			vert_ram_uniform_buffers[i]->projection = camera.matrices.perspective;
			vert_ram_uniform_buffers[i]->view = camera.matrices.view;
			vert_ram_uniform_buffers[i]->model = glm::translate(glm::mat4(1.0f), balls_positions[i]);
			vert_uniform_buffers[i]->copyTo(vert_ram_uniform_buffers[i], sizeof(UBOVS));
		}
		
		for (int i = 0;i < objectsNo;i++)
		{		
			vert_uniform_buffers[i]->copyTo(&vert_ram_uniform_buffers[i]->model,sizeof(vert_ram_uniform_buffers[i]->model));
		}
	}

	void draw()
	{
		VulkanExampleBase::prepareFrame();

		//TODO add fences
		timer.start();
		updateCommandBuffers(currentBuffer);
		timer.stop();
		timerender = timer.elapsedMicroseconds();

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &drawCmdBuffers[0];
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

		VulkanExampleBase::submitFrame();
	}

	void prepare()
	{
		VulkanExampleBase::prepare();
		init();
		
		prepareUI();
		//buildCommandBuffers();
		prepareMultiThreadedRenderer();
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

	virtual void viewChanged()
	{
		updateglobalUniformBuffers();
	}

	virtual void OnUpdateUIOverlay(engine::scene::UIOverlay *overlay)
	{
		if (overlay->header("Settings")) {
			ImGui::Text("%.2f time update", timeupdate);
			ImGui::Text("%.2f time render", timerender);
			ImGui::Text("%.2d visible objects", visible_objects);
		}
	}

};

VULKAN_EXAMPLE_MAIN()