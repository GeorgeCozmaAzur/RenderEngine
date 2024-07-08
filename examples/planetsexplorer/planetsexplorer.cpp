
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
#include <glm/gtx/string_cast.hpp>

#include "vulkanexamplebase.h"
#include "scene/SimpleModel.h"
#include "scene/UniformBuffersManager.h"
#include "scene/TerrainSphere.h"
#include "Rings.h"
#include "scene/DrawDebug.h"

using namespace engine;

#define SHADOWMAP_DIM 2048

class VulkanExample : public VulkanExampleBase
{
public:

	render::VertexLayout simpleVertexLayout = render::VertexLayout({
		render::VERTEX_COMPONENT_POSITION
		}, {});

	render::VertexLayout vertexLayout = render::VertexLayout({
		render::VERTEX_COMPONENT_POSITION,
		render::VERTEX_COMPONENT_NORMAL,
		render::VERTEX_COMPONENT_UV
		}, {});

	render::VertexLayout vertexLayoutNM = render::VertexLayout({
		render::VERTEX_COMPONENT_POSITION,
		render::VERTEX_COMPONENT_NORMAL,
		render::VERTEX_COMPONENT_UV,
		render::VERTEX_COMPONENT_TANGENT,
		render::VERTEX_COMPONENT_BITANGENT
		}, {});

	engine::scene::SimpleModel sun;
	engine::scene::SimpleModel saturn;
	engine::scene::Rings rings;
	engine::scene::TerrainUVSphere myplanet;
	//scene::SimpleModel skybox;
	scene::TerrainUVSphere atmosphere;
	scene::RenderObject shadowobjects;
	
	render::VulkanTexture* colorMap;
	render::VulkanTexture* sunMap;
	render::VulkanTexture* saturnMap;
	render::VulkanTexture* ringsMap;
	render::VulkanTexture* bluenoise;
	//render::VulkanTexture* envMap;

	render::VulkanBuffer* sceneVertexUniformBuffer;
	scene::UniformBuffersManager uniform_manager;

	struct {
		glm::mat4 modelView;
	} uboVS;
	render::VulkanBuffer* uniformBufferMPVS = nullptr;
	render::VulkanBuffer* uniformBufferSunVS = nullptr;
	struct {
		glm::mat4 model;
	} modelUniformVS;
	render::VulkanBuffer* modelSBVertexUniformBuffer;
	glm::mat4 mainplanetmatrix;

	struct {
		glm::mat4 cameraInvProjection;
		glm::mat4 cameraInvView;
	} matricesUniformVS;
	render::VulkanBuffer* matricesUniformBuffer;

	struct {
	
		glm::vec4	sun;//poistion and intensity of the sun
		glm::vec4	cameraPosition;
		glm::vec4	viewDirection;
		glm::vec4	dimensions;//radius of the planet, radius of the atmosphere, Rayleigh scale height, Mie scale height
		glm::vec4	scatteringCoefficients;//Rayleigh and Mie scattering coefficiants 
		float distanceFactor;
	} modelUniformAtmosphereFS;
	render::VulkanBuffer* modelAtmosphereFragmentUniformBuffer;

	glm::vec4 light_pos = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

	float depthBiasConstant = 2.25f;
	float depthBiasSlope = 1.75f;
	struct {
		glm::mat4 depthMVP;
	} uboOffscreenVS;
	glm::mat4 depthProjectionMatrix;
	glm::mat4 depthViewMatrix;
	render::VulkanTexture* shadowtex = nullptr;
	render::VulkanRenderPass* offscreenPass;
	render::VulkanBuffer* uniformBufferoffscreen = nullptr;

	scene::DrawDebugVectors drawdebugvectors;

	render::VulkanRenderPass* scenepass = nullptr;
	render::VulkanPipeline* peffpipeline = nullptr;
	render::VulkanDescriptorSet* peffdesc = nullptr;
	render::VulkanTexture* scenecolor;
	render::VulkanTexture* scenepositions;
	render::VulkanTexture* scenedepth;

	float planetRotation = 0.0f;

	float rayleighDensity = 1.00f;
	float mieDensity = 0.1f;

	float scatteringCoeficient = 15.0f;
	float distanceFactor = 0.1f;

	float sunIntensity = 20.0f;

	float farplane = 300000.0f;

