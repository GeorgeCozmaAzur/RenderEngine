
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
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/random.hpp>

#include "vulkanexamplebase.h"
#include "scene/SimpleModel.h"
#include "scene/UniformBuffersManager.h"
#include <random>
#include <filesystem>
#include <scene/Timer.h>

using namespace engine;

#define OBJECT_INSTANCES 1000
#define THREADS_NO 10

// Wrapper functions for aligned memory allocation
// There is currently no standard for this in C++ that works across all platforms and vendors, so we abstract this
void* alignedAlloc(size_t size, size_t alignment)
{
	void* data = nullptr;
#if defined(_MSC_VER) || defined(__MINGW32__)
	data = _aligned_malloc(size, alignment);
#else
	int res = posix_memalign(&data, alignment, size);
	if (res != 0)
		data = nullptr;
#endif
	return data;
}

void alignedFree(void* data)
{
#if	defined(_MSC_VER) || defined(__MINGW32__)
	_aligned_free(data);
#else
	free(data);
#endif
}

// Function to generate a random float between min and max
float randomFloat(float min, float max) {
	static std::random_device rd;
	static std::mt19937 gen(rd());
	std::uniform_real_distribution<float> dis(min, max);
	return dis(gen);
}

// Function to generate a random point within an ellipsoid
glm::vec3 randomPointInEllipsoid(float radiusX, float radiusY, float radiusZ) {
	float u = randomFloat(0, 1);
	float v = randomFloat(0, 1);
	float theta = u * 2.0 * M_PI;
	float phi = acos(2.0 * v - 1.0);
	float r = cbrt(randomFloat(0, 1)); // Cube root for uniform distribution within volume

	float x = r * radiusX * sin(phi) * cos(theta);
	float y = r * radiusY * sin(phi) * sin(theta);
	float z = r * radiusZ * cos(phi);

	return glm::vec3(x, y, z);
}

class VulkanExample : public VulkanExampleBase
{
public:

	render::VertexLayout vertexLayout = render::VertexLayout({
		render::VERTEX_COMPONENT_POSITION,
		render::VERTEX_COMPONENT_NORMAL,
		render::VERTEX_COMPONENT_UV
		
		}, {});

	engine::scene::SimpleModel plane;
	//render::VulkanTexture* colorMap;
	std::vector<std::string> textureNames;
	render::VulkanTexture* textures;

	render::VulkanBuffer* sceneVertexUniformBuffer;
	scene::UniformBuffersManager uniform_manager;
	render::VulkanBuffer* dynamic;

	glm::vec4 light_pos = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

	// Store random per-object rotations
	glm::vec3 rotations[OBJECT_INSTANCES];
	glm::vec3 directions[OBJECT_INSTANCES];
	glm::vec3 rotationSpeeds[OBJECT_INSTANCES];
	glm::vec3 translations[OBJECT_INSTANCES];

	

	// One big uniform buffer that contains all matrices
	// Note that we need to manually allocate the data to cope for GPU-specific uniform buffer offset alignments
	struct UboDataDynamic {
		glm::mat4* model{ nullptr };
	} uboDataDynamic;

	float animationTimer{ 0.0f };

	size_t dynamicAlignment{ 0 };

	Timer timer;
	uint64_t timeupdate = 0;
	uint64_t timerender = 0;
	uint64_t timewaitforgpu = 0;

	float swimduration = 10.0f;
	glm::vec3 defaultDirection = glm::vec3(0.0f, 0.0f, 1.0f);
	std::vector<glm::vec3> splinepoints{ glm::vec3(0.0,0.0,-10.0), glm::vec3(0.0,0.0,0.0), glm::vec3(20.0,0.0,0.0), glm::vec3(0.0,0.0,-20.0), glm::vec3(0.0,0.0,-20.0), glm::vec3(0.0,0.0,-10.0) };
	glm::vec3 oldtrans;
	glm::vec3 groupPosition;
	glm::vec3 groupDirection;
	glm::mat4 groupRotationMatrix;

	engine::ThreadPool threadPool;

