
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <vector>

#include "VulkanApplication.h"
#include "scene/SimpleModel.h"
#include "scene/UniformBuffersManager.h"
#include <random>
#include <filesystem>
#include <scene/Timer.h>

using namespace engine;

#define OBJECT_INSTANCES 1600
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

class VulkanExample : public VulkanApplication
{
public:

	render::VulkanVertexLayout vertexLayout = render::VulkanVertexLayout({
		render::VERTEX_COMPONENT_POSITION,
		render::VERTEX_COMPONENT_NORMAL,
		render::VERTEX_COMPONENT_UV
		
		}, {});

	VkDescriptorPool descriptorPool;

	engine::scene::SimpleModel allfish;
	//render::VulkanTexture* colorMap;
	std::vector<std::string> textureNames;
	render::VulkanTexture* textures;

	render::VulkanBuffer* sceneVertexUniformBuffer;
	scene::UniformBuffersManager uniform_manager;
	render::VulkanBuffer* dynamicBuffer;

	glm::vec4 light_pos = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

	// Store random per-object rotations
	//glm::vec3 rotations[OBJECT_INSTANCES];
	//glm::vec3 directions[OBJECT_INSTANCES];
	glm::vec3 rotationSpeeds[OBJECT_INSTANCES];
	glm::vec3 translations[OBJECT_INSTANCES];
	glm::vec3 scales[OBJECT_INSTANCES];
	float timeOffsets[OBJECT_INSTANCES];
	

	// One big uniform buffer that contains all matrices
	// Note that we need to manually allocate the data to cope for GPU-specific uniform buffer offset alignments
	struct UboDataDynamic {
		//glm::mat4 model;// { nullptr };
		glm::vec4 position;
		glm::vec4 scale;
	};
	UboDataDynamic *uboDataDynamic = nullptr;

	float animationTimer{ 0.0f };

	size_t dynamicAlignment{ 0 };

	Timer timer;
	uint64_t timeupdate = 0;
	uint64_t timerender = 0;
	uint64_t timewaitforgpu = 0;

	float swimduration = 100.0f;
	glm::vec3 defaultDirection = glm::vec3(0.0f, 0.0f, 1.0f);

	engine::ThreadPool threadPool;