	VulkanExample() : VulkanExampleBase(true)
	{
		zoom = -3.75f;
		rotationSpeed = 0.5f;
		rotation = glm::vec3(15.0f, 0.f, 0.0f);
		title = "Render Engine Empty Scene";
		settings.overlay = true;
		camera.type = scene::Camera::CameraType::firstperson;
		camera.subtype = scene::Camera::CameraSubType::surface;
		camera.movementSpeed = 300.0f;
		camera.setPerspective(45.0f, (float)width / (float)height, 1.0f, farplane);
		camera.setRotation(glm::vec3(0.0f, 0.0f, 0.0f));	
	}

	~VulkanExample()
	{
		// Clean up used Vulkan resources 
		// Note : Inherited destructor cleans up resources stored in base class
		//terrain.Destroy();
	}

	void prepareOffscreenRenderpass()
	{
		shadowtex = vulkanDevice->GetDepthRenderTarget(SHADOWMAP_DIM, SHADOWMAP_DIM, true);
		offscreenPass = vulkanDevice->GetRenderPass({ { shadowtex->m_format, shadowtex->m_descriptor.imageLayout } });
		render::VulkanFrameBuffer* fb = vulkanDevice->GetFrameBuffer(offscreenPass->GetRenderPass(), SHADOWMAP_DIM, SHADOWMAP_DIM, { shadowtex->m_descriptor.imageView });
		offscreenPass->AddFrameBuffer(fb);
		
	}

