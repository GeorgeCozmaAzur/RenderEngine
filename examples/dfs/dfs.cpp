
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
#include "render/vulkan/VulkanTexture.h"
#include "scene/DrawDebug.h"
#include "scene/SceneLoader.h"
#include "scene/SimpleModel.h"

#define TEX_WIDTH 128
#define TEX_HEIGHT 128
#define TEX_DEPTH 128
#define COMPUTE_GROUP_SIZE 8
#define COMPUTE_GROUP_SIZE_Z 8

#define CAMERA_NEAR_PLANE 0.1f
#define CAMERA_FAR_PLANE 256.0f

#define NUM_BLUE_NOISE_TEXTURES 16

using namespace engine;

class VulkanExample : public VulkanApplication
{
public:

	render::VulkanVertexLayout vertexLayout = render::VulkanVertexLayout({
		render::VERTEX_COMPONENT_POSITION,
		render::VERTEX_COMPONENT_NORMAL,
		render::VERTEX_COMPONENT_UV

		}, {});

	//scene::SceneLoader scene;
	//std::vector<scene::RenderObject*> scene_render_objects;
	engine::scene::SimpleModel plane;
	render::VulkanBuffer* sceneVertexUniformBuffer;
	scene::UniformBuffersManager uniform_manager;

	VkDescriptorPool descriptorPool;

	render::VulkanDescriptorSetLayout* lightinjectiondescriptorSetLayout;
	//render::VulkanDescriptorSetLayout* raymarchdescriptorSetLayout;

	render::VulkanDescriptorSet* lightinjectiondescriptorSet;
	//render::VulkanDescriptorSet* raymarchdescriptorSet;

	render::VulkanPipeline* lightinjectionpipeline;
	//render::VulkanPipeline* raymarchpipeline;

	//render::VulkanTexture* textureColorMap;
	render::VulkanTexture* textureDFS;
	//render::VulkanTexture* textureCompute3dTargets[2];
	//render::VulkanTexture* textureCompute3dTargetRaymarch;
	//std::vector<render::VulkanTexture*> texturesBlueNoise;
	//std::vector<std::string> textureBlueNoiseFilenames;
	//render::VulkanTexture* textureBlueNoise;

	struct UBOCompute
	{
		float textureWidth = TEX_WIDTH;
		float textureHeight = TEX_HEIGHT;
		float textureDepth = TEX_DEPTH;
		int triangleCount = 0;
	} uboCompute;

	render::VulkanBuffer* vertexStorageBuffer;
	render::VulkanBuffer* indexStorageBuffer;
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

	engine::scene::SimpleModel model;

	VulkanExample() : VulkanApplication(true)
	{
		zoom = -3.75f;
		rotationSpeed = 0.5f;
		rotation = glm::vec3(15.0f, 0.f, 0.0f);
		title = "Vulkan Engine Empty Scene";
		settings.overlay = true;
		camera.movementSpeed = 20.0f;
		camera.SetPerspective(60.0f, (float)width / (float)height, CAMERA_NEAR_PLANE, CAMERA_FAR_PLANE);
		camera.SetRotation(glm::vec3(0.0f, 0.0f, 0.0f));
		camera.SetPosition(glm::vec3(0.0f, 0.0f, -3.0f));
		//camera.setTranslation(glm::vec3(0.0f, 0.0f, 0.0f));

		glm::mat4 perspectiveMatrix = camera.GetPerspectiveMatrix();
		glm::mat4 viewMatrix = camera.GetViewMatrix();

		previous_view_proj = perspectiveMatrix * viewMatrix;
	}

	~VulkanExample()
	{
	}