	VulkanExample() : VulkanApplication(true)
	{
		zoom = -3.75f;
		rotationSpeed = 0.5f;
		rotation = glm::vec3(15.0f, 0.f, 0.0f);
		title = "Render Engine Empty Scene";
		settings.overlay = true;
		camera.movementSpeed = 20.0f;
		camera.SetPerspective(60.0f, (float)width / (float)height, 0.1f, 1024.0f);
		camera.SetRotation(glm::vec3(0.0f, 180.0f, 0.0f));
		camera.SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));
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
					allfish.LoadGeometry(engine::tools::getAssetPath() + objectName, &vertexLayout, 0.001f, 1, glm::vec3(0.0,0.0,0.0), glm::vec3(1.0f,1.0f,1.0f));
				else
					//plane.LoadGeometry(engine::tools::getAssetPath() + "models/TropicalFish_obj/TropicalFish12.obj", &vertexLayout, 0.001f, 1);
					allfish.LoadGeometry(engine::tools::getAssetPath() + "models/TropicalFish_obj/TropicalFish01.obj", &vertexLayout, 0.001f, 1);

				if (tools::fileExists(engine::tools::getAssetPath() + textureName))
					textureNames.push_back(engine::tools::getAssetPath() + textureName);
				else
					textureNames.push_back(engine::tools::getAssetPath() + "models/TropicalFish_obj/TropicalFish01.jpg");

				scene::Geometry* geo = allfish.m_geometries.back();
				geo->SetIndexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geo->m_indexCount * sizeof(uint32_t), geo->m_indices));
				geo->SetVertexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geo->m_verticesSize * sizeof(float), geo->m_vertices));

				geoindex++;
			}
			else
			{
				scene::Geometry* geo = new scene::Geometry();
				*geo = *allfish.m_geometries[allfish.m_geometries.size()-1];
				allfish.AddGeometry(geo);
			}
			constants.push_back(geoindex);
		}
		allfish.InitGeometriesPushConstants(sizeof(uint32_t), constants.size(), constants.data());
		allfish.PopulateDynamicUniformBufferIndices();
	}

	void SetupTextures()
	{
		render::Texture2DData data;
		data.LoadFromFiles(textureNames, render::GfxFormat::R8G8B8A8_UNORM);

		textures = vulkanDevice->GetTexture(&data, queue,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			true);

		/*textures = vulkanDevice->GetTextureArray(textureNames, VK_FORMAT_R8G8B8A8_UNORM, queue,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			true);*/
		//colorMap = vulkanDevice->GetTexture(engine::tools::getAssetPath() + "models/TropicalFish_obj/TropicalFish04.jpg", VK_FORMAT_R8G8B8A8_UNORM, queue);
	}

	void SetupUniforms()
	{
		//uniforms
		uniform_manager.SetDevice(vulkanDevice->logicalDevice);
		uniform_manager.SetEngineDevice(vulkanDevice);
		sceneVertexUniformBuffer = uniform_manager.GetGlobalUniformBuffer({ scene::UNIFORM_PROJECTION ,scene::UNIFORM_VIEW ,scene::UNIFORM_LIGHT0_POSITION, scene::UNIFORM_CAMERA_POSITION });

		size_t minUboAlignment = vulkanDevice->m_properties.limits.minUniformBufferOffsetAlignment;
		dynamicAlignment = sizeof(UboDataDynamic);
		if (minUboAlignment > 0) {
			dynamicAlignment = (dynamicAlignment + minUboAlignment - 1) & ~(minUboAlignment - 1);
		}

		size_t bufferSize = OBJECT_INSTANCES * dynamicAlignment;

		uboDataDynamic = (UboDataDynamic*)alignedAlloc(bufferSize, dynamicAlignment);
		assert(uboDataDynamic);

		std::cout << "minUniformBufferOffsetAlignment = " << minUboAlignment << std::endl;
		std::cout << "dynamicAlignment = " << dynamicAlignment << std::endl;

		dynamicBuffer = vulkanDevice->GetBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, bufferSize);
		dynamicBuffer->m_descriptor.range = dynamicAlignment;
		dynamicBuffer->Map();

		// Prepare per-object matrices with offsets and random rotations
		std::default_random_engine rndEngine(0); //benchmark.active ? 0 : (unsigned)time(nullptr));
		std::normal_distribution<float> rndDistpi(1.0, 5.0);
		std::normal_distribution<float> rndDistScale(0.01, 1.0);
		for (uint32_t i = 0; i < OBJECT_INSTANCES; i++) {
			rotationSpeeds[i] = glm::vec3(rndDistpi(rndEngine), rndDistpi(rndEngine), rndDistpi(rndEngine));
			scales[i] = glm::vec3(randomFloat(0.2f,0.5f), randomFloat(0.2f, 0.5f), randomFloat(0.2f, 0.5f));
		}

		float tripLength = 2 * M_PI * 10.0f;//(glm::length(splinepoints[1] - splinepoints[2]) + glm::length(splinepoints[2] - splinepoints[3]) + glm::length(splinepoints[3] - splinepoints[4]))/2.0f;


		float radiusX = 3.5f; // Radius along the x-axis
		float radiusY = 2.5f; // Radius along the y-axis
		float radiusZ = 5.0f; // Radius along the z-axis
		for (int i = 0; i < OBJECT_INSTANCES; ++i) {
			glm::vec3 fish = randomPointInEllipsoid(radiusX, radiusY, radiusZ);

			// Introduce some noise to make the distribution more natural
			fish.x += randomFloat(-0.1f, 0.1f);
			fish.y += randomFloat(-0.1f, 0.1f);
			fish.z += randomFloat(-0.1f, 0.1f);

			if (fish.z >= 0)
			{
				timeOffsets[i] = fish.z / tripLength;
			}
			else
			{
				timeOffsets[i] = (tripLength + fish.z) / tripLength;
			}

			translations[i] = fish;
		}

		updateUniformBuffers();
		initializeDynamicUniformBuffer();
	}

	void updateUniformBuffers()
	{
		glm::mat4 perspectiveMatrix = camera.GetPerspectiveMatrix();
		glm::mat4 viewMatrix = camera.GetViewMatrix();

		uniform_manager.UpdateGlobalParams(scene::UNIFORM_PROJECTION, &perspectiveMatrix, 0, sizeof(perspectiveMatrix));
		uniform_manager.UpdateGlobalParams(scene::UNIFORM_VIEW, &viewMatrix, 0, sizeof(viewMatrix));
		uniform_manager.UpdateGlobalParams(scene::UNIFORM_LIGHT0_POSITION, &light_pos, 0, sizeof(light_pos));
		glm::vec3 cucu = -camera.GetPosition();
		uniform_manager.UpdateGlobalParams(scene::UNIFORM_CAMERA_POSITION, &cucu, 0, sizeof(camera.GetPosition()));

		uniform_manager.Update(queue);
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
			//rotations[i] = cos(animationTimer * rotationSpeeds[i]) * 0.3f;

			//glm::mat4* modelMat = &((UboDataDynamic*)(((uint64_t)uboDataDynamic + (i * dynamicAlignment))))->model;
			//glm::vec3 finalTranslation = translations[i];//glm::vec4(translations[i], 1.0f) * groupRotationMatrix;
			//*modelMat = glm::translate(glm::mat4(1.0f), finalTranslation + groupPosition);

			//*modelMat = *modelMat * groupRotationMatrix;

			//*modelMat = *modelMat * glm::rotate(glm::mat4(1.0f), rotations[i].y, glm::vec3(0.0f, 1.0f, 0.0f));
		}
	}

	void initializeDynamicUniformBuffer()
	{
		for (uint32_t i = 0; i < OBJECT_INSTANCES; i++)
		{
			UboDataDynamic* ubodata = (UboDataDynamic*)(((uint64_t)uboDataDynamic + (i * dynamicAlignment)));		
			ubodata->position = glm::vec4(translations[i], 0.0f);
			ubodata->scale = glm::vec4(scales[i], 0.0f);
		}
		dynamicBuffer->MemCopy(uboDataDynamic, dynamicBuffer->GetSize());
		dynamicBuffer->Flush(dynamicBuffer->GetSize(), 0);
	}

	void updateDynamicUniformBuffer()
	{
		animationTimer += frameTimer;
		timer.start();
		
		//int objectsPerThread = OBJECT_INSTANCES / THREADS_NO;
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

		
		glm::mat4 rot = glm::rotate(glm::mat4(1.0f), 0.5f,glm::vec3(0.0,1.0,0.0));
		for (uint32_t i = 0; i < OBJECT_INSTANCES; i++) 
		{
			float normframetime = animationTimer / swimduration + timeOffsets[i];
			float intpart;
			normframetime = modf(normframetime, &intpart);
			int objectsPerThread = OBJECT_INSTANCES / THREADS_NO;

			UboDataDynamic* ubodata = (UboDataDynamic*)(((uint64_t)uboDataDynamic + (i * dynamicAlignment)));

			ubodata->position.w = normframetime * M_PI * 2.0;
			ubodata->scale.w = cos(animationTimer * rotationSpeeds[i].y) * 0.1f;

		}
		//threadPool.wait();

		dynamicBuffer->MemCopy(uboDataDynamic, dynamicBuffer->GetSize());
		dynamicBuffer->Flush(dynamicBuffer->GetSize(), 0);
		
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
		descriptorPool = vulkanDevice->CreateDescriptorSetsPool(poolSizes, 2);
	}

	void SetupDescriptors()
	{
		std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> modelbindings
		{
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
		};
		allfish.SetDescriptorSetLayout(vulkanDevice->GetDescriptorSetLayout(modelbindings));

		render::VulkanDescriptorSetLayout* vklayout = static_cast<render::VulkanDescriptorSetLayout*>(allfish._descriptorLayout);

		allfish.AddDescriptor(vulkanDevice->GetDescriptorSet(descriptorPool, { &sceneVertexUniformBuffer->m_descriptor, &dynamicBuffer->m_descriptor }, { &textures->m_descriptor },
			vklayout->m_descriptorSetLayout, vklayout->m_setLayoutBindings, dynamicAlignment));
	}

	void setupPipelines()
	{
		render::VulkanDescriptorSetLayout* vklayout = static_cast<render::VulkanDescriptorSetLayout*>(allfish._descriptorLayout);
		render::PipelineProperties props;
		props.vertexConstantBlockSize = sizeof(int);
		allfish.AddPipeline(vulkanDevice->GetPipeline(vklayout->m_descriptorSetLayout, vertexLayout.m_vertexInputBindings, vertexLayout.m_vertexInputAttributes,
			engine::tools::getAssetPath() + "shaders/fishes/phongmodel.vert.spv", engine::tools::getAssetPath() + "shaders/fishes/phongtextured.frag.spv", mainRenderPass->GetRenderPass(), pipelineCache, props));
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
			allfish.Draw(m_drawCommandBuffers[i]);

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
		updateDynamicUniformBuffer();
	}

	virtual void ViewChanged()
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