	void setupGeometry()
	{
		//Geometry
		saturn.LoadGeometry(engine::tools::getAssetPath() + "models/geosphere.obj", &vertexLayout, 300.0f, 1);
		for (auto geo : saturn.m_geometries)
		{
			geo->SetIndexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geo->m_indexCount * sizeof(uint32_t), geo->m_indices));
			geo->SetVertexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geo->m_verticesSize * sizeof(float), geo->m_vertices),true);
		}
		sun.LoadGeometry(engine::tools::getAssetPath() + "models/geosphere.obj", &vertexLayout, 100.0f, 1, glm::vec3(0.0,0,0.0));
		for (auto geo : sun.m_geometries)
		{
			geo->SetIndexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geo->m_indexCount * sizeof(uint32_t), geo->m_indices));
			geo->SetVertexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geo->m_verticesSize * sizeof(float), geo->m_vertices), true);
		}

		/*skybox.LoadGeometry(engine::tools::getAssetPath() + "models/cube.obj", &simpleVertexLayout, 20.0f, 1);
		for (auto geo : skybox.m_geometries)
		{
			geo->SetIndexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geo->m_indexCount * sizeof(uint32_t), geo->m_indices));
			geo->SetVertexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geo->m_verticesSize * sizeof(float), geo->m_vertices));
		}*/
	}

	void SetupTextures()
	{
		scenecolor = vulkanDevice->GetColorRenderTarget(width, height, FB_COLOR_HDR_FORMAT);
		scenepositions = vulkanDevice->GetColorRenderTarget(width, height, VK_FORMAT_R16G16B16A16_SFLOAT);
		scenedepth = vulkanDevice->GetDepthRenderTarget(width, height, true);

		scenepass = vulkanDevice->GetRenderPass({ scenecolor, scenepositions, scenedepth }, 0);
		render::VulkanFrameBuffer* fb = vulkanDevice->GetFrameBuffer(scenepass->GetRenderPass(), width, height, { scenecolor->m_descriptor.imageView, scenepositions->m_descriptor.imageView, scenedepth->m_descriptor.imageView }, { { 0.0f, 0.0f, 0.0f, 1.0f } });
		scenepass->AddFrameBuffer(fb);

		scenepass->SetClearColor({ farplane, farplane, farplane, 0.0f }, 1);

		colorMap = vulkanDevice->GetTexture(engine::tools::getAssetPath() + "textures/planets/marsmap1k.jpg", VK_FORMAT_R8G8B8A8_UNORM, queue);
		sunMap = vulkanDevice->GetTexture(engine::tools::getAssetPath() + "textures/planets/2k_sun.jpg", VK_FORMAT_R8G8B8A8_UNORM, queue);
		ringsMap = vulkanDevice->GetTexture(engine::tools::getAssetPath() + "textures/planets/2k_saturn_ring_alpha.png", VK_FORMAT_R8G8B8A8_UNORM, queue);
		saturnMap = vulkanDevice->GetTexture(engine::tools::getAssetPath() + "textures/planets/2k_saturn.jpg", VK_FORMAT_R8G8B8A8_UNORM, queue);
		bluenoise = vulkanDevice->GetTexture(engine::tools::getAssetPath() + "textures/blue_noise/LDR_LLL1_0.png", VK_FORMAT_R8G8B8A8_UNORM, queue);
		//envMap = vulkanDevice->GetTextureCubeMap(engine::tools::getAssetPath() + "textures/hdr/pisa_cube.ktx", VK_FORMAT_R16G16B16A16_SFLOAT, queue);

		prepareOffscreenRenderpass();
	}

	void SetupUniforms()
	{
		//uniforms
		uniform_manager.SetDevice(vulkanDevice->logicalDevice);
		uniform_manager.SetEngineDevice(vulkanDevice);
		sceneVertexUniformBuffer = uniform_manager.GetGlobalUniformBuffer({ scene::UNIFORM_PROJECTION ,scene::UNIFORM_VIEW, scene::UNIFORM_LIGHT0_SPACE ,scene::UNIFORM_LIGHT0_POSITION, scene::UNIFORM_CAMERA_POSITION });
		uniformBufferMPVS = vulkanDevice->GetUniformBuffer(sizeof(uboVS), true, queue);
		uniformBufferMPVS->map();

		uniformBufferSunVS = vulkanDevice->GetUniformBuffer(sizeof(uboVS), true, queue);
		uniformBufferSunVS->map();

		modelSBVertexUniformBuffer = vulkanDevice->GetUniformBuffer(sizeof(modelUniformVS), true, queue);
		modelSBVertexUniformBuffer->map();

		uniformBufferoffscreen = vulkanDevice->GetUniformBuffer(sizeof(uboOffscreenVS), true, queue);
		uniformBufferoffscreen->map();

		matricesUniformBuffer = vulkanDevice->GetUniformBuffer(sizeof(matricesUniformVS), true, queue);
		matricesUniformBuffer->map();

		updateUniformBuffers();
	}

	void setupDescriptorPool()
	{
		std::vector<VkDescriptorPoolSize> poolSizes = {
			VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 17},
			VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10}
		};
		vulkanDevice->CreateDescriptorSetsPool(poolSizes, 10);
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

		sun.SetDescriptorSetLayout(vulkanDevice->GetDescriptorSetLayout(modelbindings));
		sun.AddDescriptor(vulkanDevice->GetDescriptorSet({ &sceneVertexUniformBuffer->m_descriptor, &uniformBufferSunVS->m_descriptor }, { &sunMap->m_descriptor },
			sun._descriptorLayout->m_descriptorSetLayout, sun._descriptorLayout->m_setLayoutBindings));

		saturn.SetDescriptorSetLayout(vulkanDevice->GetDescriptorSetLayout(modelbindings));
		saturn.AddDescriptor(vulkanDevice->GetDescriptorSet({ &sceneVertexUniformBuffer->m_descriptor, &uniformBufferMPVS->m_descriptor }, { &saturnMap->m_descriptor },
			saturn._descriptorLayout->m_descriptorSetLayout, saturn._descriptorLayout->m_setLayoutBindings));

		/*std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> skyboxbindings
		{
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
		};
		skybox.SetDescriptorSetLayout(vulkanDevice->GetDescriptorSetLayout(skyboxbindings));

		skybox.AddDescriptor(vulkanDevice->GetDescriptorSet({ &sceneVertexUniformBuffer->m_descriptor, &modelSBVertexUniformBuffer->m_descriptor },
			{ &envMap->m_descriptor },
			skybox._descriptorLayout->m_descriptorSetLayout, skybox._descriptorLayout->m_setLayoutBindings));*/
	}

	void setupPipelines()
	{
		VkPipelineColorBlendAttachmentState opaqueState{};
		opaqueState.blendEnable = VK_FALSE;
		opaqueState.colorWriteMask = 0xf;
		std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStates{ opaqueState, opaqueState };

		render::PipelineProperties props;
		props.attachmentCount = blendAttachmentStates.size();
		props.pAttachments = blendAttachmentStates.data();
		/*terrain.AddPipeline(vulkanDevice->GetPipeline(terrain._descriptorLayout->m_descriptorSetLayout, vertexLayoutNM.m_vertexInputBindings, vertexLayoutNM.m_vertexInputAttributes,
			engine::tools::getAssetPath() + "shaders/basic/normalmap.vert.spv", engine::tools::getAssetPath() + "shaders/basic/normalmap.frag.spv", mainRenderPass->GetRenderPass(), pipelineCache));*/
		scenepass->SetClearColor({ 0.0f, 0.0f, 0.0f, 0.0f }, 0);
		sun.AddPipeline(vulkanDevice->GetPipeline(sun._descriptorLayout->m_descriptorSetLayout, sun._vertexLayout->m_vertexInputBindings, sun._vertexLayout->m_vertexInputAttributes,
			engine::tools::getAssetPath() + "shaders/planet/planet.vert.spv", engine::tools::getAssetPath() + "shaders/planet/sun.frag.spv", scenepass->GetRenderPass(), pipelineCache, props));

		saturn.AddPipeline(vulkanDevice->GetPipeline(saturn._descriptorLayout->m_descriptorSetLayout, saturn._vertexLayout->m_vertexInputBindings, saturn._vertexLayout->m_vertexInputAttributes,
			engine::tools::getAssetPath() + "shaders/planet/planet.vert.spv", engine::tools::getAssetPath() + "shaders/planet/planet.frag.spv", scenepass->GetRenderPass(), pipelineCache, props));

		/*skybox.AddPipeline(vulkanDevice->GetPipeline(skybox._descriptorLayout->m_descriptorSetLayout, simpleVertexLayout.m_vertexInputBindings, simpleVertexLayout.m_vertexInputAttributes,
			engine::tools::getAssetPath() + "shaders/basic/skybox.vert.spv", engine::tools::getAssetPath() + "shaders/basic/skybox.frag.spv", scenepass->GetRenderPass(), pipelineCache));*/
	
	}  

	void setupSphere()
	{
		VkPipelineColorBlendAttachmentState opaqueState{};
		opaqueState.blendEnable = VK_FALSE;
		opaqueState.colorWriteMask = 0xf;
		std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStates{ opaqueState, opaqueState };

		render::PipelineProperties sphereprops;
		sphereprops.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		sphereprops.attachmentCount = blendAttachmentStates.size();
		sphereprops.pAttachments = blendAttachmentStates.data();

		std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStatesTransparent;
		blendAttachmentStatesTransparent.push_back(engine::tools::pipelineColorBlendAttachmentState(0xf, VK_TRUE));
		blendAttachmentStatesTransparent.push_back(engine::tools::pipelineColorBlendAttachmentState(0xf, VK_TRUE));

		render::PipelineProperties transprops;
		transprops.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		transprops.blendEnable = true;
		transprops.attachmentCount = blendAttachmentStatesTransparent.size();
		transprops.pAttachments = blendAttachmentStatesTransparent.data();

		myplanet.Init(engine::tools::getAssetPath() + "textures/planets/mars_1k_topo.jpg", 6000, vulkanDevice, &vertexLayout, sceneVertexUniformBuffer, sizeof(uboVS), 0, { &colorMap->m_descriptor }, "planet/planet", "planet/planet", scenepass->GetRenderPass(), pipelineCache, sphereprops, queue, 100, 100);
		rings.Init(6700.0, 6700.0+8000.0, 300, vulkanDevice, &vertexLayout, sceneVertexUniformBuffer, { &ringsMap->m_descriptor, &shadowtex->m_descriptor }, "planet/shadowedplanet", "planet/shadowedplanet", scenepass->GetRenderPass(), pipelineCache, transprops, queue);
		
		shadowobjects.SetVertexLayout(&vertexLayout);
		scene::Geometry* mygeo = new scene::Geometry;
		*mygeo = *rings.m_geometries[0];
		shadowobjects.m_geometries.push_back(mygeo);
		mygeo = new scene::Geometry;
		*mygeo = *saturn.m_geometries[0];
		shadowobjects.m_geometries.push_back(mygeo);
		std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> offscreenbindings
		{
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT}
		};
		shadowobjects.SetDescriptorSetLayout(vulkanDevice->GetDescriptorSetLayout(offscreenbindings));
		shadowobjects.AddDescriptor(vulkanDevice->GetDescriptorSet({ &uniformBufferoffscreen->m_descriptor }, {},
			shadowobjects._descriptorLayout->m_descriptorSetLayout, shadowobjects._descriptorLayout->m_setLayoutBindings));
		
		std::vector<VkVertexInputAttributeDescription> vertexInputAttributes;
		vertexInputAttributes.push_back(VkVertexInputAttributeDescription{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 });
		shadowobjects.AddPipeline(vulkanDevice->GetPipeline(shadowobjects._descriptorLayout->m_descriptorSetLayout, shadowobjects._vertexLayout->m_vertexInputBindings, shadowobjects._vertexLayout->m_vertexInputAttributes,
			engine::tools::getAssetPath() + "shaders/shadowmapping/offscreen.vert.spv", "", offscreenPass->GetRenderPass(), pipelineCache
			, false, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, nullptr, 0, nullptr, true));

		sphereprops.blendEnable = true;
		atmosphere.Init("", 6100, vulkanDevice, &vertexLayout, sceneVertexUniformBuffer, sizeof(uboVS), sizeof(modelUniformAtmosphereFS), { &colorMap->m_descriptor }, "planet/atmosphere", "planet/atmosphere", scenepass->GetRenderPass(), pipelineCache, sphereprops, queue,
		128, 128);

		std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> atmosphereBindings
		{
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}, 
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT}
		};
		render::VulkanDescriptorSetLayout* atmosphereLayout = vulkanDevice->GetDescriptorSetLayout(atmosphereBindings);
		peffpipeline = vulkanDevice->GetPipeline(atmosphereLayout->m_descriptorSetLayout, {}, {},
			engine::tools::getAssetPath() + "shaders/posteffects/screenquad.vert.spv", engine::tools::getAssetPath() + "shaders/planet/atmosphereposteffect.frag.spv",
			mainRenderPass->GetRenderPass(), pipelineCache, false, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

		peffdesc = vulkanDevice->GetDescriptorSet({ &matricesUniformBuffer->m_descriptor, &atmosphere.GetFSUniformBuffer()->m_descriptor }, 
			{ &scenecolor->m_descriptor, &scenepositions->m_descriptor, &scenedepth->m_descriptor, &bluenoise->m_descriptor }, 
			atmosphereLayout->m_descriptorSetLayout, atmosphereLayout->m_setLayoutBindings);

		camera.setTranslationOnSphere(2.0, 0.5, myplanet.GetRadius() + 50.0f);
		/*int stride = terrain._vertexLayout->GetVertexSize(0) / sizeof(float);
		for (auto geo : terrain.m_geometries)
		{
			for (int i = 0; i < geo->m_verticesSize; i += stride)
			{
				glm::vec3 pos = glm::vec3(geo->m_vertices[i], geo->m_vertices[i + 1], geo->m_vertices[i + 2]);
				glm::vec3 normal = glm::vec3(geo->m_vertices[i + 3], geo->m_vertices[i + 4], geo->m_vertices[i + 5]);
				glm::vec3 tangent = glm::vec3(geo->m_vertices[i + 8], geo->m_vertices[i + 9], geo->m_vertices[i + 10]);
				glm::vec3 bitangent = glm::vec3(geo->m_vertices[i + 11], geo->m_vertices[i + 12], geo->m_vertices[i + 13]);
				drawdebugvectors.CreateDebugVectorsGeometry(pos, { tangent, normal ,bitangent }, {glm::vec3(1.0,0.0,0.0),glm::vec3(0.0,1.0,0.0) ,glm::vec3(0.0,0.0,1.0) });
			}
		}
		for (auto geo : drawdebugvectors.m_geometries)
		{
			geo->SetIndexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geo->m_indexCount * sizeof(uint32_t), geo->m_indices));
			geo->SetVertexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geo->m_verticesSize * sizeof(float), geo->m_vertices));
		}
		drawdebugvectors.Init(vulkanDevice, sceneVertexUniformBuffer, queue, mainRenderPass->GetRenderPass(), pipelineCache);*/
	}

	void init()
	{	
		setupGeometry();
		SetupTextures();
		SetupUniforms();
		setupDescriptorPool();
		SetupDescriptors();
		setupPipelines();
		setupSphere();
	}

	void buildCommandBuffers()
	{
		VkCommandBufferBeginInfo cmdBufInfo{};
		cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		for (int32_t i = 0; i < drawCmdBuffers.size(); ++i)
		{
			VK_CHECK_RESULT(vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufInfo));

			offscreenPass->Begin(drawCmdBuffers[i], 0);

			vkCmdSetDepthBias(
				drawCmdBuffers[i],
				depthBiasConstant,
				0.0f,
				depthBiasSlope);

			shadowobjects.Draw(drawCmdBuffers[i]);

			offscreenPass->End(drawCmdBuffers[i]);

			scenepass->Begin(drawCmdBuffers[i], 0);

			sun.Draw(drawCmdBuffers[i]);
			saturn.Draw(drawCmdBuffers[i]);
			rings.Draw(drawCmdBuffers[i]);
			myplanet.Draw(drawCmdBuffers[i]);

			scenepass->End(drawCmdBuffers[i]);

			mainRenderPass->Begin(drawCmdBuffers[i], i);

			peffpipeline->Draw(drawCmdBuffers[i]);
			peffdesc->Draw(drawCmdBuffers[i], peffpipeline->getPipelineLayout(), 0);
			vkCmdDraw(drawCmdBuffers[i], 3, 1, 0, 0);

			drawUI(drawCmdBuffers[i]);

			mainRenderPass->End(drawCmdBuffers[i]);

			VK_CHECK_RESULT(vkEndCommandBuffer(drawCmdBuffers[i]));
		}
	}

	void updateUniformBufferOffscreen()
	{
		// Matrix from light's point of view
		depthProjectionMatrix = glm::perspective(glm::radians(45.0f), 1.0f, 100.0f, 200000.0f);
		glm::vec3 translation = mainplanetmatrix[3];
		depthViewMatrix = glm::lookAt(glm::vec3(0.0f), translation, glm::vec3(0, 1, 0));
				
		glm::mat4 depthModelMatrix = mainplanetmatrix;

		uboOffscreenVS.depthMVP = depthProjectionMatrix * depthViewMatrix * depthModelMatrix;

		memcpy(uniformBufferoffscreen->m_mapped, &uboOffscreenVS, sizeof(uboOffscreenVS));

	}
	
	void updateUniformBuffers()
	{
		uboVS.modelView = glm::mat4(1.0f);
		uniformBufferSunVS->copyTo(&uboVS, sizeof(uboVS));

		mainplanetmatrix = glm::mat4(1.0f);
		
		mainplanetmatrix = glm::rotate(mainplanetmatrix, glm::radians(planetRotation), glm::vec3(0.0f, 1.0f, 0.0f));
		mainplanetmatrix = glm::translate(mainplanetmatrix, glm::vec3(-100000.0f, 0.0f, 0.0f));
		mainplanetmatrix = glm::rotate(mainplanetmatrix, glm::radians(30.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		mainplanetmatrix = glm::rotate(mainplanetmatrix, glm::radians(planetRotation), glm::vec3(0.0f, 1.0f, 0.0f));

		uboVS.modelView = mainplanetmatrix;
		uniformBufferMPVS->copyTo(&uboVS, sizeof(uboVS));
		rings.UpdateUniforms(mainplanetmatrix);
		
		glm::mat4 modelmatrix = glm::rotate(mainplanetmatrix, glm::radians(30.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		modelmatrix = glm::rotate(modelmatrix, glm::radians(planetRotation), glm::vec3(0.0f, 1.0f, 0.0f));
		modelmatrix = glm::translate(modelmatrix, glm::vec3(-30000.0f, 0.0f, 0.0f));
		modelmatrix = glm::rotate(modelmatrix, glm::radians(planetRotation), glm::vec3(0.0f, 1.0f, 0.0f));
		
		//sphere.UpdateUniforms(modelmatrix);
		uboVS.modelView = modelmatrix;
		if(myplanet.GetVSUniformBuffer())
		myplanet.GetVSUniformBuffer()->copyTo(&uboVS, sizeof(uboVS));
		if (atmosphere.GetVSUniformBuffer())
			atmosphere.GetVSUniformBuffer()->copyTo(&uboVS, sizeof(uboVS));
		glm::vec3 center = glm::vec3(4.0, 0.0, 0.0);
		if (atmosphere.GetFSUniformBuffer())
		{		
			modelUniformAtmosphereFS.sun = glm::vec4( glm::vec3(glm::inverse(modelmatrix) * glm::vec4(0.0,0.0,0.0, 1.0f)), sunIntensity);//modelmatrix[3];
			modelUniformAtmosphereFS.cameraPosition = glm::vec4(camera.position, 1.0f);//modelmatrix * glm::vec4(0.0, 0.0, 0.0, 1.0f);
			glm::vec3 vdir = glm::normalize(center - glm::vec3(modelUniformAtmosphereFS.cameraPosition));
			modelUniformAtmosphereFS.viewDirection = glm::vec4(vdir, 0.0);
			modelUniformAtmosphereFS.dimensions = glm::vec4(myplanet.GetRadius(), atmosphere.GetRadius(), rayleighDensity, mieDensity);

			float scatteringStrength = scatteringCoeficient;
			glm::vec3 wavelenghts = glm::vec3(700.0, 530.0, 440.0);
			glm::vec3 scatteringCoefficients = glm::vec3(
				pow(400.0 / wavelenghts.x, 4) * scatteringStrength, //0.1066
				pow(400.0 / wavelenghts.y, 4) * scatteringStrength, //0.324
				pow(400.0 / wavelenghts.z, 4) * scatteringStrength //0.683
			);

			modelUniformAtmosphereFS.scatteringCoefficients = glm::vec4(scatteringCoefficients, 0.021);
			modelUniformAtmosphereFS.distanceFactor = distanceFactor;
			atmosphere.GetFSUniformBuffer()->copyTo(&modelUniformAtmosphereFS, sizeof(modelUniformAtmosphereFS));
		}

		camera.updateViewMatrix(glm::inverse(modelmatrix));

		modelUniformVS.model = glm::mat4(glm::mat3(camera.matrices.view));
		modelSBVertexUniformBuffer->copyTo(&modelUniformVS, sizeof(modelUniformVS));

		glm::mat4 cammat = glm::lookAt(glm::vec3(modelUniformAtmosphereFS.cameraPosition), center, glm::normalize(glm::vec3(-0.3,-0.8,0.0)));

		matricesUniformVS.cameraInvProjection = glm::inverse(camera.matrices.perspective);		
		matricesUniformVS.cameraInvView = glm::inverse(camera.matrices.viewold);
		matricesUniformBuffer->copyTo(&matricesUniformVS, sizeof(matricesUniformVS));

		updateUniformBufferOffscreen();

		uniform_manager.UpdateGlobalParams(scene::UNIFORM_PROJECTION, &camera.matrices.perspective, 0, sizeof(camera.matrices.perspective));
		uniform_manager.UpdateGlobalParams(scene::UNIFORM_VIEW, &camera.matrices.view, 0, sizeof(camera.matrices.view));
		uniform_manager.UpdateGlobalParams(scene::UNIFORM_LIGHT0_POSITION, &light_pos, 0, sizeof(light_pos));
		glm::vec3 cucu = modelmatrix * glm::vec4(camera.position, 1.0f);
		uniform_manager.UpdateGlobalParams(scene::UNIFORM_CAMERA_POSITION, &cucu, 0, sizeof(cucu));
		glm::mat4 lightspace = depthProjectionMatrix * depthViewMatrix;
		uniform_manager.UpdateGlobalParams(scene::UNIFORM_LIGHT0_SPACE, &lightspace, 0, sizeof(lightspace));

		uniform_manager.Update();
	}

	void draw()
	{
		VulkanExampleBase::prepareFrame();

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &drawCmdBuffers[currentBuffer];
		VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

		VulkanExampleBase::submitFrame();
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
		planetRotation += 10.0f * frameTimer;
		if (planetRotation > 360.0f)
			planetRotation -= 360.0f;
		updateUniformBuffers();
	}

	virtual void viewChanged()
	{
		updateUniformBuffers();
	}

	virtual void OnUpdateUIOverlay(engine::scene::UIOverlay *overlay)
	{
		if (overlay->header("Settings")) {
			if (ImGui::SliderFloat("Rayleigh density", &rayleighDensity, 0.0f, 10.0f))
			{
				updateUniformBuffers();
			}
			if (ImGui::SliderFloat("Mie density", &mieDensity, 0.0f, 2.0f))
			{
				updateUniformBuffers();
			}
			if (ImGui::SliderFloat("Scattering coeficient", &scatteringCoeficient, 0.0f, 50.0f))
			{
				updateUniformBuffers();
			}
			if (ImGui::SliderFloat("Distance factor", &distanceFactor, 0.0001f, 1.0f))
			{
				updateUniformBuffers();
			}
			if (ImGui::SliderFloat("Sun intensity", &sunIntensity, 0.0f, 100.0f))
			{
				updateUniformBuffers();
			}
		}
	}

};

VULKAN_EXAMPLE_MAIN()