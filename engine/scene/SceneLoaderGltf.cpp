#include "SceneLoaderGltf.h"

#include "render/VulkanDevice.h"
#include "scene/Timer.h"
#include "Camera.h"
#include "render/VulkanRenderPass.h"

#include <algorithm>
#define TINYGLTF_IMPLEMENTATION

#define TINYGLTF_NO_STB_IMAGE_WRITE

#define TINYGLTF_NO_STB_IMAGE_WRITE
#ifdef VK_USE_PLATFORM_ANDROID_KHR
#define TINYGLTF_ANDROID_LOAD_FROM_ASSETS
#endif
#include "tiny_gltf.h"

namespace engine
{
	namespace scene
	{
		#define SHADOWMAP_DIM 1024

		void SceneLoaderGltf::LoadImages(tinygltf::Model& input, VkQueue queue)
		{
			modelsTextures.resize(input.images.size());
			for (size_t i = 0; i < input.images.size(); i++) {
				tinygltf::Image& glTFImage = input.images[i];
				unsigned char* buffer = nullptr;
				VkDeviceSize bufferSize = 0;
				bool deleteBuffer = false;
				// We convert RGB-only images to RGBA, as most devices don't support RGB-formats in Vulkan
				if (glTFImage.component == 3) {
					bufferSize = glTFImage.width * glTFImage.height * 4;
					buffer = new unsigned char[bufferSize];
					unsigned char* rgba = buffer;
					unsigned char* rgb = &glTFImage.image[0];
					for (size_t i = 0; i < glTFImage.width * glTFImage.height; ++i) {
						memcpy(rgba, rgb, sizeof(unsigned char) * 3);
						rgba += 4;
						rgb += 3;
					}
					deleteBuffer = true;
				}
				else {
					buffer = &glTFImage.image[0];
					bufferSize = glTFImage.image.size();
				}

				modelsTextures[i] = _device->GetTexture(buffer, bufferSize, glTFImage.width, glTFImage.height, VK_FORMAT_R8G8B8A8_UNORM, queue);

				if (deleteBuffer) {
					delete[] buffer;
				}
			}
			m_placeholder = _device->GetTexture(engine::tools::getAssetPath() + "textures/white_placeholder.png", VK_FORMAT_R8G8B8A8_UNORM, queue);
		}