	void initComputeObjects()
	{
		/*for (int i = 0; i < NUM_BLUE_NOISE_TEXTURES; i++)
			textureBlueNoiseFilenames.push_back(engine::tools::getAssetPath() + "textures/blue_noise/LDR_LLL1_" + std::to_string(i) + ".png");
		textureBlueNoise = vulkanDevice->GetTextureArray(textureBlueNoiseFilenames, VK_FORMAT_R8G8B8A8_UNORM, queue);*/

		computeUniformBuffer = vulkanDevice->GetUniformBuffer(sizeof(uboCompute));
		VK_CHECK_RESULT(computeUniformBuffer->Map());

		std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> computebindings
		{
			{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},
			{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},
			{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT}
		};
		lightinjectiondescriptorSetLayout = vulkanDevice->GetDescriptorSetLayout(computebindings);
		lightinjectiondescriptorSet = vulkanDevice->GetDescriptorSet(descriptorPool,
			{ &vertexStorageBuffer->m_descriptor, &indexStorageBuffer->m_descriptor, &computeUniformBuffer->m_descriptor }, { &textureDFS->m_descriptor },
			lightinjectiondescriptorSetLayout->m_descriptorSetLayout, lightinjectiondescriptorSetLayout->m_setLayoutBindings);

		/*std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> rmbindings
		{
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT},
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},
			{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT}
		};
		raymarchdescriptorSetLayout = vulkanDevice->GetDescriptorSetLayout(rmbindings);
		raymarchdescriptorSet = vulkanDevice->GetDescriptorSet({ &computeUniformBuffer->m_descriptor }, { &textureCompute3dTargets[0]->m_descriptor, &textureCompute3dTargetRaymarch->m_descriptor },
			raymarchdescriptorSetLayout->m_descriptorSetLayout, raymarchdescriptorSetLayout->m_setLayoutBindings);*/

		std::string fileName = engine::tools::getAssetPath() + "shaders/computeshader/" + "generatedf" + ".comp.spv";
		lightinjectionpipeline = vulkanDevice->GetComputePipeline(fileName, device, lightinjectiondescriptorSetLayout->m_descriptorSetLayout, pipelineCache);

		/*fileName = engine::tools::getAssetPath() + "shaders/computeshader/" + "raymarch" + ".comp.spv";
		raymarchpipeline = vulkanDevice->GetComputePipeline(fileName, device, raymarchdescriptorSetLayout->m_descriptorSetLayout, pipelineCache);*/
	}

	void generateSSOs(std::vector<scene::Geometry*> geos)
	{
		std::vector<glm::vec4> vertexBuffer;
		std::vector<uint32_t> indexBuffer;
		int vertexSize = vertexLayout.GetVertexSize(0)/sizeof(float);
		for(auto geo : geos)
		for (int i = 0; i < geo->m_verticesSize; i += vertexSize)
		{
			vertexBuffer.push_back(glm::vec4(geo->m_vertices[i], geo->m_vertices[i+1], geo->m_vertices[i+2], 1.0));
		}

		VkDeviceSize vertexStorageBufferSize = vertexBuffer.size() * sizeof(glm::vec4);
		render::VulkanBuffer* stagingBuffer;
		stagingBuffer = vulkanDevice->CreateStagingBuffer(vertexStorageBufferSize, vertexBuffer.data());
		vertexStorageBuffer = vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
			, queue, vertexStorageBufferSize, nullptr);

		int indexOffset = 0;
		for (auto geo : geos)
		{
			for (int i = 0; i < geo->m_indexCount; i++)
			{
				indexBuffer.push_back(geo->m_indices[i] + indexOffset);
			}
			indexOffset += geo->m_indexCount;
		}