	VulkanExample() : VulkanExampleBase(true)
	{
		zoom = -3.75f;
		rotationSpeed = 0.5f;
		rotation = glm::vec3(15.0f, 0.f, 0.0f);
		title = "Render Engine Empty Scene";
		settings.overlay = true;
		camera.type = scene::Camera::CameraType::firstperson;
		camera.movementSpeed = 5.0f;
		camera.setPerspective(60.0f, (float)width / (float)height, 0.1f, 1024.0f);
		camera.setRotation(glm::vec3(0.0f, 0.0f, 0.0f));
		camera.setTranslation(glm::vec3(0.0f, 0.0f, 0.0f));
	}

	~VulkanExample()
	{
		// Clean up used Vulkan resources 
		// Note : Inherited destructor cleans up resources stored in base class
	}

	void setupThreads()
	{
		threadPool.setThreadCount(THREADS_NO);
	}

	void setupGeometry()
	{
		int geoindex = 0;
		int instancespermdodel = OBJECT_INSTANCES/16;
		std::vector<uint32_t> constants;
		//Geometry
		for (int i = 0; i < OBJECT_INSTANCES; i++)
		{		
			if (i % instancespermdodel == 0)
			{
				std::string objectNumber = (geoindex < 9 ? "0" : "") + std::to_string(geoindex + 1);
				std::string objectName = "models/TropicalFish_obj/TropicalFish" + objectNumber + ".obj";
				std::string textureName = "models/TropicalFish_obj/TropicalFish" + objectNumber + ".jpg";
				//plane.LoadGeometry(engine::tools::getAssetPath() + "models/torusknot.obj", &vertexLayout, 0.001f, 1);	
				if (tools::fileExists(engine::tools::getAssetPath() + objectName))
					plane.LoadGeometry(engine::tools::getAssetPath() + objectName, &vertexLayout, 0.001f, 1, glm::vec3(0.0,0.0,0.0), glm::vec3(1.0f,1.0f,1.0f));
				else
					//plane.LoadGeometry(engine::tools::getAssetPath() + "models/TropicalFish_obj/TropicalFish12.obj", &vertexLayout, 0.001f, 1);
					plane.LoadGeometry(engine::tools::getAssetPath() + "models/geosphere.obj", &vertexLayout, 0.01f, 1);

				if (tools::fileExists(engine::tools::getAssetPath() + textureName))
					textureNames.push_back(engine::tools::getAssetPath() + textureName);
				else
					textureNames.push_back(engine::tools::getAssetPath() + "models/TropicalFish_obj/TropicalFish01.jpg");

				scene::Geometry* geo = plane.m_geometries.back();
				geo->SetIndexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geo->m_indexCount * sizeof(uint32_t), geo->m_indices));
				geo->SetVertexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geo->m_verticesSize * sizeof(float), geo->m_vertices));

				geoindex++;
			}
			else
			{
				scene::Geometry* geo = new scene::Geometry();
				*geo = *plane.m_geometries[plane.m_geometries.size()-1];
				plane.AddGeometry(geo);
			}
			constants.push_back(geoindex);
		}
		plane.InitGeometriesPushConstants(sizeof(uint32_t), constants.size(), constants.data());
		plane.PopulateDynamicUniformBufferIndices();
	}

	void SetupTextures()
	{
		textures = vulkanDevice->GetTextureArray(textureNames, VK_FORMAT_R8G8B8A8_UNORM, queue,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			true);
		//colorMap = vulkanDevice->GetTexture(engine::tools::getAssetPath() + "models/TropicalFish_obj/TropicalFish04.jpg", VK_FORMAT_R8G8B8A8_UNORM, queue);
	}

	void SetupUniforms()
	{
		//uniforms
		uniform_manager.SetDevice(vulkanDevice->logicalDevice);
		uniform_manager.SetEngineDevice(vulkanDevice);
		sceneVertexUniformBuffer = uniform_manager.GetGlobalUniformBuffer({ scene::UNIFORM_PROJECTION ,scene::UNIFORM_VIEW ,scene::UNIFORM_LIGHT0_POSITION, scene::UNIFORM_CAMERA_POSITION });

		size_t minUboAlignment = vulkanDevice->m_properties.limits.minUniformBufferOffsetAlignment;
		dynamicAlignment = sizeof(glm::mat4);
		if (minUboAlignment > 0) {
			dynamicAlignment = (dynamicAlignment + minUboAlignment - 1) & ~(minUboAlignment - 1);
		}

		size_t bufferSize = OBJECT_INSTANCES * dynamicAlignment;

		uboDataDynamic.model = (glm::mat4*)alignedAlloc(bufferSize, dynamicAlignment);
		assert(uboDataDynamic.model);

		std::cout << "minUniformBufferOffsetAlignment = " << minUboAlignment << std::endl;
		std::cout << "dynamicAlignment = " << dynamicAlignment << std::endl;

		dynamic = vulkanDevice->GetBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, bufferSize);
		dynamic->m_descriptor.range = dynamicAlignment;
		dynamic->Map();

		// Prepare per-object matrices with offsets and random rotations
		std::default_random_engine rndEngine(0); //benchmark.active ? 0 : (unsigned)time(nullptr));
		std::normal_distribution<float> rndDistpi(1.0, 5.0);
		for (uint32_t i = 0; i < OBJECT_INSTANCES; i++) {
			rotationSpeeds[i] = glm::vec3(rndDistpi(rndEngine), rndDistpi(rndEngine), rndDistpi(rndEngine));
		}

		float radiusX = 3.5f; // Radius along the x-axis
		float radiusY = 2.5f; // Radius along the y-axis
		float radiusZ = 5.0f; // Radius along the z-axis
		for (int i = 0; i < OBJECT_INSTANCES; ++i) {
			glm::vec3 fish = randomPointInEllipsoid(radiusX, radiusY, radiusZ);

			// Introduce some noise to make the distribution more natural
			fish.x += randomFloat(-0.1f, 0.1f);
			fish.y += randomFloat(-0.1f, 0.1f);
			fish.z += randomFloat(-0.1f, 0.1f);

			translations[i] = fish;
		}

		oldtrans = splinepoints[0];

		updateUniformBuffers();
		updateDynamicUniformBuffer();
	}

	void updateUniformBuffers()
	{
		uniform_manager.UpdateGlobalParams(scene::UNIFORM_PROJECTION, &camera.matrices.perspective, 0, sizeof(camera.matrices.perspective));
		uniform_manager.UpdateGlobalParams(scene::UNIFORM_VIEW, &camera.matrices.view, 0, sizeof(camera.matrices.view));
		uniform_manager.UpdateGlobalParams(scene::UNIFORM_LIGHT0_POSITION, &light_pos, 0, sizeof(light_pos));
		glm::vec3 cucu = -camera.position;
		uniform_manager.UpdateGlobalParams(scene::UNIFORM_CAMERA_POSITION, &cucu, 0, sizeof(camera.position));

		uniform_manager.Update();
	}
	
	std::vector<glm::vec3> bezierInterpolate(std::vector<glm::vec3> points, float time)
	{
		if (points.size() == 0)
			return points;

		std::vector<glm::vec3> outvector;
		for (int i = 0; i < points.size() - 1; i++)
		{
			outvector.push_back(glm::mix(points[i], points[i + 1], time));
		}
		if (outvector.size() == 1)
			return outvector;

		return bezierInterpolate(outvector, time);
	}

	void updateDynamicUniformBufferThreaded(int fromIndex, int toIndex)
	{
		for (uint32_t i = fromIndex; i <= toIndex; i++)
		{
			rotations[i] = cos(animationTimer * rotationSpeeds[i]) * 0.3f;

			glm::mat4* modelMat = (glm::mat4*)(((uint64_t)uboDataDynamic.model + (i * dynamicAlignment)));
			glm::vec3 finalTranslation = translations[i];//glm::vec4(translations[i], 1.0f) * groupRotationMatrix;
			*modelMat = glm::translate(glm::mat4(1.0f), finalTranslation + groupPosition);

			//*modelMat = *modelMat * groupRotationMatrix;

			//*modelMat = *modelMat * glm::rotate(glm::mat4(1.0f), rotations[i].y, glm::vec3(0.0f, 1.0f, 0.0f));
		}
	}

	void updateDynamicUniformBuffer()
	{
		animationTimer += frameTimer;
		timer.start();
		
		float normframetime = animationTimer / swimduration;
		float intpart;
		normframetime = modf(normframetime, &intpart);

		groupPosition = bezierInterpolate(splinepoints, normframetime)[0];
		groupDirection = glm::normalize(groupPosition - oldtrans);
		glm::quat rotationQuat = glm::rotation(defaultDirection, groupDirection);
		groupRotationMatrix = glm::toMat4(rotationQuat);

		int objectsPerThread = OBJECT_INSTANCES / THREADS_NO;
		/*for (uint32_t t = 0; t < THREADS_NO; t++)
		{
			int from = t * objectsPerThread;
			int to = (t + 1) * objectsPerThread - 1;
			if (t == THREADS_NO - 1)
			{
				to += OBJECT_INSTANCES % THREADS_NO;
			}
			threadPool.threads[t]->addJob([=] { updateDynamicUniformBufferThreaded(from, to); });
		}*/

		//glm::vec3 currentDirection = glm::vec3(0.0f, 0.0f, 1.0f);
		//glm::vec3 dir(0.5f, 0.5f, 0.0f);
		for (uint32_t i = 0; i < OBJECT_INSTANCES; i++) 
		{
			rotations[i] = cos(animationTimer * rotationSpeeds[i]) * 0.3f;

			glm::mat4* modelMat = (glm::mat4*)(((uint64_t)uboDataDynamic.model + (i * dynamicAlignment)));
			glm::vec3 finalTranslation = groupRotationMatrix * glm::vec4(translations[i], 1.0f);
			*modelMat = glm::translate(glm::mat4(1.0f), finalTranslation + groupPosition);

			*modelMat = *modelMat * groupRotationMatrix;

			*modelMat = *modelMat * glm::rotate(glm::mat4(1.0f), rotations[i].y, glm::vec3(0.0f, 1.0f, 0.0f));
		}
		threadPool.wait();
		oldtrans = groupPosition;

		dynamic->MemCopy(uboDataDynamic.model, dynamic->GetSize());
		dynamic->Flush(dynamic->GetSize(), 0);
		
		timer.stop();
		timeupdate = timer.elapsedMicroseconds();
	}

	//here a descriptor pool will be created for the entire app. Now it contains 1 sampler because this is what the ui overlay needs
	void setupDescriptorPool()
	{
		std::vector<VkDescriptorPoolSize> poolSizes = {
			VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
			VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1},
			VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2}
		};
		vulkanDevice->CreateDescriptorSetsPool(poolSizes, 2);
	}

	void SetupDescriptors()
	{
		std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> modelbindings
		{
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
		};
		plane.SetDescriptorSetLayout(vulkanDevice->GetDescriptorSetLayout(modelbindings));

		plane.AddDescriptor(vulkanDevice->GetDescriptorSet({ &sceneVertexUniformBuffer->m_descriptor, &dynamic->m_descriptor }, { &textures->m_descriptor },
			plane._descriptorLayout->m_descriptorSetLayout, plane._descriptorLayout->m_setLayoutBindings, dynamicAlignment));
	}

	void setupPipelines()
	{
		plane.AddPipeline(vulkanDevice->GetPipeline(plane._descriptorLayout->m_descriptorSetLayout, vertexLayout.m_vertexInputBindings, vertexLayout.m_vertexInputAttributes,
			engine::tools::getAssetPath() + "shaders/fishes/phongmodel.vert.spv", engine::tools::getAssetPath() + "shaders/fishes/phongtextured.frag.spv", mainRenderPass->GetRenderPass(), pipelineCache, false, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, sizeof(int)));
	}

	void init()
	{	
		setupThreads();
		setupGeometry();
		SetupTextures();
		SetupUniforms();
		setupDescriptorPool();
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
			plane.Draw(drawCmdBuffers[i]);

			drawUI(drawCmdBuffers[i]);

			mainRenderPass->End(drawCmdBuffers[i]);

			VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
		}
	}

	void draw()
	{
		VulkanExampleBase::prepareFrame();

		timer.start();
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

		VulkanExampleBase::submitFrame();
		timer.stop();
		timewaitforgpu = timer.elapsedMicroseconds();
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
		updateDynamicUniformBuffer();
	}

	virtual void viewChanged()
	{
		updateUniformBuffers();
	}

	virtual void OnUpdateUIOverlay(engine::scene::UIOverlay *overlay)
	{
		if (overlay->header("Settings")) {
			ImGui::Text("Update positions time: %ld", timeupdate);
			ImGui::Text("Wait for gpu time: %ld", timewaitforgpu);
		}
	}

};

VULKAN_EXAMPLE_MAIN()