		void SceneLoaderGltf::LoadMaterials(tinygltf::Model& input, bool deferred)
		{
			render::VertexLayout* vertex_layout = nullptr;

			uniform_manager.SetDevice(_device->logicalDevice);
			uniform_manager.SetEngineDevice(_device);
			sceneVertexUniformBuffer = uniform_manager.GetGlobalUniformBuffer({ UNIFORM_PROJECTION ,UNIFORM_VIEW ,UNIFORM_LIGHT0_POSITION, UNIFORM_CAMERA_POSITION });
			shadow_uniform_buffer = uniform_manager.GetGlobalUniformBuffer({ UNIFORM_LIGHT0_SPACE });

			render_objects.resize(input.materials.size());
			VkPipelineColorBlendAttachmentState opaqueState{};
			opaqueState.blendEnable = VK_FALSE;
			opaqueState.colorWriteMask = 0xf;
			std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStates;
			blendAttachmentStates.push_back(opaqueState);
			if (deferred)
				blendAttachmentStates.push_back(opaqueState);

			VkPipelineColorBlendAttachmentState transparentState{
				VK_TRUE,
				VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
				VK_BLEND_OP_ADD,
				VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_FACTOR_ZERO,
				VK_BLEND_OP_ADD,
				0xf
			};
			std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStatestrans;
			blendAttachmentStatestrans.push_back(transparentState);
			if (deferred)
				blendAttachmentStatestrans.push_back(transparentState);

			std::vector<VkDescriptorPoolSize> poolSizes =
			{
				VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 5 * static_cast<uint32_t>(render_objects.size())},
				VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5 * static_cast<uint32_t>(render_objects.size() + globalTextures.size())},
				VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 2}
			};

			_device->CreateDescriptorSetsPool(poolSizes, static_cast<uint32_t>(2 * render_objects.size()) + 5);
			std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> modelbindings
			{
				{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
				{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
				{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
			};
			render::VulkanDescriptorSetLayout* currentDesclayout = _device->GetDescriptorSetLayout(modelbindings);

			render::PipelineProperties props;
			props.cullMode = VK_CULL_MODE_NONE;//TODO why tf
			props.pAttachments = blendAttachmentStates.data();
			render::VulkanPipeline* currentPipeline = _device->GetPipeline(currentDesclayout->m_descriptorSetLayout, vlayout.m_vertexInputBindings, vlayout.m_vertexInputAttributes,
				engine::tools::getAssetPath() + "shaders/scene/pbr.vert.spv", engine::tools::getAssetPath() + "shaders/scene/pbrtextured.frag.spv", modelsVkRenderPass, vKpipelineCache,
				props);

			std::vector<VkDescriptorBufferInfo*> buffersDescriptors;
			buffersDescriptors.push_back(&sceneVertexUniformBuffer->m_descriptor);

			for (size_t i = 0; i < input.materials.size(); i++) {
				render_objects[i] = new RenderObject;
				std::vector<VkDescriptorImageInfo*> texturesDescriptors;
				tinygltf::Material glTFMaterial = input.materials[i];
				if (glTFMaterial.values.find("baseColorFactor") != glTFMaterial.values.end()) {
					//materials[i].baseColorFactor = glm::make_vec4(glTFMaterial.values["baseColorFactor"].ColorFactor().data());
				}
				if (glTFMaterial.values.find("baseColorTexture") != glTFMaterial.values.end()) {
					texturesDescriptors.push_back(&modelsTextures[glTFMaterial.values["baseColorTexture"].TextureIndex()]->m_descriptor);
				}
				else
				{
					texturesDescriptors.push_back(&m_placeholder->m_descriptor);
				}
				if (glTFMaterial.values.find("metallicRoughnessTexture") != glTFMaterial.values.end()) {
					texturesDescriptors.push_back(&modelsTextures[glTFMaterial.values["metallicRoughnessTexture"].TextureIndex()]->m_descriptor);
				}
				else
				{
					texturesDescriptors.push_back(&m_placeholder->m_descriptor);
				}
;
				render_objects[i]->SetDescriptorSetLayout(currentDesclayout);
				render_objects[i]->_vertexLayout = &vslayout;
				render_objects[i]->AddPipeline(currentPipeline);
				render::VulkanDescriptorSet* desc = _device->GetDescriptorSet(buffersDescriptors, texturesDescriptors,
					currentDesclayout->m_descriptorSetLayout, currentDesclayout->m_setLayoutBindings);
				render_objects[i]->AddDescriptor(desc);
			}
		}

		void SceneLoaderGltf::LoadNode(const tinygltf::Node& inputNode, glm::mat4 parentMatrix, const tinygltf::Model& input)
		{
			glm::mat4 mymatrix = parentMatrix;

			if (inputNode.translation.size() == 3) {
				mymatrix = glm::translate(mymatrix, glm::vec3(glm::make_vec3(inputNode.translation.data())));
			}
			if (inputNode.rotation.size() == 4) {
				glm::quat q = glm::make_quat(inputNode.rotation.data());
				glm::mat4 quatmat = glm::mat4(q);
				mymatrix *= quatmat;
			}
			if (inputNode.scale.size() == 3) {
				mymatrix = glm::scale(mymatrix, glm::vec3(glm::make_vec3(inputNode.scale.data())));
			}
			if (inputNode.matrix.size() == 16) {
				mymatrix = glm::make_mat4x4(inputNode.matrix.data());
			};
			if (inputNode.children.size() > 0) {
				for (size_t i = 0; i < inputNode.children.size(); i++) {
					LoadNode(input.nodes[inputNode.children[i]], mymatrix, input);
				}
			}

			if (inputNode.mesh > -1)
			{
				const tinygltf::Mesh mesh = input.meshes[inputNode.mesh];
				// Iterate through all primitives of this node's mesh
				for (size_t i = 0; i < mesh.primitives.size(); i++)
				{
					const tinygltf::Primitive& glTFPrimitive = mesh.primitives[i];
					uint32_t indexCount = 0;
					size_t vertexCount = 0;

					Geometry* geometry = new Geometry();
					geometry->_device = _device->logicalDevice;
					geometry->m_instanceNo = 1;//TODO what if we want multiple instances
					geometry->m_vertexCount = vertexCount;
					geometry->m_indexCount = indexCount;

					// Vertices
					{
						const float* positionBuffer = nullptr;
						const float* normalsBuffer = nullptr;
						const float* texCoordsBuffer = nullptr;						

						// Get buffer data for vertex positions
						if (glTFPrimitive.attributes.find("POSITION") != glTFPrimitive.attributes.end()) {
							const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.attributes.find("POSITION")->second];
							const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
							positionBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
							vertexCount = accessor.count;
						}
						// Get buffer data for vertex normals
						if (glTFPrimitive.attributes.find("NORMAL") != glTFPrimitive.attributes.end()) {
							const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.attributes.find("NORMAL")->second];
							const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
							normalsBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
						}
						// Get buffer data for vertex texture coordinates
						// glTF supports multiple sets, we only load the first one
						if (glTFPrimitive.attributes.find("TEXCOORD_0") != glTFPrimitive.attributes.end()) {
							const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.attributes.find("TEXCOORD_0")->second];
							const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
							texCoordsBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
						}

						geometry->m_vertexCount = vertexCount;
						geometry->m_verticesSize = geometry->m_vertexCount * vlayout.GetVertexSize(0) / sizeof(float);
						geometry->m_vertices = new float[geometry->m_verticesSize];

						int vertex_index = 0;
						// Append data to model's vertex buffer
						for (size_t v = 0; v < vertexCount; v++) {
							
							glm::vec4 pPos = mymatrix * glm::vec4(glm::make_vec3(&positionBuffer[v * 3]), 1.0f) ;
							glm::vec3 pNormal = glm::normalize(glm::vec3(normalsBuffer ? glm::make_vec3(&normalsBuffer[v * 3]) : glm::vec3(0.0f)));
							pNormal = glm::vec3(mymatrix * glm::vec4(pNormal, 0.0));
							glm::vec2 pTexCoord = texCoordsBuffer ? glm::make_vec2(&texCoordsBuffer[v * 2]) : glm::vec3(0.0f);

							geometry->m_vertices[vertex_index++] = pPos.x ;
							geometry->m_vertices[vertex_index++] = -pPos.y ;
							geometry->m_vertices[vertex_index++] = pPos.z ;

							geometry->m_vertices[vertex_index++] = pNormal.x;
							geometry->m_vertices[vertex_index++] = -pNormal.y;
							geometry->m_vertices[vertex_index++] = pNormal.z;

							geometry->m_vertices[vertex_index++] = pTexCoord.x;
							geometry->m_vertices[vertex_index++] = pTexCoord.y;
						}
					}
					
					// Indices
					{
						const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.indices];
						const tinygltf::BufferView& bufferView = input.bufferViews[accessor.bufferView];
						const tinygltf::Buffer& buffer = input.buffers[bufferView.buffer];
						

						geometry->m_indexCount += static_cast<uint32_t>(accessor.count);
						geometry->m_indices = new uint32_t[geometry->m_indexCount];
						
						int index_index = 0;
						switch (accessor.componentType) {
						case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
							const uint32_t* buf = reinterpret_cast<const uint32_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
							for (size_t index = 0; index < accessor.count; index++) {
								geometry->m_indices[index] = buf[index];
							}
							break;
						}
						case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
							const uint16_t* buf = reinterpret_cast<const uint16_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
							for (size_t index = 0; index < accessor.count; index++) {
								geometry->m_indices[index] = (uint32_t)buf[index];
							}
							break;
						}
						case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
							const uint8_t* buf = reinterpret_cast<const uint8_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
							for (size_t index = 0; index < accessor.count; index++) {
								geometry->m_indices[index] = buf[index];
							}
							break;
						}
						default:
							std::cerr << "Index component type " << accessor.componentType << " not supported!" << std::endl;
							return;
						}
						
					}
					render_objects[glTFPrimitive.material]->AddGeometry(geometry);
				}
				
				
			}
		}

		std::vector<RenderObject*> SceneLoaderGltf::LoadFromFile2(const std::string& foldername, const std::string& filename, ModelCreateInfo2* createInfo,
			render::VulkanDevice* device,
			VkQueue copyQueue, VkRenderPass renderPass, VkPipelineCache pipelineCache, bool deferred)
		{
			_device = device;
			modelsVkRenderPass = renderPass;
			vKpipelineCache = pipelineCache;
			tinygltf::Model glTFInput;
			tinygltf::TinyGLTF gltfContext;
			std::string error, warning;

			bool fileLoaded = gltfContext.LoadASCIIFromFile(&glTFInput, &error, &warning, foldername + filename);
			if (!fileLoaded)
			{
				std::cerr << "Could not load " << filename << std::endl;
				return render_objects;
			}
			LoadImages(glTFInput, copyQueue);
			LoadMaterials(glTFInput);
		
			const tinygltf::Scene& scene = glTFInput.scenes[0];
			for (size_t i = 0; i < scene.nodes.size(); i++) {
				const tinygltf::Node node = glTFInput.nodes[scene.nodes[i]];
				LoadNode(node, glm::mat4(1.0f), glTFInput);
			}

			for (auto robj : render_objects)
			{
				if (robj)
					for (auto geo : robj->m_geometries)
					{
						geo->SetIndexBuffer(_device->GetGeometryBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, copyQueue, geo->m_indexCount * sizeof(uint32_t), geo->m_indices));
						geo->SetVertexBuffer(_device->GetGeometryBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, copyQueue, geo->m_verticesSize * sizeof(float), geo->m_vertices));
					}
			}

			return render_objects;
		};

		std::vector<RenderObject*> SceneLoaderGltf::LoadFromFile(const std::string& foldername, const std::string& filename, float scale, engine::render::VulkanDevice* device, VkQueue copyQueue
			, VkRenderPass renderPass, VkPipelineCache pipelineCache, bool deferred)
		{
			ModelCreateInfo2 modelCreateInfo(glm::vec3(1.0f), glm::vec2(1.0, -1.0), glm::vec3(1.0f));
			return LoadFromFile2(foldername, filename, &modelCreateInfo, device, copyQueue, renderPass, pipelineCache, deferred);
		}

		void SceneLoaderGltf::CreateShadow(VkQueue copyQueue)
		{
			shadowmap = _device->GetDepthRenderTarget(SHADOWMAP_DIM, SHADOWMAP_DIM, true, VK_IMAGE_ASPECT_DEPTH_BIT, false);
			//shadowmap = _device->GetDepthRenderTarget(SHADOWMAP_DIM, SHADOWMAP_DIM, true);
			shadowmapColor = _device->GetColorRenderTarget(SHADOWMAP_DIM, SHADOWMAP_DIM, FB_COLOR_FORMAT);
			shadowPass = _device->GetRenderPass({ { shadowmapColor->m_format, shadowmapColor->m_descriptor.imageLayout}, { shadowmap->m_format, shadowmap->m_descriptor.imageLayout} });
			render::VulkanFrameBuffer* fb = _device->GetFrameBuffer(shadowPass->GetRenderPass(), SHADOWMAP_DIM, SHADOWMAP_DIM, { shadowmapColor->m_descriptor.imageView, shadowmap->m_descriptor.imageView });
			shadowPass->AddFrameBuffer(fb);
			shadowPass->SetClearColor({ 1.0f,1.0f,1.0f }, 0);
		}

		void SceneLoaderGltf::CreateShadowObjects(VkPipelineCache pipelineCache)
		{
			std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> offscreenbindings
			{
				{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT}
			};
			render::VulkanDescriptorSetLayout* desc_layout = _device->GetDescriptorSetLayout(offscreenbindings);

			std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> offscreencolorbindings
			{
				{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
				{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT}
			};
			render::VulkanDescriptorSetLayout* descColorLayout = _device->GetDescriptorSetLayout(offscreencolorbindings);

			std::vector<render::VertexLayout*> vlayouts;

			for (int ro_index=0;ro_index<render_objects.size();ro_index++)
			{
				RenderObject* ro = render_objects[ro_index];
				if (!ro)
					continue;
				render::VertexLayout* l = ro->_vertexLayout;
				//it = std::find(vlayouts.begin(), vlayouts.end(), l);
				int index = -1;
				for (int i = 0; i < vlayouts.size(); i++)
				{
					if (vlayouts[i] == l)
						index = i;
				}

				/*if (index != -1 && !individualFragmentUniformBuffers[ro_index])
				{
					shadow_objects[index]->AdoptGeometriesFrom(*ro);
				}
				else*/
				//if(!individualFragmentUniformBuffers[ro_index])
				{
					RenderObject* sro = new RenderObject;
					sro->_vertexLayout = l;
					

					//sro->m_geometries.insert(sro->m_geometries.end(), ro->m_geometries.begin(), ro->m_geometries.end());
					sro->AdoptGeometriesFrom(*ro);

					render::VulkanDescriptorSetLayout* currentdescayout = nullptr;

					std::vector<VkDescriptorBufferInfo*> buffersDescriptors{ &shadow_uniform_buffer->m_descriptor };
					if (individualFragmentUniformBuffers[ro_index])
					{
						buffersDescriptors.push_back(&individualFragmentUniformBuffers[ro_index]->m_descriptor);
						currentdescayout = descColorLayout;
					}
					else
						currentdescayout = desc_layout;

					sro->SetDescriptorSetLayout(currentdescayout);

					std::string sm_vertex_file(engine::tools::getAssetPath() + std::string("shaders/shadowmapping/offscreen.vert.spv"));
					std::string sm_fragment_file(engine::tools::getAssetPath() + ((individualFragmentUniformBuffers[ro_index] == nullptr) ? std::string("shaders/shadowmapping/offscreen.frag.spv") : std::string("shaders/shadowmapping/offscreencolor.frag.spv")) );

					bool blendenable = areTransparents[ro_index];

					//This one doesn't write to color. Carefull here if we want to use variance shadowmapping or other techniques that require aditional data from the color buffer
					VkPipelineColorBlendAttachmentState opaqueState{};
					opaqueState.blendEnable = VK_FALSE;
					opaqueState.colorWriteMask = 0xf;
					std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStates{ opaqueState };

					render::PipelineProperties props;
					props.attachmentCount = static_cast<uint32_t>(blendAttachmentStates.size());
					props.pAttachments = blendAttachmentStates.data();
					props.depthBias = true;
					props.cullMode = VK_CULL_MODE_NONE;
					props.depthTestEnable = true;
					

					if (blendenable)
					{
						props.depthWriteEnable = false;
						blendAttachmentStates[0].colorWriteMask = 0xf;//We want to write to color only for transparents;
					}

					render::VulkanPipeline* p = _device->GetPipeline(currentdescayout->m_descriptorSetLayout, l->m_vertexInputBindings, l->m_vertexInputAttributes,
						sm_vertex_file, sm_fragment_file, shadowPass->GetRenderPass(), pipelineCache, props);
					sro->AddPipeline(p);			

					render::VulkanDescriptorSet* set = _device->GetDescriptorSet(buffersDescriptors, {},
						currentdescayout->m_descriptorSetLayout, currentdescayout->m_setLayoutBindings);
					sro->AddDescriptor(set);

					vlayouts.push_back(l);
					shadow_objects.push_back(sro);
				}

			}
		}

		void SceneLoaderGltf::DrawShadowsInSeparatePass(VkCommandBuffer command_buffer)
		{
			float depthBiasConstant = 0.25f;
			// Slope depth bias factor, applied depending on polygon's slope
			float depthBiasSlope = 0.75f;
			shadowPass->Begin(command_buffer, 0);

			// Set depth bias (aka "Polygon offset")
			// Required to avoid shadow mapping artefacts
			vkCmdSetDepthBias(
				command_buffer,
				depthBiasConstant,
				0.0f,
				depthBiasSlope);

			for (auto obj : shadow_objects)
				obj->Draw(command_buffer);

			shadowPass->End(command_buffer);
		}

		render::VulkanDescriptorSetLayout* SceneLoaderGltf::GetDescriptorSetlayout(std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> layoutBindigs)
		{
			for (auto dl : descriptorSetlayouts)
			{
				if (dl->m_setLayoutBindings.size() != layoutBindigs.size())
					continue;
				
				bool match = true;
				for (uint32_t i=0; i<dl->m_setLayoutBindings.size();i++)
				{
					if (dl->m_setLayoutBindings[i].descriptorType != layoutBindigs[i].first || dl->m_setLayoutBindings[i].stageFlags != layoutBindigs[i].second)
					{
						match = false;
						break;
					}
				}
				if (match)
				{
					return dl;
				}
			}
			render::VulkanDescriptorSetLayout* dsl = _device->GetDescriptorSetLayout(layoutBindigs);
			descriptorSetlayouts.push_back(dsl);
			return dsl;
		}

		void SceneLoaderGltf::Update(float timer)
		{
			float zNear = 10.0f;
			float zFar = 906.0f;
			// Matrix from light's point of view
			glm::mat4 depthProjectionMatrix = glm::perspective(glm::radians(lightFOV), 1.0f, zNear, zFar);
			glm::mat4 depthViewMatrix = glm::lookAt(glm::vec3(light_pos), glm::vec3(0.0f), glm::vec3(0, 1, 0));
			glm::mat4 depthModelMatrix = glm::mat4(1.0f);

			uboShadowOffscreenVS.depthMVP = depthProjectionMatrix * depthViewMatrix * depthModelMatrix;

			uniform_manager.UpdateGlobalParams(UNIFORM_LIGHT0_SPACE, &uboShadowOffscreenVS.depthMVP, 0, sizeof(uboShadowOffscreenVS.depthMVP));

			glm::mat4 depthbiasMatrix = glm::mat4(
				glm::vec4(0.5, 0.0, 0.0, 0.0),
				glm::vec4(0.0, 0.5, 0.0, 0.0),
				glm::vec4(0.0, 0.0, 1.0, 0.0),
				glm::vec4(0.5, 0.5, 0.0, 1.0)
			);

			glm::mat4 biasedDepthMVP = depthbiasMatrix * uboShadowOffscreenVS.depthMVP;
			glm::mat4 viewMatrix = m_camera->GetViewMatrix();
			glm::mat4 perspectiveMatrix = m_camera->GetPerspectiveMatrix();

			uniform_manager.UpdateGlobalParams(UNIFORM_PROJECTION, &perspectiveMatrix, 0, sizeof(perspectiveMatrix));
			uniform_manager.UpdateGlobalParams(UNIFORM_VIEW, &viewMatrix, 0, sizeof(viewMatrix));
			uniform_manager.UpdateGlobalParams(UNIFORM_LIGHT0_SPACE_BIASED, &biasedDepthMVP, 0, sizeof(biasedDepthMVP));
			uniform_manager.UpdateGlobalParams(UNIFORM_LIGHT0_POSITION, &light_pos, 0, sizeof(light_pos));
			glm::vec3 cucu = m_camera->GetPosition();
			cucu.y = -cucu.y;
			uniform_manager.UpdateGlobalParams(UNIFORM_CAMERA_POSITION, &cucu, 0, sizeof(m_camera->GetPosition()));

			uniform_manager.Update();
		}

		void SceneLoaderGltf::UpdateView(float timer)
		{
			glm::mat4 viewMatrix = m_camera->GetViewMatrix();
			uniform_manager.UpdateGlobalParams(UNIFORM_VIEW, &viewMatrix, 0, sizeof(viewMatrix));
			glm::vec3 cucu = m_camera->GetPosition();
			cucu.y = -cucu.y;
			uniform_manager.UpdateGlobalParams(UNIFORM_CAMERA_POSITION, &cucu, 0, sizeof(m_camera->GetPosition()));

			/*glm::mat4 viewproj = m_camera->matrices.perspective * m_camera->matrices.view;
			uniform_manager.UpdateGlobalParams(UNIFORM_PROJECTION_VIEW, &viewproj, 0, sizeof(viewproj));
			glm::vec4 bias_near_far_pow = glm::vec4(0.002f, m_camera->getNearClip(), m_camera->getFarClip(), 1.0f);
			uniform_manager.UpdateGlobalParams(UNIFORM_LIGHT0_SPACE_BIASED, &bias_near_far_pow, 0, sizeof(bias_near_far_pow));*/

			uniform_manager.Update();
		}

		SceneLoaderGltf::~SceneLoaderGltf()
		{
			for (auto shadowobj : shadow_objects)
				delete shadowobj;

			shadow_objects.clear();

			for (auto robj : render_objects)
				delete robj;

			render_objects.clear();
		}
	}
}