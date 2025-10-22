
#include "vulkan/vulkan.h"
#include "VulkanApplication.h"
#include "VulkanTools.h"
#include "render/vulkan/VulkanDevice.h"
#include "scene/SimpleModelRT.h"
#include "render/vulkan/VulkanCommandBuffer.h"

class VulkanExample : public VulkanApplication
{
public:
	// Function pointers for ray tracing related stuff
	PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddressKHR;
	PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;
	PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR;
	PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR;
	PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR;
	PFN_vkBuildAccelerationStructuresKHR vkBuildAccelerationStructuresKHR;
	PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR;
	PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR;
	PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR;
	PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR;

	// Available features and properties
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR  rayTracingPipelineProperties{};
	VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{};

	// Enabled features and properties
	VkPhysicalDeviceBufferDeviceAddressFeatures enabledBufferDeviceAddresFeatures{};
	VkPhysicalDeviceRayTracingPipelineFeaturesKHR enabledRayTracingPipelineFeatures{};
	VkPhysicalDeviceAccelerationStructureFeaturesKHR enabledAccelerationStructureFeatures{};

	// Holds information for a ray tracing scratch buffer that is used as a temporary storage
	struct ScratchBuffer
	{
		uint64_t deviceAddress = 0;
		render::VulkanBuffer* buffer;
		//VkBuffer handle = VK_NULL_HANDLE;
		//VkDeviceMemory memory = VK_NULL_HANDLE;
	};

	// Holds information for a ray tracing acceleration structure
	struct AccelerationStructure {
		VkAccelerationStructureKHR handle;
		uint64_t deviceAddress = 0;
		render::VulkanBuffer* buffer;
		//VkDeviceMemory memory;
		//VkBuffer buffer;
	};

	// Holds information for a storage image that the ray tracing shaders output to
	render::VulkanTexture* storageImage = nullptr;

	struct ShaderBindingTable {
		render::VulkanBuffer* vbuffer;
		VkStridedDeviceAddressRegionKHR stridedDeviceAddressRegion;
	};

	std::vector<AccelerationStructure> bottomLevelASs;
	AccelerationStructure topLevelAS;

	std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups{};
	struct ShaderBindingTables {
		ShaderBindingTable raygen;
		ShaderBindingTable miss;
		ShaderBindingTable hit;
	} shaderBindingTables;

	struct UniformData {
		glm::mat4 viewInverse;
		glm::mat4 projInverse;
		glm::vec4 lightPos;
		int32_t vertexSize;
	} uniformData;
	render::VulkanBuffer* ubo;

	std::vector<VkShaderModule> m_shaderModules;

	VkDescriptorPool descriptorPool;
	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;
	VkDescriptorSet descriptorSet;
	VkDescriptorSetLayout descriptorSetLayout;

	std::vector<render::VulkanTexture*> alltextures;

	engine::scene::SimpleModelRT scene;

	render::VulkanVertexLayout vertexLayoutRT = render::VulkanVertexLayout({
		render::VERTEX_COMPONENT_POSITION,
		render::VERTEX_COMPONENT_NORMAL,
		render::VERTEX_COMPONENT_UV,
		render::VERTEX_COMPONENT_DUMMY_FLOAT,
		render::VERTEX_COMPONENT_TANGENT,
		render::VERTEX_COMPONENT_DUMMY_VEC4
		}, {});

	std::vector<scene::GeometryRT::MaterialObj> m_materials;
	std::vector<int32_t>     m_matIndx;

	// Information of a obj model when referenced in a shader
	struct ObjDesc
	{
		int      txtOffset;             // Texture index offset in the array of textures
		uint64_t vertexAddress;         // Address of the Vertex buffer
		uint64_t indexAddress;          // Address of the index buffer
		uint64_t materialAddress;       // Address of the material buffer
		uint64_t materialIndexAddress;  // Address of the triangle material index buffer
	};
	std::vector<ObjDesc>     m_objDesc;    // Model description for device access
	render::VulkanBuffer* m_bObjDesc;

	void enableExtensions()
	{
		// Require Vulkan 1.1
		apiVersion = VK_API_VERSION_1_2;

		// Ray tracing related extensions required by this sample
		enabledDeviceExtensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
		//if (!rayQueryOnly) {
			enabledDeviceExtensions.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
		//}

		// Required by VK_KHR_acceleration_structure
		enabledDeviceExtensions.push_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
		//if (!rayQueryOnly) {
			enabledDeviceExtensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
		//}
		enabledDeviceExtensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);

		// Required for VK_KHR_ray_tracing_pipeline
		enabledDeviceExtensions.push_back(VK_KHR_SPIRV_1_4_EXTENSION_NAME);