		VkDeviceSize indexStorageBufferSize = indexBuffer.size() * sizeof(uint32_t);
		render::VulkanBuffer* istagingBuffer;
		istagingBuffer = vulkanDevice->CreateStagingBuffer(indexStorageBufferSize, indexBuffer.data());
		indexStorageBuffer = vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
			, queue, indexStorageBufferSize, nullptr);

		VkCommandBuffer copyCmd = vulkanDevice->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
		VkBufferCopy copyRegion = {};
		copyRegion.size = vertexStorageBufferSize;
		vkCmdCopyBuffer(copyCmd, stagingBuffer->GetVkBuffer(), vertexStorageBuffer->GetVkBuffer(), 1, &copyRegion);
		copyRegion.size = indexStorageBufferSize;
		vkCmdCopyBuffer(copyCmd, istagingBuffer->GetVkBuffer(), indexStorageBuffer->GetVkBuffer(), 1, &copyRegion);
		vulkanDevice->FlushCommandBuffer(copyCmd, queue, true);
		vulkanDevice->DestroyBuffer(stagingBuffer);
		vulkanDevice->DestroyBuffer(istagingBuffer);
		vkQueueWaitIdle(queue);
	}

	void setupDescriptorPool()
	{
		std::vector<VkDescriptorPoolSize> poolSizes = {
			VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
			VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2},
			VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2},
			VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 2}
		};
		descriptorPool = vulkanDevice->CreateDescriptorSetsPool(poolSizes, 4);
	}

	void init()
	{	
		setupDescriptorPool();
		plane.LoadGeometry(engine::tools::getAssetPath() + "models/plane.obj", &vertexLayout, 1.1f, 1, glm::vec3(0.0f, 1.0f,0.0f));
		//plane.LoadGeometry(engine::tools::getAssetPath() + "models/sphere.obj", &vertexLayout, 0.02f, 1, glm::vec3(-0.7f, 0.0f, 0.0f));
		plane.LoadGeometry(engine::tools::getAssetPath() + "models/venus.fbx", &vertexLayout, 0.1f, 1, glm::vec3(-0.7f, 1.0f, 0.0f));
		plane.LoadGeometry(engine::tools::getAssetPath() + "models/teapot.dae", &vertexLayout, 0.05f, 1, glm::vec3(0.7f, 0.0f, 0.0f));

		for (auto geo : plane.m_geometries)
		{
			geo->SetIndexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geo->m_indexCount * sizeof(uint32_t), geo->m_indices),true);
			geo->SetVertexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geo->m_verticesSize * sizeof(float), geo->m_vertices),true);
		}

		//model.LoadGeometry(engine::tools::getAssetPath() + "models/venus.fbx", &vertexLayout, 0.1f, 1);
		//model.LoadGeometry(engine::tools::getAssetPath() + "models/oak_trunk.dae", &vertexLayout, 1.0f, 1);
		//model.LoadGeometry(engine::tools::getAssetPath() + "models/teapot.dae", &vertexLayout, 0.08f, 1);
		//model.LoadGeometry(engine::tools::getAssetPath() + "models/sphere.obj", &vertexLayout, 0.1f, 1);
		//model.LoadGeometry(engine::tools::getAssetPath() + "models/cube.obj", &vertexLayout, 0.2f, 1);
		/*for (auto geo : model.m_geometries)
		{
			geo->SetIndexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geo->m_indexCount * sizeof(uint32_t), geo->m_indices),true);
			geo->SetVertexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geo->m_verticesSize * sizeof(float), geo->m_vertices),true);
		}*/
		std::vector<scene::Geometry*> geos(plane.m_geometries.begin()+1, plane.m_geometries.end());
		generateSSOs(geos);

		textureDFS = vulkanDevice->GetTextureStorage({ TEX_WIDTH, TEX_HEIGHT, TEX_DEPTH }, VK_FORMAT_R16G16B16A16_SFLOAT, queue, VK_IMAGE_VIEW_TYPE_3D);

		uniform_manager.SetDevice(vulkanDevice->logicalDevice);
		uniform_manager.SetEngineDevice(vulkanDevice);
		sceneVertexUniformBuffer = uniform_manager.GetGlobalUniformBuffer({ scene::UNIFORM_PROJECTION ,scene::UNIFORM_VIEW ,scene::UNIFORM_LIGHT0_POSITION, scene::UNIFORM_CAMERA_POSITION });
		updateUniformBuffers();
		std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> modelbindings
		{
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
		};
		plane.SetDescriptorSetLayout(vulkanDevice->GetDescriptorSetLayout(modelbindings));

		plane.AddDescriptor(vulkanDevice->GetDescriptorSet(descriptorPool, { &sceneVertexUniformBuffer->m_descriptor }, { &textureDFS->m_descriptor },
			plane._descriptorLayout->m_descriptorSetLayout, plane._descriptorLayout->m_setLayoutBindings));
		render::PipelineProperties props;
		plane.AddPipeline(vulkanDevice->GetPipeline(plane._descriptorLayout->m_descriptorSetLayout, vertexLayout.m_vertexInputBindings, vertexLayout.m_vertexInputAttributes,
			engine::tools::getAssetPath() + "shaders/basic/dfs.vert.spv", engine::tools::getAssetPath() + "shaders/basic/dfs.frag.spv", mainRenderPass->GetRenderPass(), pipelineCache, props));

		/*textureCompute3dTargets[0] = vulkanDevice->GetTextureStorage({ TEX_WIDTH, TEX_HEIGHT, TEX_DEPTH }, VK_FORMAT_R16G16B16A16_SFLOAT, queue, VK_IMAGE_VIEW_TYPE_3D);
		textureCompute3dTargets[1] = vulkanDevice->GetTextureStorage({ TEX_WIDTH, TEX_HEIGHT, TEX_DEPTH }, VK_FORMAT_R16G16B16A16_SFLOAT, queue, VK_IMAGE_VIEW_TYPE_3D);
		textureCompute3dTargetRaymarch = vulkanDevice->GetTextureStorage({ TEX_WIDTH, TEX_HEIGHT, TEX_DEPTH }, VK_FORMAT_R16G16B16A16_SFLOAT, queue, VK_IMAGE_VIEW_TYPE_3D);*/

		/*scene.globalTextures.push_back(textureDFS);
		scene._device = vulkanDevice;
		scene.CreateShadow(queue);
		scene.globalTextures.push_back(scene.shadowmap);
		scene.globalTextures.push_back(scene.shadowmapColor);

		scene.SetCamera(&camera);
		scene.uniform_manager.SetEngineDevice(vulkanDevice);

		scene.lightingVS = "dfs.vert.spv";
		scene.lightingFS = "dfs.frag.spv";
		scene.lightingTexturedFS = "phongtexturedshadowmap_vf.frag.spv";
		scene.normalmapVS = "normalmapshadowmap.vert.spv";
		scene.normalmapFS = "normalmapshadowmap_vf.frag.spv";
		scene.sceneFragmentUniformBuffer = vulkanDevice->GetUniformBuffer(sizeof(uboFSscene));
		VK_CHECK_RESULT(scene.sceneFragmentUniformBuffer->Map());
		scene::ModelCreateInfo2 modelCreateInfo(3.0, 1.0f, glm::vec3(0.0,2.0,0.0));
		scene_render_objects = scene.LoadFromFile2(engine::tools::getAssetPath() + "models/", "plane.obj", &modelCreateInfo, vulkanDevice, queue, mainRenderPass->GetRenderPass(), pipelineCache);
		scene.light_pos = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f); 

		scene.CreateShadowObjects(pipelineCache);*/

		//textureColorMap = vulkanDevice->GetTexture(engine::tools::getAssetPath() + "textures/vulkan_11_rgba.ktx", VK_FORMAT_R8G8B8A8_UNORM, queue, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(vulkanDevice->physicalDevice, VK_FORMAT_R16G16B16A16_SFLOAT, &formatProperties);
		// Check if requested image format supports image storage operations
		assert(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT);
		
		//scene.Update(timer * 0.05f, queue);

		dbgtex.Init(vulkanDevice, descriptorPool, textureDFS, queue, mainRenderPass->GetRenderPass(), pipelineCache);

		initComputeObjects();
	}

	virtual void CreateCommandBuffers()
	{
		if (vulkanDevice->queueFamilyIndices.graphicsFamily == UINT32_MAX)
			return;
		// Create one command buffer for each swap chain image and reuse for rendering
		
		//drawShadowCmdBuffers = vulkanDevice->CreatedrawCommandBuffers(swapChain.swapChainImageViews.size(), vulkanDevice->queueFamilyIndices.graphicsFamily);
		drawComputeCmdBuffers = vulkanDevice->CreatedrawCommandBuffers(swapChain.swapChainImageViews.size(), vulkanDevice->queueFamilyIndices.graphicsFamily);
		drawCommandBuffers = vulkanDevice->CreatedrawCommandBuffers(swapChain.swapChainImageViews.size(), vulkanDevice->queueFamilyIndices.graphicsFamily);

		//allDrawCommandBuffers.push_back(drawShadowCmdBuffers);
		allDrawCommandBuffers.push_back(drawComputeCmdBuffers);
		allDrawCommandBuffers.push_back(drawCommandBuffers);
		
	}

	/*void buildShadowCommanBuffers()
	{
		VkCommandBufferBeginInfo cmdBufInfo{};
		cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		for (int32_t i = 0; i < drawShadowCmdBuffers.size(); ++i)
		{

			VK_CHECK_RESULT(vkBeginCommandBuffer(drawShadowCmdBuffers[i], &cmdBufInfo));

			scene.DrawShadowsInSeparatePass(drawShadowCmdBuffers[i]);

			VK_CHECK_RESULT(vkEndCommandBuffer(drawShadowCmdBuffers[i]));
		}
	}*/

	void addComputeToGraphicsBarriers(VkCommandBuffer commandBuffer)
	{
		VkBufferMemoryBarrier bufferBarrier{};
		bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		bufferBarrier.srcAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
		bufferBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		bufferBarrier.size = VK_WHOLE_SIZE;
		std::vector<VkBufferMemoryBarrier> bufferBarriers;
		bufferBarrier.buffer = indexStorageBuffer->GetVkBuffer();
		bufferBarriers.push_back(bufferBarrier);
		bufferBarrier.buffer = vertexStorageBuffer->GetVkBuffer();
		bufferBarriers.push_back(bufferBarrier);
		vkCmdPipelineBarrier(
			commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_FLAGS_NONE,
			0, nullptr,
			static_cast<uint32_t>(bufferBarriers.size()), bufferBarriers.data(),
			0, nullptr);
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
			/*scene.shadowmap->PipelineBarrier(drawComputeCmdBuffers[i],
				VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
				);*/

			lightinjectionpipeline->Draw(drawComputeCmdBuffers[i], VK_PIPELINE_BIND_POINT_COMPUTE);
			lightinjectiondescriptorSet->Draw(drawComputeCmdBuffers[i], lightinjectionpipeline->getPipelineLayout(), 0, VK_PIPELINE_BIND_POINT_COMPUTE);
			vkCmdDispatch(drawComputeCmdBuffers[i], TEX_WIDTH / COMPUTE_GROUP_SIZE, TEX_HEIGHT / COMPUTE_GROUP_SIZE, TEX_DEPTH / COMPUTE_GROUP_SIZE_Z);

			VK_CHECK_RESULT(vkEndCommandBuffer(drawComputeCmdBuffers[i]));
		}

		/*VkCommandBuffer cmdBuf = vulkanDevice->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
		addComputeToGraphicsBarriers(cmdBuf);
		lightinjectionpipeline->Draw(cmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE);
		lightinjectiondescriptorSet->Draw(cmdBuf, lightinjectionpipeline->getPipelineLayout(), 0, VK_PIPELINE_BIND_POINT_COMPUTE);
		vkCmdDispatch(cmdBuf, TEX_WIDTH / COMPUTE_GROUP_SIZE, TEX_HEIGHT / COMPUTE_GROUP_SIZE, TEX_DEPTH / COMPUTE_GROUP_SIZE_Z);
		vulkanDevice->FlushCommandBuffer(cmdBuf, queue);
		vkQueueWaitIdle(queue);*/
	}

	virtual void BuildCommandBuffers()
	{
		VkCommandBufferBeginInfo cmdBufInfo{};
		cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		for (int32_t i = 0; i < drawCommandBuffers.size(); ++i)
		{
			VK_CHECK_RESULT(vkBeginCommandBuffer(drawCommandBuffers[i], &cmdBufInfo));

			// Image memory barrier to make sure that compute shader writes are finished before sampling from the texture
			textureDFS->PipelineBarrier(drawCommandBuffers[i],
				VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
				);

			mainRenderPass->Begin(drawCommandBuffers[i], i);

			plane.Draw(drawCommandBuffers[i]);

			/*for (int j = 0; j < scene_render_objects.size(); j++) {
				if(scene_render_objects[j])
				scene_render_objects[j]->Draw(drawCommandBuffers[i]);
			}*/

			dbgtex.Draw(drawCommandBuffers[i]);

			DrawUI(drawCommandBuffers[i]);

			mainRenderPass->End(drawCommandBuffers[i]);

			VK_CHECK_RESULT(vkEndCommandBuffer(drawCommandBuffers[i]));
		}
	}

	void updateUniformBuffers()
	{
		glm::mat4 perspectiveMatrix = camera.GetPerspectiveMatrix();
		glm::mat4 viewMatrix = camera.GetViewMatrix();

		uniform_manager.UpdateGlobalParams(scene::UNIFORM_PROJECTION, &perspectiveMatrix, 0, sizeof(perspectiveMatrix));
		uniform_manager.UpdateGlobalParams(scene::UNIFORM_VIEW, &viewMatrix, 0, sizeof(viewMatrix));
		glm::vec4 light_pos = glm::vec4(0.0f, -3.0f, 0.0f, 1.0f);
		light_pos.x = light_pos.x + cos(glm::radians(timer * 360.0f)) * 5.0f;
		light_pos.z = light_pos.z + sin(glm::radians(timer * 360.0f)) * 5.0f;

		uniform_manager.UpdateGlobalParams(scene::UNIFORM_LIGHT0_POSITION, &light_pos, 0, sizeof(light_pos));
		glm::vec3 cucu = -camera.GetPosition();
		uniform_manager.UpdateGlobalParams(scene::UNIFORM_CAMERA_POSITION, &cucu, 0, sizeof(cucu));

		uniform_manager.Update(queue);
	}

	void Prepare()
	{
		
		init();
		PrepareUI();
		//buildShadowCommanBuffers();
		buildComputeCommandBuffers();
		BuildCommandBuffers();
		
		prepared = true;
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
		//scene.light_pos = glm::vec4(sin(lightAngle) * 80, -20.0f, cos(lightAngle) * 80, 1.0f);

		glm::mat4 perspectiveMatrix = camera.GetPerspectiveMatrix();
		glm::mat4 viewMatrix = camera.GetViewMatrix();

		//uboCompute.view = viewMatrix;
		//uboCompute.projection = perspectiveMatrix;
		//uboCompute.inv_view_proj = glm::inverse(perspectiveMatrix * viewMatrix);
		//uboCompute.prev_view_proj = previous_view_proj;
		//uboCompute.bias_near_far_pow = glm::vec4(0.002f, CAMERA_NEAR_PLANE, CAMERA_FAR_PLANE, 1.0f);
		//uboCompute.light_view_proj = scene.uboShadowOffscreenVS.depthMVP;
		//uboCompute.camera_position = glm::vec4(-camera.GetPosition(), 1.0f);
		//uboCompute.time = depth;
		uboCompute.triangleCount = plane.m_geometries[1]->m_indexCount + plane.m_geometries[2]->m_indexCount;//m_currentNoiseIndex;
		computeUniformBuffer->MemCopy(&uboCompute, sizeof(uboCompute));

		previous_view_proj = perspectiveMatrix * viewMatrix;

		/*uboFSscene.view_proj = perspectiveMatrix * viewMatrix;
		uboFSscene.bias_near_far_pow = glm::vec4(0.002f, scene.m_camera->getNearClip(), scene.m_camera->getFarClip(), 1.0f);
		scene.sceneFragmentUniformBuffer->MemCopy(&uboFSscene, sizeof(uboFSscene));*/

		glm::mat4 transM = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f,-6.0f,0.0f));

		dbgtex.UpdateUniformBuffers(perspectiveMatrix, viewMatrix * transM, depth);

		//uint32_t read_idx = static_cast<uint32_t>(m_ct_ping_pong);
		//uint32_t write_idx = static_cast<uint32_t>(!m_ct_ping_pong);

		//vkWaitForFences(device, submitFences.size(), submitFences.data(), VK_TRUE, UINT64_MAX);

		//lightinjectiondescriptorSet->Update(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr, &textureCompute3dTargets[read_idx]->m_descriptor);
		//lightinjectiondescriptorSet->Update(5, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, nullptr, &textureCompute3dTargets[write_idx]->m_descriptor);
		//raymarchdescriptorSet->Update(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr, &textureCompute3dTargets[write_idx]->m_descriptor);
		//buildComputeCommandBuffers();
		//m_ct_ping_pong = !m_ct_ping_pong;

		updateUniformBuffers();
	}

	virtual void ViewChanged()
	{		
		//updateUniformBuffers();		
		
	}
	int framesno = 0;
	void Render()
	{
		if (!prepared)
			return;

		vkWaitForFences(device, 1, &submitFences[currentBuffer], VK_TRUE, UINT64_MAX);

		VkResult result = swapChain.acquireNextImage(presentCompleteSemaphores[currentBuffer], &currentBuffer);
		// Recreate the swapchain if it's no longer compatible with the surface (OUT_OF_DATE) or no longer optimal for presentation (SUBOPTIMAL)
		if ((result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR)) {
			WindowResize();
		}
		else {
			VK_CHECK_RESULT(result);
		}

		vkResetFences(device, 1, &submitFences[currentBuffer]);

		framesno++;

		int dif = framesno > 2 ? 1 : 0;

		std::vector<VkCommandBuffer> submitCommandBuffers(allDrawCommandBuffers.size()-dif);
		for (int i = 0; i < submitCommandBuffers.size(); i++)
		{
			submitCommandBuffers[i] = allDrawCommandBuffers[i+dif][currentBuffer];
		}
		submitInfo.pWaitSemaphores = &presentCompleteSemaphores[currentBuffer];
		submitInfo.pSignalSemaphores = &renderCompleteSemaphores[currentBuffer];
		submitInfo.commandBufferCount = submitCommandBuffers.size();
		submitInfo.pCommandBuffers = submitCommandBuffers.data();
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, submitFences[currentBuffer]));

		//VulkanApplication::PresentFrame();
		result = swapChain.queuePresent(presentationQueue, currentBuffer, renderCompleteSemaphores[currentBuffer]);
		if (!((result == VK_SUCCESS) || (result == VK_SUBOPTIMAL_KHR))) {
			if (result == VK_ERROR_OUT_OF_DATE_KHR) {
				// Swap chain is no longer compatible with the surface and needs to be recreated
				WindowResize();
				return;
			}
			else {
				VK_CHECK_RESULT(result);
			}
		}
		currentBuffer = (currentBuffer + 1) % swapChain.swapChainImageViews.size();
	}

	virtual void OnUpdateUIOverlay(engine::scene::UIOverlay *overlay)
	{
		if (overlay->header("Settings")) {
			ImGui::SliderAngle("Sun Angle", &lightAngle, 0.0f, -180.0f);
		}
	}

};

VULKAN_EXAMPLE_MAIN()