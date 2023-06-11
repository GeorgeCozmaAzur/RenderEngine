
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
	
		glm::vec4	sun;//poistion and intensity of the sun
	//float I_sun;    // Intensity of the sun
		glm::vec4	cameraPosition;
		glm::vec4	viewDirection;
		glm::vec4	dimensions;//radius of the planet, radius of the atmosphere, Rayleigh scale height, Mie scale height
		//float R_e;      // Radius of the planet [m]
		//float R_a;      // Radius of the atmosphere [m]
		glm::vec4	scatteringCoefficients;//Rayleigh and Mie scattering coefficiants
		//vec3  beta_R;   // Rayleigh scattering coefficient
		//float beta_M;   // Mie scattering coefficient
		//float H_R;      // Rayleigh scale height
		//float H_M;      // Mie scale height
		//float g;        // Mie scattering direction - 
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

	float planetrotation = 0.0f;

	float h0 = 7.994;

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
		camera.setPerspective(60.0f, (float)width / (float)height, 0.1f, 20048.0f);
		camera.setRotation(glm::vec3(0.0f, 0.0f, 0.0f));
		camera.setTranslation(glm::vec3(0.0f, 3.2f, 0.0f));
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
		saturn.LoadGeometry(engine::tools::getAssetPath() + "models/geosphere.obj", &vertexLayout, 30.0f, 1);
		for (auto geo : saturn.m_geometries)
		{
			geo->SetIndexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geo->m_indexCount * sizeof(uint32_t), geo->m_indices));
			geo->SetVertexBuffer(vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, queue, geo->m_verticesSize * sizeof(float), geo->m_vertices),true);
		}
		sun.LoadGeometry(engine::tools::getAssetPath() + "models/geosphere.obj", &vertexLayout, 10.0f, 1, glm::vec3(0.0,0,0.0));
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
		render::VulkanTexture* scenedepth = vulkanDevice->GetDepthRenderTarget(width, height, false);

		scenepass = vulkanDevice->GetRenderPass({ scenecolor , scenedepth }, 0);
		render::VulkanFrameBuffer* fb = vulkanDevice->GetFrameBuffer(scenepass->GetRenderPass(), width, height, { scenecolor->m_descriptor.imageView, scenedepth->m_descriptor.imageView }, { { 0.0f, 0.0f, 0.0f, 1.0f } });
		scenepass->AddFrameBuffer(fb);

		colorMap = vulkanDevice->GetTexture(engine::tools::getAssetPath() + "textures/planets/marsmap1k.jpg", VK_FORMAT_R8G8B8A8_UNORM, queue);
		sunMap = vulkanDevice->GetTexture(engine::tools::getAssetPath() + "textures/planets/2k_sun.jpg", VK_FORMAT_R8G8B8A8_UNORM, queue);
		ringsMap = vulkanDevice->GetTexture(engine::tools::getAssetPath() + "textures/planets/2k_saturn_ring_alpha.png", VK_FORMAT_R8G8B8A8_UNORM, queue);
		saturnMap = vulkanDevice->GetTexture(engine::tools::getAssetPath() + "textures/planets/2k_saturn.jpg", VK_FORMAT_R8G8B8A8_UNORM, queue);
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

		updateUniformBuffers();
	}

	//here a descriptor pool will be created for the entire app. Now it contains 1 sampler because this is what the ui overlay needs
	void setupDescriptorPool()
	{
		std::vector<VkDescriptorPoolSize> poolSizes = {
			engine::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 17),
			engine::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10)
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
		/*terrain.AddPipeline(vulkanDevice->GetPipeline(terrain._descriptorLayout->m_descriptorSetLayout, vertexLayoutNM.m_vertexInputBindings, vertexLayoutNM.m_vertexInputAttributes,
			engine::tools::getAssetPath() + "shaders/basic/normalmap.vert.spv", engine::tools::getAssetPath() + "shaders/basic/normalmap.frag.spv", mainRenderPass->GetRenderPass(), pipelineCache));*/
		scenepass->SetClearColor({ 0.0f, 0.0f, 0.0f, 0.0f }, 0);
		sun.AddPipeline(vulkanDevice->GetPipeline(sun._descriptorLayout->m_descriptorSetLayout, sun._vertexLayout->m_vertexInputBindings, sun._vertexLayout->m_vertexInputAttributes,
			engine::tools::getAssetPath() + "shaders/planet/planet.vert.spv", engine::tools::getAssetPath() + "shaders/planet/sun.frag.spv", scenepass->GetRenderPass(), pipelineCache));

		saturn.AddPipeline(vulkanDevice->GetPipeline(saturn._descriptorLayout->m_descriptorSetLayout, saturn._vertexLayout->m_vertexInputBindings, saturn._vertexLayout->m_vertexInputAttributes,
			engine::tools::getAssetPath() + "shaders/planet/planet.vert.spv", engine::tools::getAssetPath() + "shaders/planet/planet.frag.spv", scenepass->GetRenderPass(), pipelineCache));

		/*skybox.AddPipeline(vulkanDevice->GetPipeline(skybox._descriptorLayout->m_descriptorSetLayout, simpleVertexLayout.m_vertexInputBindings, simpleVertexLayout.m_vertexInputAttributes,
			engine::tools::getAssetPath() + "shaders/basic/skybox.vert.spv", engine::tools::getAssetPath() + "shaders/basic/skybox.frag.spv", scenepass->GetRenderPass(), pipelineCache));*/
	
	}

	void setupSphere()
	{
		render::PipelineProperties sphereprops;
		sphereprops.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		myplanet.Init(engine::tools::getAssetPath() + "textures/planets/mars_1k_topo.jpg", 600, vulkanDevice, &vertexLayout, sceneVertexUniformBuffer, sizeof(uboVS), 0, { &colorMap->m_descriptor }, "planet/planet", "planet/planet", scenepass->GetRenderPass(), pipelineCache, sphereprops, queue);
		rings.Init(670.0, 670.0+800.0, 300, vulkanDevice, &vertexLayout, sceneVertexUniformBuffer, { &ringsMap->m_descriptor, &shadowtex->m_descriptor }, "planet/shadowedplanet", "planet/shadowedplanet", scenepass->GetRenderPass(), pipelineCache, queue);
		
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
		vertexInputAttributes.push_back(engine::initializers::vertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0));
		shadowobjects.AddPipeline(vulkanDevice->GetPipeline(shadowobjects._descriptorLayout->m_descriptorSetLayout, shadowobjects._vertexLayout->m_vertexInputBindings, shadowobjects._vertexLayout->m_vertexInputAttributes,
			engine::tools::getAssetPath() + "shaders/shadowmapping/offscreen.vert.spv", "", offscreenPass->GetRenderPass(), pipelineCache//);
			, false, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, nullptr, 0, nullptr, true));

		sphereprops.blendEnable = true;
		atmosphere.Init("", 630, vulkanDevice, &vertexLayout, sceneVertexUniformBuffer, sizeof(uboVS), sizeof(modelUniformAtmosphereFS), { &colorMap->m_descriptor }, "planet/atmosphere", "planet/atmosphere", scenepass->GetRenderPass(), pipelineCache, sphereprops, queue,
		128, 128);

		std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> blurbindings
		{
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
		};
		render::VulkanDescriptorSetLayout* blur_layout = vulkanDevice->GetDescriptorSetLayout(blurbindings);
		peffpipeline = vulkanDevice->GetPipeline(blur_layout->m_descriptorSetLayout, {}, {},
			engine::tools::getAssetPath() + "shaders/posteffects/screenquad.vert.spv", engine::tools::getAssetPath() + "shaders/posteffects/simpletexture.frag.spv",
			mainRenderPass->GetRenderPass(), pipelineCache, false, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

		peffdesc = vulkanDevice->GetDescriptorSet({}, { &scenecolor->m_descriptor }, blur_layout->m_descriptorSetLayout, blur_layout->m_setLayoutBindings);

		camera.setTranslation(glm::vec3(0.0f, myplanet.GetRadius() + 8.0f, 0.0f));
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
		VkCommandBufferBeginInfo cmdBufInfo = engine::initializers::commandBufferBeginInfo();

		for (int32_t i = 0; i < drawCmdBuffers.size(); ++i)
		{
			VK_CHECK_RESULT(vkBeginCommandBuffer(drawCmdBuffers[i], &cmdBufInfo));

			offscreenPass->Begin(drawCmdBuffers[i], 0);

			// Set depth bias (aka "Polygon offset")
			// Required to avoid shadow mapping artefacts
			vkCmdSetDepthBias(
				drawCmdBuffers[i],
				depthBiasConstant,
				0.0f,
				depthBiasSlope);

			shadowobjects.Draw(drawCmdBuffers[i]);

			offscreenPass->End(drawCmdBuffers[i]);


			scenepass->Begin(drawCmdBuffers[i], 0);

			//draw here
			//skybox.Draw(drawCmdBuffers[i]);
			sun.Draw(drawCmdBuffers[i]);
			saturn.Draw(drawCmdBuffers[i]);
			rings.Draw(drawCmdBuffers[i]);
			myplanet.Draw(drawCmdBuffers[i]);
			atmosphere.Draw(drawCmdBuffers[i]);
			//drawdebugvectors.Draw(drawCmdBuffers[i]);

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
		depthProjectionMatrix = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 20048.0f);
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
		
		mainplanetmatrix = glm::rotate(mainplanetmatrix, glm::radians(planetrotation), glm::vec3(0.0f, 1.0f, 0.0f));
		mainplanetmatrix = glm::translate(mainplanetmatrix, glm::vec3(-10000.0f, 0.0f, 0.0f));
		mainplanetmatrix = glm::rotate(mainplanetmatrix, glm::radians(30.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		mainplanetmatrix = glm::rotate(mainplanetmatrix, glm::radians(planetrotation), glm::vec3(0.0f, 1.0f, 0.0f));

		uboVS.modelView = mainplanetmatrix;
		uniformBufferMPVS->copyTo(&uboVS, sizeof(uboVS));
		rings.UpdateUniforms(mainplanetmatrix);
		
		glm::mat4 modelmatrix = glm::rotate(mainplanetmatrix, glm::radians(30.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		modelmatrix = glm::rotate(modelmatrix, glm::radians(planetrotation), glm::vec3(0.0f, 1.0f, 0.0f));
		modelmatrix = glm::translate(modelmatrix, glm::vec3(-3000.0f, 0.0f, 0.0f));
		modelmatrix = glm::rotate(modelmatrix, glm::radians(planetrotation), glm::vec3(0.0f, 1.0f, 0.0f));
		
		//sphere.UpdateUniforms(modelmatrix);
		uboVS.modelView = modelmatrix;
		if(myplanet.GetVSUniformBuffer())
		myplanet.GetVSUniformBuffer()->copyTo(&uboVS, sizeof(uboVS));
		if (atmosphere.GetVSUniformBuffer())
			atmosphere.GetVSUniformBuffer()->copyTo(&uboVS, sizeof(uboVS));
		if (atmosphere.GetFSUniformBuffer())
		{		
			modelUniformAtmosphereFS.sun = glm::vec4( glm::vec3(glm::inverse(modelmatrix) * glm::vec4(0.0,0.0,0.0, 1.0f)), 20.0f);//modelmatrix[3];
			modelUniformAtmosphereFS.cameraPosition = glm::vec4(camera.position, 1.0f);//modelmatrix * glm::vec4(0.0, 0.0, 0.0, 1.0f);
			modelUniformAtmosphereFS.viewDirection = glm::vec4(camera.camWorldFront, 1.0f);//modelmatrix * glm::vec4(0.0, 0.0, 0.0, 1.0f);
			modelUniformAtmosphereFS.dimensions = glm::vec4(myplanet.GetRadius(), atmosphere.GetRadius(), h0, 1.200);
			modelUniformAtmosphereFS.scatteringCoefficients = glm::vec4(5.8e-3f, 13.5e-3f, 33.1e-3f, 21e-3f);
			//modelUniformAtmosphereFS.g = 0.888;
			atmosphere.GetFSUniformBuffer()->copyTo(&modelUniformAtmosphereFS, sizeof(modelUniformAtmosphereFS));
		}

		camera.updateViewMatrix(glm::inverse(modelmatrix));

		modelUniformVS.model = glm::mat4(glm::mat3(camera.matrices.view));
		modelSBVertexUniformBuffer->copyTo(&modelUniformVS, sizeof(modelUniformVS));

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
		planetrotation += 10.0f * frameTimer;
		if (planetrotation > 360.0f)
			planetrotation -= 360.0f;
		updateUniformBuffers();
	}

	virtual void viewChanged()
	{
		updateUniformBuffers();
	}

	virtual void OnUpdateUIOverlay(engine::scene::UIOverlay *overlay)
	{
		if (overlay->header("Settings")) {
			if (ImGui::SliderFloat("Rayleigh scale height", &h0, 0.0f, 20.0f))
			{
			}
		}
	}

};

VULKAN_EXAMPLE_MAIN()