		// Required by VK_KHR_spirv_1_4
		enabledDeviceExtensions.push_back(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);
	}

	// This sample is derived from an extended base class that saves most of the ray tracing setup boiler plate
	VulkanExample() : VulkanApplication(true)
	{
		title = "Ray traced textured models with shadows";
		timerSpeed *= 0.25f;
		camera.SetPerspective(60.0f, (float)width / (float)height, 0.1f, 512.0f);
		camera.SetRotation(glm::vec3(0.0f, 0.0f, 0.0f));
		camera.SetPosition(glm::vec3(0.0f, 0.5f, -1.5f));
		enableExtensions();
	}

	void deleteScratchBuffer(ScratchBuffer& scratchBuffer)
	{
		vulkanDevice->DestroyBuffer(scratchBuffer.buffer);
	}
	void deleteAccelerationStructure(AccelerationStructure& accelerationStructure)
	{
		vulkanDevice->DestroyBuffer(accelerationStructure.buffer);
		vkDestroyAccelerationStructureKHR(device, accelerationStructure.handle, nullptr);
	}
	void deleteStorageImage()
	{
		vulkanDevice->DestroyTexture(storageImage);
	}

	~VulkanExample()
	{
		vkDestroyPipeline(device, pipeline, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
		deleteStorageImage();
		for(auto blas : bottomLevelASs)
		deleteAccelerationStructure(blas);
		deleteAccelerationStructure(topLevelAS);
		vulkanDevice->DestroyBuffer(ubo);
		for (auto& shaderModule : m_shaderModules)
		{
			vkDestroyShaderModule(device, shaderModule, nullptr);
		}
	}

	uint32_t alignedSize(uint32_t value, uint32_t alignment)
	{
		return (value + alignment - 1) & ~(alignment - 1);
	}
	VkPipelineShaderStageCreateInfo loadShader(std::string fileName, VkShaderStageFlagBits stage)
	{
		VkPipelineShaderStageCreateInfo shaderStage = {};
		shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStage.stage = stage;
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
		shaderStage.module = vks::tools::loadShader(androidApp->activity->assetManager, fileName.c_str(), device);
#else
		shaderStage.module = engine::tools::loadShader(fileName.c_str(), device);
#endif
		shaderStage.pName = "main";
		assert(shaderStage.module != VK_NULL_HANDLE);
		m_shaderModules.push_back(shaderStage.module);
		return shaderStage;
	}

	ScratchBuffer createScratchBuffer(VkDeviceSize size)
	{
		ScratchBuffer scratchBuffer{};
		scratchBuffer.buffer = vulkanDevice->GetBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, size);

		VkBufferDeviceAddressInfoKHR bufferDeviceAddresInfo{};
		bufferDeviceAddresInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		bufferDeviceAddresInfo.buffer = scratchBuffer.buffer->GetVkBuffer();
		scratchBuffer.deviceAddress = vkGetBufferDeviceAddressKHR(vulkanDevice->logicalDevice, &bufferDeviceAddresInfo);
		return scratchBuffer;
	}

	void createAccelerationStructure(AccelerationStructure& accelerationStructure, VkAccelerationStructureTypeKHR type, VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo)
	{
		accelerationStructure.buffer = vulkanDevice->GetBuffer(VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buildSizeInfo.accelerationStructureSize);
		// Acceleration structure
		VkAccelerationStructureCreateInfoKHR accelerationStructureCreate_info{};
		accelerationStructureCreate_info.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		accelerationStructureCreate_info.buffer = accelerationStructure.buffer->GetVkBuffer();
		accelerationStructureCreate_info.size = buildSizeInfo.accelerationStructureSize;
		accelerationStructureCreate_info.type = type;
		vkCreateAccelerationStructureKHR(vulkanDevice->logicalDevice, &accelerationStructureCreate_info, nullptr, &accelerationStructure.handle);
		// AS device address
		VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
		accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
		accelerationDeviceAddressInfo.accelerationStructure = accelerationStructure.handle;
		accelerationStructure.deviceAddress = vkGetAccelerationStructureDeviceAddressKHR(vulkanDevice->logicalDevice, &accelerationDeviceAddressInfo);
	}

	void createStorageImage(VkFormat format, VkExtent3D extent)
	{
		// Release ressources if image is to be recreated
		if (storageImage && storageImage->m_vkImage != VK_NULL_HANDLE) {
			vulkanDevice->DestroyTexture(storageImage);
		}

		storageImage = vulkanDevice->GetRenderTarget(extent.width, extent.height, format, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED);

		VkCommandBuffer cmdBuffer = vulkanDevice->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
		storageImage->ChangeLayout(cmdBuffer,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_GENERAL);
		vulkanDevice->FlushCommandBuffer(cmdBuffer, queue);
	}

	uint64_t getBufferDeviceAddress(VkBuffer buffer)
	{
		VkBufferDeviceAddressInfoKHR bufferDeviceAI{};
		bufferDeviceAI.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		bufferDeviceAI.buffer = buffer;
		return vkGetBufferDeviceAddressKHR(vulkanDevice->logicalDevice, &bufferDeviceAI);
	}

	VkStridedDeviceAddressRegionKHR getSbtEntryStridedDeviceAddressRegion(VkBuffer buffer, uint32_t handleCount)
	{
		const uint32_t handleSizeAligned = alignedSize(rayTracingPipelineProperties.shaderGroupHandleSize, rayTracingPipelineProperties.shaderGroupHandleAlignment);
		VkStridedDeviceAddressRegionKHR stridedDeviceAddressRegionKHR{};
		stridedDeviceAddressRegionKHR.deviceAddress = getBufferDeviceAddress(buffer);
		stridedDeviceAddressRegionKHR.stride = handleSizeAligned;
		stridedDeviceAddressRegionKHR.size = handleCount * handleSizeAligned;
		return stridedDeviceAddressRegionKHR;
	}

	void createShaderBindingTable(ShaderBindingTable& shaderBindingTable, uint32_t handleCount)
	{
		// Create buffer to hold all shader handles for the SBT
		const VkBufferUsageFlags bufferUsageFlags = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
		const VkMemoryPropertyFlags memoryUsageFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		shaderBindingTable.vbuffer = vulkanDevice->GetBuffer(bufferUsageFlags, memoryUsageFlags, rayTracingPipelineProperties.shaderGroupHandleSize * handleCount);
		// Get the strided address to be used when dispatching the rays
		VkStridedDeviceAddressRegionKHR bla = getSbtEntryStridedDeviceAddressRegion(shaderBindingTable.vbuffer->GetVkBuffer(), handleCount);
		shaderBindingTable.stridedDeviceAddressRegion.deviceAddress = bla.deviceAddress;
		shaderBindingTable.stridedDeviceAddressRegion.size = bla.size;
		shaderBindingTable.stridedDeviceAddressRegion.stride = bla.stride;
		// Map persistent 
		shaderBindingTable.vbuffer->Map();
	}

	void createTextures()
	{
		//alltextures.push_back(vulkanDevice->GetTexture(engine::tools::getAssetPath() + "textures/tilesWood0005_2_S.jpg", VK_FORMAT_R8G8B8A8_UNORM, queue));
	
		for (auto geo : scene.m_geometriesRT)
		{
			scene::GeometryRT* geoRT = (scene::GeometryRT*)geo;
			//for (int i = 0; i < geoRT->m_textures.size();i++)
			if (geoRT->m_textures.size() > 0)
				for (int i = 0; i < geoRT->m_textures.size();i++)
				{
					std::string textureName = "textures/" + geoRT->m_textures[i];
					render::Texture2DData data;
					data.LoadFromFile(engine::tools::getAssetPath() + textureName, render::GfxFormat::R8G8B8A8_UNORM);
					alltextures.push_back(vulkanDevice->GetTexture(&data, queue));
					//alltextures.push_back(vulkanDevice->GetTexture(engine::tools::getAssetPath() + shit, VK_FORMAT_R8G8B8A8_UNORM, queue));
				}
		}
	}
	/*
		Create the bottom level acceleration structure contains the scene's actual geometry (vertices, triangles)
	*/
	void createBottomLevelAccelerationStructure()
	{
		scene.LoadGeometry(engine::tools::getAssetPath() + "models/Medieval_building/Medieval_building.obj", &vertexLayoutRT, 0.1f, 1);
		//scene.LoadGeometry(engine::tools::getAssetPath() + "models/CornellBox-Original.obj", &vertexLayoutRT, 0.5f, 1);
		scene.LoadGeometry(engine::tools::getAssetPath() + "models/plane.obj", &vertexLayoutRT, 0.5f, 1);
		/*scene::GeometryRT* geoRT = (scene::GeometryRT*)scene.m_geometries[1];
		geoRT->m_materials[0].textureID = 0;
		geoRT->m_textures.push_back("compass.jpg");*/
		int txtOffset = 0;
		for (auto geo : scene.m_geometriesRT)
		{
			scene::GeometryRT* geoRT = (scene::GeometryRT*)geo;

			geo->SetIndexBuffer(vulkanDevice->GetBuffer(VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				geo->m_indexCount * sizeof(uint32_t), geo->m_indices

			));
			geo->SetVertexBuffer(vulkanDevice->GetBuffer(VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				geo->m_verticesSize * sizeof(float), geo->m_vertices)
			);

			
			geoRT->materialsBuffer = vulkanDevice->GetBuffer(VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				geoRT->m_materials.size() * sizeof(scene::GeometryRT::MaterialObj), geoRT->m_materials.data()
			);
			geoRT->materialsIndexBuffer = vulkanDevice->GetBuffer(VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				geoRT->m_matIndx.size() * sizeof(int32_t), geoRT->m_matIndx.data()
			);

			ObjDesc desc;
			desc.txtOffset = txtOffset;
			txtOffset += static_cast<int>(geoRT->m_textures.size());
			desc.vertexAddress = getBufferDeviceAddress(geo->_vertexBuffer->GetVkBuffer());
			desc.indexAddress = getBufferDeviceAddress(geo->_indexBuffer->GetVkBuffer());
			desc.materialAddress = getBufferDeviceAddress(geoRT->materialsBuffer->GetVkBuffer());
			desc.materialIndexAddress = getBufferDeviceAddress(geoRT->materialsIndexBuffer->GetVkBuffer());
			m_objDesc.emplace_back(desc);

			VkDeviceOrHostAddressConstKHR vertexBufferDeviceAddress{};
			VkDeviceOrHostAddressConstKHR indexBufferDeviceAddress{};

			vertexBufferDeviceAddress.deviceAddress = getBufferDeviceAddress(geo->_vertexBuffer->GetVkBuffer());
			indexBufferDeviceAddress.deviceAddress = getBufferDeviceAddress(geo->_indexBuffer->GetVkBuffer());

			uint32_t numTriangles = geo->m_indexCount / 3;
			uint32_t maxVertex = static_cast<uint32_t>(geo->m_vertexCount);

			// Build
			//VkAccelerationStructureGeometryKHR accelerationStructureGeometry = engine::initializers::accelerationStructureGeometryKHR();
			VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
			accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
			accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
			accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
			accelerationStructureGeometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
			accelerationStructureGeometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
			accelerationStructureGeometry.geometry.triangles.vertexData = vertexBufferDeviceAddress;
			accelerationStructureGeometry.geometry.triangles.maxVertex = maxVertex;
			accelerationStructureGeometry.geometry.triangles.vertexStride = vertexLayoutRT.GetVertexSize(0);
			accelerationStructureGeometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
			accelerationStructureGeometry.geometry.triangles.indexData = indexBufferDeviceAddress;
			accelerationStructureGeometry.geometry.triangles.transformData.deviceAddress = 0;
			accelerationStructureGeometry.geometry.triangles.transformData.hostAddress = nullptr;

			// Get size info
			//VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo = engine::initializers::accelerationStructureBuildGeometryInfoKHR();
			VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
			accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
			accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
			accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
			accelerationStructureBuildGeometryInfo.geometryCount = 1;
			accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;

			//VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo = engine::initializers::accelerationStructureBuildSizesInfoKHR();
			VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
			accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
			vkGetAccelerationStructureBuildSizesKHR(
				device,
				VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
				&accelerationStructureBuildGeometryInfo,
				&numTriangles,
				&accelerationStructureBuildSizesInfo);

			AccelerationStructure blas;
			createAccelerationStructure(blas, VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR, accelerationStructureBuildSizesInfo);

			// Create a small scratch buffer used during build of the bottom level acceleration structure
			ScratchBuffer scratchBuffer = createScratchBuffer(accelerationStructureBuildSizesInfo.buildScratchSize);

			//VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo = engine::initializers::accelerationStructureBuildGeometryInfoKHR();
			VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{};
			accelerationBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
			accelerationBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
			accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
			accelerationBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
			accelerationBuildGeometryInfo.dstAccelerationStructure = blas.handle;
			accelerationBuildGeometryInfo.geometryCount = 1;
			accelerationBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
			accelerationBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer.deviceAddress;

			VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo{};
			accelerationStructureBuildRangeInfo.primitiveCount = numTriangles;
			accelerationStructureBuildRangeInfo.primitiveOffset = 0;
			accelerationStructureBuildRangeInfo.firstVertex = 0;
			accelerationStructureBuildRangeInfo.transformOffset = 0;
			std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = { &accelerationStructureBuildRangeInfo };

			// Build the acceleration structure on the device via a one-time command buffer submission
			// Some implementations may support acceleration structure building on the host (VkPhysicalDeviceAccelerationStructureFeaturesKHR->accelerationStructureHostCommands), but we prefer device builds
			VkCommandBuffer commandBuffer = vulkanDevice->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
			vkCmdBuildAccelerationStructuresKHR(
				commandBuffer,
				1,
				&accelerationBuildGeometryInfo,
				accelerationBuildStructureRangeInfos.data());
			vulkanDevice->FlushCommandBuffer(commandBuffer, queue);

			deleteScratchBuffer(scratchBuffer);

			bottomLevelASs.push_back(blas);

		}

		VkDeviceSize bufferSize = m_objDesc.size() * sizeof(ObjDesc);
		m_bObjDesc = vulkanDevice->GetBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
											VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			bufferSize, nullptr);

		render::VulkanBuffer* staging = vulkanDevice->CreateStagingBuffer(bufferSize, m_objDesc.data());
		VkBufferCopy bufferCopy{};
		bufferCopy.size = bufferSize;
		VkCommandBuffer copyCmd = vulkanDevice->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
		vkCmdCopyBuffer(copyCmd, staging->GetVkBuffer(), m_bObjDesc->GetVkBuffer(), 1, &bufferCopy);
		vulkanDevice->FlushCommandBuffer(copyCmd, queue, true);
	}

	/*
		The top level acceleration structure contains the scene's object instances
	*/
	void createTopLevelAccelerationStructure()
	{
		VkTransformMatrixKHR transformMatrix = {
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f };
		VkTransformMatrixKHR othertransformMatrix = {
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f };

		std::vector<VkAccelerationStructureInstanceKHR> instances;
		instances.resize(bottomLevelASs.size());
		for (int i = 0; i< bottomLevelASs.size();i++)
		{	
			instances[i].transform = i==0 ? transformMatrix : othertransformMatrix;
			instances[i].instanceCustomIndex = 0;
			instances[i].mask = 0xFF;
			instances[i].instanceShaderBindingTableRecordOffset = 0;
			instances[i].flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
			instances[i].accelerationStructureReference = bottomLevelASs[i].deviceAddress;
		}

		// Buffer for instance data
		render::VulkanBuffer* instancesBuffer;
		instancesBuffer = vulkanDevice->GetBuffer(VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			sizeof(VkAccelerationStructureInstanceKHR) * instances.size(), instances.data());

		VkDeviceOrHostAddressConstKHR instanceDataDeviceAddress{};
		instanceDataDeviceAddress.deviceAddress = getBufferDeviceAddress(instancesBuffer->GetVkBuffer());

		//VkAccelerationStructureGeometryKHR accelerationStructureGeometry = engine::initializers::accelerationStructureGeometryKHR();
		VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
		accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
		accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
		accelerationStructureGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
		accelerationStructureGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
		accelerationStructureGeometry.geometry.instances.data = instanceDataDeviceAddress;

		// Get size info
		//VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo = engine::initializers::accelerationStructureBuildGeometryInfoKHR();
		VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
		accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		accelerationStructureBuildGeometryInfo.geometryCount = 1;
		accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;

		uint32_t primitive_count = static_cast<uint32_t>(bottomLevelASs.size());

		//VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo = engine::initializers::accelerationStructureBuildSizesInfoKHR();
		VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
		accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
		vkGetAccelerationStructureBuildSizesKHR(
			device,
			VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
			&accelerationStructureBuildGeometryInfo,
			&primitive_count,
			&accelerationStructureBuildSizesInfo);

		// @todo: as return value?
		createAccelerationStructure(topLevelAS, VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR, accelerationStructureBuildSizesInfo);

		// Create a small scratch buffer used during build of the top level acceleration structure
		ScratchBuffer scratchBuffer = createScratchBuffer(accelerationStructureBuildSizesInfo.buildScratchSize);

		//VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo = engine::initializers::accelerationStructureBuildGeometryInfoKHR();
		VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{};
		accelerationBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		accelerationBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		accelerationBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
		accelerationBuildGeometryInfo.dstAccelerationStructure = topLevelAS.handle;
		accelerationBuildGeometryInfo.geometryCount = 1;
		accelerationBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
		accelerationBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer.deviceAddress;

		VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo{};
		accelerationStructureBuildRangeInfo.primitiveCount = static_cast<uint32_t>(bottomLevelASs.size());
		accelerationStructureBuildRangeInfo.primitiveOffset = 0;
		accelerationStructureBuildRangeInfo.firstVertex = 0;
		accelerationStructureBuildRangeInfo.transformOffset = 0;
		std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = { &accelerationStructureBuildRangeInfo };

		// Build the acceleration structure on the device via a one-time command buffer submission
		// Some implementations may support acceleration structure building on the host (VkPhysicalDeviceAccelerationStructureFeaturesKHR->accelerationStructureHostCommands), but we prefer device builds
		VkCommandBuffer commandBuffer = vulkanDevice->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
		vkCmdBuildAccelerationStructuresKHR(
			commandBuffer,
			1,
			&accelerationBuildGeometryInfo,
			accelerationBuildStructureRangeInfos.data());
		vulkanDevice->FlushCommandBuffer(commandBuffer, queue);

		deleteScratchBuffer(scratchBuffer);
		//instancesBuffer.destroy();
		vulkanDevice->DestroyBuffer(instancesBuffer);
	}


	/*
		Create the Shader Binding Tables that binds the programs and top-level acceleration structure
	*/
	void createShaderBindingTables() {
		const uint32_t handleSize = rayTracingPipelineProperties.shaderGroupHandleSize;
		const uint32_t handleSizeAligned = alignedSize(rayTracingPipelineProperties.shaderGroupHandleSize, rayTracingPipelineProperties.shaderGroupHandleAlignment);
		const uint32_t groupCount = static_cast<uint32_t>(shaderGroups.size());
		const uint32_t sbtSize = groupCount * handleSizeAligned;

		std::vector<uint8_t> shaderHandleStorage(sbtSize);
		VK_CHECK_RESULT(vkGetRayTracingShaderGroupHandlesKHR(device, pipeline, 0, groupCount, sbtSize, shaderHandleStorage.data()));

		createShaderBindingTable(shaderBindingTables.raygen, 1);
		// We are using two miss shaders
		createShaderBindingTable(shaderBindingTables.miss, 2);
		createShaderBindingTable(shaderBindingTables.hit, 1);

		// Copy handles
		shaderBindingTables.raygen.vbuffer->MemCopy(shaderHandleStorage.data(), handleSize);
		// We are using two miss shaders, so we need to get two handles for the miss shader binding table
		shaderBindingTables.miss.vbuffer->MemCopy(shaderHandleStorage.data() + handleSizeAligned, handleSize * 2);
		shaderBindingTables.hit.vbuffer->MemCopy(shaderHandleStorage.data() + handleSizeAligned * 3, handleSize);
	}

	inline VkWriteDescriptorSet writeDescriptorSet(
		VkDescriptorSet dstSet,
		VkDescriptorType type,
		uint32_t binding,
		VkDescriptorBufferInfo* bufferInfo,
		uint32_t descriptorCount = 1)
	{
		VkWriteDescriptorSet writeDescriptorSet{};
		writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSet.dstSet = dstSet;
		writeDescriptorSet.descriptorType = type;
		writeDescriptorSet.dstBinding = binding;
		writeDescriptorSet.pBufferInfo = bufferInfo;
		writeDescriptorSet.descriptorCount = descriptorCount;
		return writeDescriptorSet;
	}

	inline VkWriteDescriptorSet writeDescriptorSet(
		VkDescriptorSet dstSet,
		VkDescriptorType type,
		uint32_t binding,
		VkDescriptorImageInfo* imageInfo,
		uint32_t descriptorCount = 1)
	{
		VkWriteDescriptorSet writeDescriptorSet{};
		writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSet.dstSet = dstSet;
		writeDescriptorSet.descriptorType = type;
		writeDescriptorSet.dstBinding = binding;
		writeDescriptorSet.pImageInfo = imageInfo;
		writeDescriptorSet.descriptorCount = descriptorCount;
		return writeDescriptorSet;
	}

	/*
		Create the descriptor sets used for the ray tracing dispatch
	*/
	void createDescriptorSets()
	{
		std::vector<VkDescriptorPoolSize> poolSizes = {
			{ VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<uint32_t>(alltextures.size()) }
		};
		descriptorPool = vulkanDevice->CreateDescriptorSetsPool(poolSizes, 1);

		VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO , nullptr, descriptorPool, 1, &descriptorSetLayout };
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo, &descriptorSet));

		//VkWriteDescriptorSetAccelerationStructureKHR descriptorAccelerationStructureInfo = engine::initializers::writeDescriptorSetAccelerationStructureKHR();
		VkWriteDescriptorSetAccelerationStructureKHR descriptorAccelerationStructureInfo{};
		descriptorAccelerationStructureInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
		descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
		descriptorAccelerationStructureInfo.pAccelerationStructures = &topLevelAS.handle;

		VkWriteDescriptorSet accelerationStructureWrite{};
		accelerationStructureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		// The specialized acceleration structure descriptor has to be chained
		accelerationStructureWrite.pNext = &descriptorAccelerationStructureInfo;
		accelerationStructureWrite.dstSet = descriptorSet;
		accelerationStructureWrite.dstBinding = 0;
		accelerationStructureWrite.descriptorCount = 1;
		accelerationStructureWrite.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;

		VkDescriptorImageInfo storageImageDescriptor{ VK_NULL_HANDLE, storageImage->m_descriptor.imageView, VK_IMAGE_LAYOUT_GENERAL };
		VkDescriptorBufferInfo vertexBufferDescriptor{ scene.m_geometriesRT[0]->_vertexBuffer->GetVkBuffer(), 0, VK_WHOLE_SIZE };
		VkDescriptorBufferInfo indexBufferDescriptor{ scene.m_geometriesRT[0]->_indexBuffer->GetVkBuffer(), 0, VK_WHOLE_SIZE };
		VkDescriptorBufferInfo objdescBufferDescriptor{ m_bObjDesc->GetVkBuffer(), 0, VK_WHOLE_SIZE };

		std::vector<VkDescriptorImageInfo> diit;
		for (auto& texture : alltextures)
		{
			diit.emplace_back(texture->m_descriptor);
		}

		std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
			// Binding 0: Top level acceleration structure
			accelerationStructureWrite,
			// Binding 1: Ray tracing result image
			writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, &storageImageDescriptor),
			// Binding 2: Uniform data
			writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2, &ubo->m_descriptor),
			// Binding 3: Scene vertex buffer
			writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3, &vertexBufferDescriptor),
			// Binding 4: Scene index buffer
			writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4, &indexBufferDescriptor),
			// Binding 5: All textures
			writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5, diit.data(), static_cast<uint32_t>(diit.size())),
			// Binding 6: Reference to all objects
			writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 6, &objdescBufferDescriptor)
		};
		vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, VK_NULL_HANDLE);
	}

	VkDescriptorSetLayoutBinding descriptorSetLayoutBinding(
		VkDescriptorType type,
		VkShaderStageFlags stageFlags,
		uint32_t binding,
		uint32_t descriptorCount = 1)
	{
		VkDescriptorSetLayoutBinding setLayoutBinding{};
		setLayoutBinding.descriptorType = type;
		setLayoutBinding.stageFlags = stageFlags;
		setLayoutBinding.binding = binding;
		setLayoutBinding.descriptorCount = descriptorCount;
		return setLayoutBinding;
	}
	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo(
		const std::vector<VkDescriptorSetLayoutBinding>& bindings)
	{
		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
		descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorSetLayoutCreateInfo.pBindings = bindings.data();
		descriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		return descriptorSetLayoutCreateInfo;
	}

	/*
		Create our ray tracing pipeline
	*/
	void createRayTracingPipeline()
	{
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
			// Binding 0: Acceleration structure
			descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 0),
			// Binding 1: Storage image
			descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_RAYGEN_BIT_KHR, 1),
			// Binding 2: Uniform buffer
			descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 2),
			// Binding 3: Vertex buffer 
			descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 3),
			// Binding 4: Index buffer
			descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 4),
			// Binding 5: All textures
			descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 5, static_cast<uint32_t>(alltextures.size())),
			// Binding 6: Reference to all objects
			descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, 6)
		};

		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI = descriptorSetLayoutCreateInfo(setLayoutBindings);
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCI, nullptr, &descriptorSetLayout));

		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
		pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCreateInfo.setLayoutCount = 1;
		pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));
	
		std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

		// Ray generation group
		{
			shaderStages.push_back(loadShader(engine::tools::getAssetPath() + "shaders/raytracingscene/raygen.rgen.spv", VK_SHADER_STAGE_RAYGEN_BIT_KHR));
			VkRayTracingShaderGroupCreateInfoKHR shaderGroup{};
			shaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
			shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
			shaderGroup.generalShader = static_cast<uint32_t>(shaderStages.size()) - 1;
			shaderGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
			shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
			shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
			shaderGroups.push_back(shaderGroup);
		}

		// Miss group
		{
			shaderStages.push_back(loadShader(engine::tools::getAssetPath() + "shaders/raytracingscene/miss.rmiss.spv", VK_SHADER_STAGE_MISS_BIT_KHR));
			VkRayTracingShaderGroupCreateInfoKHR shaderGroup{};
			shaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
			shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
			shaderGroup.generalShader = static_cast<uint32_t>(shaderStages.size()) - 1;
			shaderGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
			shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
			shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
			shaderGroups.push_back(shaderGroup);
			// Second shader for shadows
			shaderStages.push_back(loadShader(engine::tools::getAssetPath() + "shaders/raytracingscene/shadow.rmiss.spv", VK_SHADER_STAGE_MISS_BIT_KHR));
			shaderGroup.generalShader = static_cast<uint32_t>(shaderStages.size()) - 1;
			shaderGroups.push_back(shaderGroup);
		}

		// Closest hit group
		{
			shaderStages.push_back(loadShader(engine::tools::getAssetPath() + "shaders/raytracingscene/closesthit.rchit.spv", VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR));
			VkRayTracingShaderGroupCreateInfoKHR shaderGroup{};
			shaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
			shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
			shaderGroup.generalShader = VK_SHADER_UNUSED_KHR;
			shaderGroup.closestHitShader = static_cast<uint32_t>(shaderStages.size()) - 1;
			shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
			shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
			shaderGroups.push_back(shaderGroup);
		}

		//VkRayTracingPipelineCreateInfoKHR rayTracingPipelineCI = engine::initializers::rayTracingPipelineCreateInfoKHR();
		VkRayTracingPipelineCreateInfoKHR rayTracingPipelineCI{};
		rayTracingPipelineCI.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
		rayTracingPipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
		rayTracingPipelineCI.pStages = shaderStages.data();
		rayTracingPipelineCI.groupCount = static_cast<uint32_t>(shaderGroups.size());
		rayTracingPipelineCI.pGroups = shaderGroups.data();
		rayTracingPipelineCI.maxPipelineRayRecursionDepth = 2;
		rayTracingPipelineCI.layout = pipelineLayout;
		VK_CHECK_RESULT(vkCreateRayTracingPipelinesKHR(device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &rayTracingPipelineCI, nullptr, &pipeline));
		
	}

	/*
		Create the uniform buffer used to pass matrices to the ray tracing ray generation shader
	*/
	void createUniformBuffer()
	{
		ubo = vulkanDevice->GetBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			sizeof(uniformData),
			&uniformData);
		VK_CHECK_RESULT(ubo->Map());

		updateUniformBuffers();
	}

	/*
		If the window has been resized, we need to recreate the storage image and it's descriptor
	*/
	virtual void WindowResized()
	{
		// Recreate image
		createStorageImage(swapChain.m_surfaceFormat.format, { width, height, 1 });
		// Update descriptor
		VkDescriptorImageInfo storageImageDescriptor{ VK_NULL_HANDLE, storageImage->m_descriptor.imageView, VK_IMAGE_LAYOUT_GENERAL };
		VkWriteDescriptorSet resultImageWrite = writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, &storageImageDescriptor);
		vkUpdateDescriptorSets(device, 1, &resultImageWrite, 0, VK_NULL_HANDLE);
	}

	/*
		Command buffer generation
	*/
	void BuildCommandBuffers()
	{
		VkCommandBufferBeginInfo cmdBufInfo{};
		cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		for (int32_t i = 0; i < m_drawCommandBuffers.size(); ++i)
		{
			//VK_CHECK_RESULT(vkBeginCommandBuffer(drawCommandBuffers[i], &cmdBufInfo));
			m_drawCommandBuffers[i]->Begin();

			VkCommandBuffer vkbuffer = ((render::VulkanCommandBuffer*)m_drawCommandBuffers[i])->m_vkCommandBuffer;
			/*
				Dispatch the ray tracing commands
			*/
			vkCmdBindPipeline(vkbuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline);
			vkCmdBindDescriptorSets(vkbuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipelineLayout, 0, 1, &descriptorSet, 0, 0);

			VkStridedDeviceAddressRegionKHR emptySbtEntry = {};
			vkCmdTraceRaysKHR(
				vkbuffer,
				&shaderBindingTables.raygen.stridedDeviceAddressRegion,
				&shaderBindingTables.miss.stridedDeviceAddressRegion,
				&shaderBindingTables.hit.stridedDeviceAddressRegion,
				&emptySbtEntry,
				width,
				height,
				1);

			/*
				Copy ray tracing output to swap chain image
			*/
			//temporary texture
			render::VulkanTexture swapChainTexture;
			swapChainTexture.m_vkImage = swapChain.m_images[i];

			// Prepare current swap chain image as transfer destination
			swapChainTexture.ChangeLayout(vkbuffer,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

			// Prepare ray tracing output image as transfer source
			storageImage->ChangeLayout(vkbuffer,
				VK_IMAGE_LAYOUT_GENERAL,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

			VkImageCopy copyRegion{};
			copyRegion.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
			copyRegion.srcOffset = { 0, 0, 0 };
			copyRegion.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
			copyRegion.dstOffset = { 0, 0, 0 };
			copyRegion.extent = { width, height, 1 };
			vkCmdCopyImage(vkbuffer, storageImage->m_vkImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, swapChain.m_images[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

			// Transition swap chain image back for presentation
			swapChainTexture.ChangeLayout(vkbuffer,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

			// Transition ray tracing output image back to general layout
			storageImage->ChangeLayout(vkbuffer,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_IMAGE_LAYOUT_GENERAL);

			//drawUI(drawCmdBuffers[i], frameBuffers[i]);//george

			//VK_CHECK_RESULT(vkEndCommandBuffer(drawCommandBuffers[i]));
			m_drawCommandBuffers[i]->End();

			//make sure the temporary texture doesn't take the image with it's destruction
			swapChainTexture.m_vkImage = nullptr;
		}
	}

	void updateUniformBuffers()
	{
		glm::mat4 perspectiveMatrix = camera.GetPerspectiveMatrix();
		glm::mat4 viewMatrix = camera.GetViewMatrix();

		uniformData.projInverse = glm::inverse(perspectiveMatrix);
		uniformData.viewInverse = glm::inverse(viewMatrix);
		uniformData.lightPos = glm::vec4(cos(glm::radians(timer * 360.0f)) * 40.0f, 50.0f + sin(glm::radians(timer * 360.0f)) * 20.0f, 25.0f + sin(glm::radians(timer * 360.0f)) * 5.0f, 0.0f);
		// Pass the vertex size to the shader for unpacking vertices
		uniformData.vertexSize = vertexLayoutRT.GetVertexSize(0);
		ubo->MemCopy(&uniformData, sizeof(uniformData));
	}

	void GetEnabledFeatures()
	{
		// Enable features required for ray tracing using feature chaining via pNext		
		enabledBufferDeviceAddresFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
		enabledBufferDeviceAddresFeatures.bufferDeviceAddress = VK_TRUE;

		enabledRayTracingPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
		enabledRayTracingPipelineFeatures.rayTracingPipeline = VK_TRUE;
		enabledRayTracingPipelineFeatures.pNext = &enabledBufferDeviceAddresFeatures;

		enabledAccelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
		enabledAccelerationStructureFeatures.accelerationStructure = VK_TRUE;
		enabledAccelerationStructureFeatures.pNext = &enabledRayTracingPipelineFeatures;

		deviceCreatepNextChain = &enabledAccelerationStructureFeatures;

		enabledFeatures.shaderInt64 = true;
	}

	void Prepare()
	{
		

		// Get properties and features
		rayTracingPipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
		VkPhysicalDeviceProperties2 deviceProperties2{};
		deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		deviceProperties2.pNext = &rayTracingPipelineProperties;
		vkGetPhysicalDeviceProperties2(vulkanDevice->physicalDevice, &deviceProperties2);
		accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
		VkPhysicalDeviceFeatures2 deviceFeatures2{};
		deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		deviceFeatures2.pNext = &accelerationStructureFeatures;
		vkGetPhysicalDeviceFeatures2(vulkanDevice->physicalDevice, &deviceFeatures2);
		// Get the function pointers required for ray tracing
		vkGetBufferDeviceAddressKHR = reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(vkGetDeviceProcAddr(device, "vkGetBufferDeviceAddressKHR"));
		vkCmdBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(device, "vkCmdBuildAccelerationStructuresKHR"));
		vkBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(device, "vkBuildAccelerationStructuresKHR"));
		vkCreateAccelerationStructureKHR = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(device, "vkCreateAccelerationStructureKHR"));
		vkDestroyAccelerationStructureKHR = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(vkGetDeviceProcAddr(device, "vkDestroyAccelerationStructureKHR"));
		vkGetAccelerationStructureBuildSizesKHR = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(vkGetDeviceProcAddr(device, "vkGetAccelerationStructureBuildSizesKHR"));
		vkGetAccelerationStructureDeviceAddressKHR = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(vkGetDeviceProcAddr(device, "vkGetAccelerationStructureDeviceAddressKHR"));
		vkCmdTraceRaysKHR = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(device, "vkCmdTraceRaysKHR"));
		vkGetRayTracingShaderGroupHandlesKHR = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(vkGetDeviceProcAddr(device, "vkGetRayTracingShaderGroupHandlesKHR"));
		vkCreateRayTracingPipelinesKHR = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetDeviceProcAddr(device, "vkCreateRayTracingPipelinesKHR"));

		// Create the acceleration structures used to render the ray traced scene
		createBottomLevelAccelerationStructure();
		createTopLevelAccelerationStructure();

		createStorageImage(swapChain.m_surfaceFormat.format, { width, height, 1 });
		createTextures();
		createUniformBuffer();
		createRayTracingPipeline();
		createShaderBindingTables();
		createDescriptorSets();
		BuildCommandBuffers();
		prepared = true;
	}

	virtual void update(float dt)
	{
		if (!paused /*|| camera.updated*/)
			updateUniformBuffers();
	}
};

VULKAN_EXAMPLE_MAIN()
