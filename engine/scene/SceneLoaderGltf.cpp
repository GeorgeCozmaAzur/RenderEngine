#include "SceneLoaderGltf.h"

//#include "render/vulkan/VulkanDevice.h"
#include "scene/Timer.h"
#include "Camera.h"
//#include "render/vulkan/VulkanRenderPass.h"

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

		void SceneLoaderGltf::CreateDescriptorPool(tinygltf::Model& input)
		{
			render_objects.resize(input.materials.size());

			descriptorPool = _device->GetDescriptorPool({
			{ render::DescriptorType::UNIFORM_BUFFER, 5 * static_cast<uint32_t>(render_objects.size()) },
			{render::DescriptorType::IMAGE_SAMPLER, 6 * static_cast<uint32_t>(render_objects.size() + globalTextures.size()) },
			{render::DescriptorType::STORAGE_IMAGE, 2 }
				}, static_cast<uint32_t>(2 * render_objects.size()) + 5);

			descriptorPoolDSV = _device->GetDescriptorPool({ { render::DescriptorType::DSV ,1 } }, 1);
			descriptorPoolRTV = _device->GetDescriptorPool({ { render::DescriptorType::RTV ,1 } }, 1);
		}

		void SceneLoaderGltf::LoadImages(tinygltf::Model& input)
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

				render::GfxFormat format = render::GfxFormat::R8G8B8A8_UNORM;
				switch (input.images[i].bits)
				{
					case 16: format = render::GfxFormat::R16G16B16A16_UNORM; break;
					case 32: format = render::GfxFormat::R32G32B32A32_SFLOAT; break;
					case 8: 
					default: format = render::GfxFormat::R8G8B8A8_UNORM;
				}

				render::Texture2DData data;
				data.CreateFromBuffer(buffer, bufferSize, glTFImage.width, glTFImage.height, format);

				/*modelsTextures[i] = _device->GetTexture(&data, queue,
					VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, TRUE);	*/
				modelsTextures[i] = _device->GetTexture(&data, descriptorPool, m_loadingCommandBuffer);
				//data.Destroy();

				if (deleteBuffer) {
					delete[] buffer;
				}
			}
			render::Texture2DData data;
			data.LoadFromFile(engine::tools::getAssetPath() + "textures/white_placeholder.png", render::GfxFormat::R8G8B8A8_UNORM);
			//m_placeholder = _device->GetTexture(&data, queue);
			m_placeholder = _device->GetTexture(&data, descriptorPool, m_loadingCommandBuffer);
			//data.Destroy();
			//m_placeholder = _device->GetTexture(engine::tools::getAssetPath() + "textures/white_placeholder.png", VK_FORMAT_R8G8B8A8_UNORM, queue);

			modelsTexturesIds.resize(input.textures.size());
			for (size_t i = 0; i < input.textures.size(); i++) {
				modelsTexturesIds[i] = input.textures[i].source;
			}
		}

		void SceneLoaderGltf::LoadMaterials(tinygltf::Model& input, bool deferred)
		{
			render::VulkanVertexLayout* vertex_layout = nullptr;

			uniform_manager.SetDescriptorPool(descriptorPool);
			uniform_manager.SetEngineDevice(_device);
			sceneVertexUniformBuffer = deferred == false ? (useShadows ==false ? uniform_manager.GetGlobalUniformBuffer({ UNIFORM_PROJECTION ,UNIFORM_VIEW ,UNIFORM_LIGHT0_POSITION, UNIFORM_CAMERA_POSITION }) :
																				uniform_manager.GetGlobalUniformBuffer({ UNIFORM_PROJECTION ,UNIFORM_VIEW, UNIFORM_LIGHT0_SPACE_BIASED ,UNIFORM_LIGHT0_POSITION, UNIFORM_CAMERA_POSITION })) :
																		uniform_manager.GetGlobalUniformBuffer({ UNIFORM_PROJECTION ,UNIFORM_VIEW, UNIFORM_CAMERA_POSITION });
			sceneFragmentUniformBuffer = deferred == false ? uniform_manager.GetGlobalUniformBuffer({ UNIFORM_LIGHT0_COLOR }) : nullptr;
			shadow_uniform_buffer = uniform_manager.GetGlobalUniformBuffer({ UNIFORM_LIGHT0_SPACE });

			render::BlendAttachmentState opaqueState{false};
			std::vector <render::BlendAttachmentState> blendAttachmentStates;
			/*VkPipelineColorBlendAttachmentState opaqueState{};
			opaqueState.blendEnable = VK_FALSE;
			opaqueState.colorWriteMask = 0xf;
			std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStates;*/
			
			if (deferred)
				blendAttachmentStates = { opaqueState, opaqueState, opaqueState, opaqueState };
			else
				blendAttachmentStates = { opaqueState };

			/*VkPipelineColorBlendAttachmentState transparentState{
				VK_TRUE,
				VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
				VK_BLEND_OP_ADD,
				VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_FACTOR_ZERO,
				VK_BLEND_OP_ADD,
				0xf
			};*/
			render::BlendAttachmentState transparentState{ true };
			//std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStatestrans;
			std::vector <render::BlendAttachmentState> blendAttachmentStatestrans;
			blendAttachmentStatestrans.push_back(transparentState);
			if (deferred)
				blendAttachmentStatestrans.push_back(transparentState);

			/*std::vector<VkDescriptorPoolSize> poolSizes =
			{
				VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 5 * static_cast<uint32_t>(render_objects.size())},
				VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5 * static_cast<uint32_t>(render_objects.size() + globalTextures.size())},
				VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 2}
			};
			descriptorPool = _device->CreateDescriptorSetsPool(poolSizes, static_cast<uint32_t>(2 * render_objects.size()) + 5);*/
			

			std::vector<render::LayoutBinding> modelbindings
			{
				{render::DescriptorType::UNIFORM_BUFFER, render::ShaderStage::VERTEX},
				{render::DescriptorType::UNIFORM_BUFFER, render::ShaderStage::FRAGMENT},
				{render::DescriptorType::IMAGE_SAMPLER, render::ShaderStage::FRAGMENT},
				{render::DescriptorType::IMAGE_SAMPLER, render::ShaderStage::FRAGMENT},
			};
			if(deferred == false)
				modelbindings.insert(modelbindings.begin()+2, { render::DescriptorType::UNIFORM_BUFFER, render::ShaderStage::FRAGMENT });

			for (auto tex : globalTextures)
			{
				modelbindings.push_back({ render::DescriptorType::IMAGE_SAMPLER, render::ShaderStage::FRAGMENT });
			}

			render::DescriptorSetLayout* currentDesclayoutSimple = _device->GetDescriptorSetLayout(modelbindings);
			render::DescriptorSetLayout* currentDesclayoutNormalmap = nullptr;

			std::string shaderfolder = (deferred ? deferredShadersFolder : forwardShadersFolder) + "/";
			render::PipelineProperties props;
			props.cullMode = render::CullMode::FRONT;
			props.attachmentCount = blendAttachmentStates.size();
			props.pAttachments = blendAttachmentStates.data();
			/*render::VulkanPipeline* currentPipeline = _device->GetPipeline(currentDesclayoutSimple->m_descriptorSetLayout, vertexlayout.m_vertexInputBindings, vertexlayout.m_vertexInputAttributes,
				shaderfolder + lightingVS, shaderfolder + lightingFS, modelsVkRenderPass, vKpipelineCache, props);*/
			render::Pipeline* currentPipeline = _device->GetPipeline(
				shaderfolder + lightingVS, "VSMain", shaderfolder + lightingFS, "PSMain",
				vertexlayout, currentDesclayoutSimple, props, modelsVkRenderPass);

			render::Pipeline* currentPipelineNormalmap = nullptr;
			bool hasNormalmap = false;
			for (size_t i = 0; i < input.materials.size(); i++)
			{
				if (input.materials[i].additionalValues.find("normalTexture") != input.materials[i].additionalValues.end())
				{
					hasNormalmap = true;
					modelbindings.push_back({ render::DescriptorType::IMAGE_SAMPLER, render::ShaderStage::FRAGMENT });
					currentDesclayoutNormalmap = _device->GetDescriptorSetLayout(modelbindings);
					/*currentPipelineNormalmap = _device->GetPipeline(currentDesclayoutNormalmap->m_descriptorSetLayout, vertexlayoutNormalmap.m_vertexInputBindings, vertexlayoutNormalmap.m_vertexInputAttributes,
						shaderfolder + normalmapVS, shaderfolder + normalmapFS, modelsVkRenderPass, vKpipelineCache, props);*/
					currentPipelineNormalmap = _device->GetPipeline(
						shaderfolder + normalmapVS, "VSMain", shaderfolder + normalmapFS, "PSMain",
						vertexlayoutNormalmap, currentDesclayoutNormalmap, props, modelsVkRenderPass);
					break;
				}
			}

			individualFragmentUniformBuffers.resize(input.materials.size());
			std::fill(individualFragmentUniformBuffers.begin(), individualFragmentUniformBuffers.end(), nullptr);

			struct frag_data
			{
				float baseColorFactor = 1.0f;
				float metallicFactor = 1.0f;
				float roughnessFactor = 1.0f;
				float aoFactor = 0.01f;
				void Reset() {
					baseColorFactor = 1.0f; metallicFactor = 1.0f; roughnessFactor = 1.0f; aoFactor = 0.01f;
				}
			} fdata;

			for (size_t i = 0; i < input.materials.size(); i++) {
				render_objects[i] = new RenderObject;
				std::vector<render::Texture*> texturesDescriptors;
				std::vector<render::Buffer*> buffersDescriptors;
				buffersDescriptors.push_back(sceneVertexUniformBuffer);
				if(sceneFragmentUniformBuffer)
				buffersDescriptors.push_back(sceneFragmentUniformBuffer);
				tinygltf::Material glTFMaterial = input.materials[i];
				fdata.Reset();
				hasNormalmap = false;
				if (glTFMaterial.values.find("baseColorFactor") != glTFMaterial.values.end()) {
					//fdata.baseColorFactor = glm::make_vec4(glTFMaterial.values["baseColorFactor"].ColorFactor().data());
				}
				if (glTFMaterial.values.find("metallicFactor") != glTFMaterial.values.end()) {
					fdata.metallicFactor = glTFMaterial.values["metallicFactor"].Factor();
				}
				if (glTFMaterial.values.find("baseColorTexture") != glTFMaterial.values.end()) {
					int texIndex = modelsTexturesIds[glTFMaterial.values["baseColorTexture"].TextureIndex()];
					texturesDescriptors.push_back(modelsTextures[texIndex]);
				}
				else
				{
					texturesDescriptors.push_back(m_placeholder);
				}
				if (glTFMaterial.values.find("metallicRoughnessTexture") != glTFMaterial.values.end()) {
					texturesDescriptors.push_back(modelsTextures[modelsTexturesIds[glTFMaterial.values["metallicRoughnessTexture"].TextureIndex()]]);
				}
				else
				{
					texturesDescriptors.push_back(m_placeholder);
				}
				if (glTFMaterial.additionalValues.find("normalTexture") != glTFMaterial.additionalValues.end()) {
					texturesDescriptors.push_back(modelsTextures[modelsTexturesIds[glTFMaterial.additionalValues["normalTexture"].TextureIndex()]]);
					hasNormalmap = true;
				}

				for (auto tex : globalTextures)
				{
					texturesDescriptors.push_back(tex);
				}

				//individualFragmentUniformBuffers[i] = _device->GetUniformBuffer(sizeof(fdata), false, queue, &fdata);
				individualFragmentUniformBuffers[i] = _device->GetUniformBuffer(sizeof(fdata), &fdata, descriptorPool, true);//TODO make it gpu visible
				buffersDescriptors.push_back(individualFragmentUniformBuffers[i]);

				render::DescriptorSetLayout* currentDesclayout = hasNormalmap ? currentDesclayoutNormalmap : currentDesclayoutSimple;

				render_objects[i]->SetDescriptorSetLayout(currentDesclayout);
				render_objects[i]->_vertexLayout = hasNormalmap ? vertexlayoutNormalmap : vertexlayout;
				render_objects[i]->AddPipeline(hasNormalmap ? currentPipelineNormalmap : currentPipeline);
				/*render::VulkanDescriptorSet* desc = _device->GetDescriptorSet(descriptorPool, buffersDescriptors, texturesDescriptors,
					currentDesclayout->m_descriptorSetLayout, currentDesclayout->m_setLayoutBindings);
				render_objects[i]->AddDescriptor(desc);*/
				render::DescriptorSet* desc = _device->GetDescriptorSet(currentDesclayout, descriptorPool, buffersDescriptors, texturesDescriptors);
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

					render::MeshData* geometry = new render::MeshData();
					//geometry->_device = _device->logicalDevice;
					geometry->m_instanceNo = 1;//TODO what if we want multiple instances
					geometry->m_vertexCount = vertexCount;
					geometry->m_indexCount = indexCount;

					// Vertices
					
						const float* positionBuffer = nullptr;
						const float* normalsBuffer = nullptr;
						const float* texCoordsBuffer = nullptr;
						const float* tangentsBuffer = nullptr;

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
						if (glTFPrimitive.attributes.find("TANGENT") != glTFPrimitive.attributes.end()) {
							const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.attributes.find("TANGENT")->second];
							const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
							tangentsBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
						}

						bool hasNormalmap = false;
						if (input.materials[glTFPrimitive.material].additionalValues.find("normalTexture") != input.materials[glTFPrimitive.material].additionalValues.end())
						{
							hasNormalmap = true;
						}
						render::VertexLayout* vlayout = hasNormalmap ? vertexlayoutNormalmap : vertexlayout;

						geometry->m_vertexCount = vertexCount;
						geometry->m_verticesSize = geometry->m_vertexCount * vlayout->GetVertexSize(0) / sizeof(float);
						geometry->m_vertices = new float[geometry->m_verticesSize];

						int vertex_index = 0;
						// Append data to model's vertex buffer
						for (size_t v = 0; v < vertexCount; v++) {

							glm::vec3 pPos = mymatrix * glm::vec4(glm::make_vec3(&positionBuffer[v * 3]), 1.0f); //pPos.y = -pPos.y;
							glm::vec3 pNormal = glm::normalize(glm::vec3(normalsBuffer ? glm::make_vec3(&normalsBuffer[v * 3]) : glm::vec3(0.0f))); 
							pNormal = glm::vec3(mymatrix * glm::vec4(pNormal, 0.0)); //pNormal.y = -pNormal.y;
							glm::vec2 pTexCoord = texCoordsBuffer ? glm::make_vec2(&texCoordsBuffer[v * 2]) : glm::vec3(0.0f);
							glm::vec4 tangent = tangentsBuffer ? glm::make_vec4(&tangentsBuffer[v * 4]) : glm::vec4(0.0f); tangent.w = 0.0f;
							tangent = mymatrix * tangent;
							//tangent.y = -tangent.y;

							for (auto& component : vlayout->m_components[0])
							{
								switch (component) {
								case render::VERTEX_COMPONENT_POSITION:
									memcpy(geometry->m_vertices + vertex_index, &pPos, sizeof(pPos)); vertex_index += 3;								
									break;
								case render::VERTEX_COMPONENT_NORMAL:
									memcpy(geometry->m_vertices + vertex_index, &pNormal, sizeof(pNormal)); vertex_index += 3;
									break;
								case render::VERTEX_COMPONENT_UV:
									memcpy(geometry->m_vertices + vertex_index, &pTexCoord, sizeof(pTexCoord)); vertex_index += 2;
									break;
								case render::VERTEX_COMPONENT_COLOR:
									geometry->m_vertices[vertex_index++] = 1.0f;
									geometry->m_vertices[vertex_index++] = 1.0f;
									geometry->m_vertices[vertex_index++] = 1.0f;
									break;
								case render::VERTEX_COMPONENT_TANGENT4:
									memcpy(geometry->m_vertices + vertex_index, &tangent, sizeof(tangent)); vertex_index += 4;
									break;
								case render::VERTEX_COMPONENT_BITANGENT:
									geometry->m_vertices[vertex_index++] = 0.0f;
									geometry->m_vertices[vertex_index++] = 0.0f;
									geometry->m_vertices[vertex_index++] = 0.0f;
									break;
									// Dummy components for padding
								case render::VERTEX_COMPONENT_DUMMY_FLOAT:
									geometry->m_vertices[vertex_index++] = 0.0f;
									break;
								case render::VERTEX_COMPONENT_DUMMY_VEC4:
									geometry->m_vertices[vertex_index++] = 0.0f;
									geometry->m_vertices[vertex_index++] = 0.0f;
									geometry->m_vertices[vertex_index++] = 0.0f;
									geometry->m_vertices[vertex_index++] = 0.0f;
									break;
								};
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
								geometry->m_indices[index] = (uint32_t)buf[index];
							}
							break;
						}
						default:
							std::cerr << "Index component type " << accessor.componentType << " not supported!" << std::endl;
							return;
						}

					}
					render_objects[glTFPrimitive.material]->AddGeometry(_device->GetMesh(geometry, vlayout, m_loadingCommandBuffer));
					delete geometry;
				}
			}
			else
			{
				if (inputNode.extensions.find("KHR_lights_punctual") != (inputNode.extensions.end()))
				{
					//light_pos = glm::vec4(0.0f,0.0f,0.0f,1.0f) * mymatrix;
					glm::vec3 lightpos = glm::vec3(0.0f);//inputNode.translation.size() > 0 ? glm::make_vec3(inputNode.translation.data()) : glm::vec3(0.0f);
					lightPositions.push_back(mymatrix * glm::vec4(lightpos, 1.0f));
					//light_pos.y = -light_pos.y;
					//glm::quat q = glm::make_quat(inputNode.rotation.data());
					//glm::mat4 quatmat = glm::mat4(q);
					//light_pos = quatmat * light_pos;
				}
			}
		}

		std::vector<RenderObject*> SceneLoaderGltf::LoadFromFile(const std::string& foldername, const std::string& filename, float scale, engine::render::GraphicsDevice* device
			, render::RenderPass* renderPass, bool deferred, bool withShadow)
		{
			_device = device;
			modelsVkRenderPass = renderPass;
			useShadows = withShadow;
			m_deferred = deferred;
			//vKpipelineCache = pipelineCache;
			tinygltf::Model glTFInput;
			tinygltf::TinyGLTF gltfContext;
			std::string error, warning;

			if(!vertexlayout)
			vertexlayout = _device->GetVertexLayout(
				{
					render::VERTEX_COMPONENT_POSITION,
					render::VERTEX_COMPONENT_NORMAL,
					render::VERTEX_COMPONENT_UV
				}, {});
			if(!vertexlayoutNormalmap)
			vertexlayoutNormalmap = _device->GetVertexLayout(
				{
					render::VERTEX_COMPONENT_POSITION,
					render::VERTEX_COMPONENT_NORMAL,
					render::VERTEX_COMPONENT_UV,
					render::VERTEX_COMPONENT_TANGENT4
				}, {});


			bool fileLoaded = gltfContext.LoadASCIIFromFile(&glTFInput, &error, &warning, foldername + filename);
			if (!fileLoaded)
			{
				std::cerr << "Could not load " << filename << std::endl;
				return render_objects;
			}
			CreateDescriptorPool(glTFInput);
			if (withShadow)
			{
				CreateShadow();
			}
			
			LoadImages(glTFInput);
			LoadMaterials(glTFInput, deferred);

			const tinygltf::Scene& scene = glTFInput.scenes[0];
			for (size_t i = 0; i < scene.nodes.size(); i++) {
				const tinygltf::Node node = glTFInput.nodes[scene.nodes[i]];
				LoadNode(node, glm::mat4(1.0f), glTFInput);
			}

			if (withShadow)
			{
				CreateShadowObjects();
			}

			/*for (auto robj : render_objects)
			{
				if (robj)
					for (auto geo : robj->m_geometries)
					{
						geo->SetIndexBuffer(_device->GetGeometryBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, copyQueue, geo->m_indexCount * sizeof(uint32_t), geo->m_indices));
						geo->SetVertexBuffer(_device->GetGeometryBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, copyQueue, geo->m_verticesSize * sizeof(float), geo->m_vertices));
					}
			}*/

			if (lightPositions.size() == 0)
				lightPositions.push_back(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

			return render_objects;
		};

		void SceneLoaderGltf::CreateShadow()
		{
			//shadowmap = _device->GetDepthRenderTarget(SHADOWMAP_DIM, SHADOWMAP_DIM, true, VK_IMAGE_ASPECT_DEPTH_BIT, false);
			shadowmap = _device->GetDepthRenderTarget(SHADOWMAP_DIM, SHADOWMAP_DIM, render::GfxFormat::D32_FLOAT, descriptorPool, descriptorPoolDSV, m_loadingCommandBuffer, true, false);
			//shadowmap = _device->GetDepthRenderTarget(SHADOWMAP_DIM, SHADOWMAP_DIM, true);
			shadowmapColor = _device->GetRenderTarget(SHADOWMAP_DIM, SHADOWMAP_DIM, render::GfxFormat::R8G8B8A8_UNORM, descriptorPool, descriptorPoolRTV, m_loadingCommandBuffer);
			/*shadowPass = _device->GetRenderPass({ { shadowmapColor->m_format, shadowmapColor->m_descriptor.imageLayout}, { shadowmap->m_format, shadowmap->m_descriptor.imageLayout} });
			render::VulkanFrameBuffer* fb = _device->GetFrameBuffer(shadowPass->GetRenderPass(), SHADOWMAP_DIM, SHADOWMAP_DIM, { shadowmapColor->m_descriptor.imageView, shadowmap->m_descriptor.imageView });
			shadowPass->AddFrameBuffer(fb);
			shadowPass->SetClearColor({ 1.0f,1.0f,1.0f }, 0);*/
			shadowPass = _device->GetRenderPass(SHADOWMAP_DIM, SHADOWMAP_DIM, { shadowmapColor }, shadowmap);

			if(!m_deferred)
				globalTextures.push_back(shadowmap);
		}

		void SceneLoaderGltf::CreateShadowObjects()
		{
			std::vector<render::LayoutBinding> offscreenbindings
			{
				{render::DescriptorType::UNIFORM_BUFFER, render::ShaderStage::VERTEX}
			};

			/*std::vector<render::LayoutBinding> offscreenbindings
			{
				{render::DescriptorType::UNIFORM_BUFFER, render::ShaderStage::VERTEX}
			};*/
			render::DescriptorSetLayout* desc_layout = _device->GetDescriptorSetLayout(offscreenbindings);

			std::vector<render::LayoutBinding> offscreencolorbindings
			{
				{render::DescriptorType::UNIFORM_BUFFER, render::ShaderStage::VERTEX},
				{render::DescriptorType::UNIFORM_BUFFER, render::ShaderStage::FRAGMENT}
			};
			/*std::vector<render::LayoutBinding> offscreencolorbindings
			{
				{render::DescriptorType::UNIFORM_BUFFER, render::ShaderStage::VERTEX},
				{render::DescriptorType::UNIFORM_BUFFER, render::ShaderStage::FRAGMENT}
			};*/
			render::DescriptorSetLayout* descColorLayout = _device->GetDescriptorSetLayout(offscreencolorbindings);

			std::vector<render::VertexLayout*> vlayouts;

			for (int ro_index = 0; ro_index < render_objects.size(); ro_index++)
			{
				RenderObject* ro = render_objects[ro_index];
				if (!ro)
					continue;
				render::VertexLayout* l = (render::VertexLayout*)ro->_vertexLayout;//george bad
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

					render::DescriptorSetLayout* currentdescayout = nullptr;

					std::vector<render::Buffer*> buffersDescriptors{ shadow_uniform_buffer };
					/*if (individualFragmentUniformBuffers[ro_index])
					{
						buffersDescriptors.push_back(individualFragmentUniformBuffers[ro_index]);
						currentdescayout = descColorLayout;
					}
					else
						currentdescayout = desc_layout;*/

					currentdescayout = desc_layout;
					sro->SetDescriptorSetLayout(currentdescayout);

					std::string sm_vertex_file = shadowmapVS;//(engine::tools::getAssetPath() + std::string("shaders/shadowmapping/offscreen.vert.spv"));
					std::string sm_fragment_file = shadowmapFS;//(individualFragmentUniformBuffers[ro_index] == nullptr) ? shadowmapFS : shadowmapFSColored;//(engine::tools::getAssetPath() + ((individualFragmentUniformBuffers[ro_index] == nullptr) ? std::string("shaders/shadowmapping/offscreen.frag.spv") : std::string("shaders/shadowmapping/offscreencolor.frag.spv")));

					bool blendenable = false;// areTransparents[ro_index];

					//This one doesn't write to color. Carefull here if we want to use variance shadowmapping or other techniques that require aditional data from the color buffer
					/*VkPipelineColorBlendAttachmentState opaqueState{};
					opaqueState.blendEnable = VK_FALSE;
					opaqueState.colorWriteMask = 0xf;
					std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStates{ opaqueState };*/
					render::BlendAttachmentState opaqueState{ false };
					std::vector <render::BlendAttachmentState> blendAttachmentStates;

					render::PipelineProperties props;
					props.attachmentCount = static_cast<uint32_t>(blendAttachmentStates.size());
					props.pAttachments = blendAttachmentStates.data();
					props.depthBias = true;
					props.cullMode = render::CullMode::NONE;
					props.depthTestEnable = true;


					if (blendenable)
					{
						props.depthWriteEnable = false;
						//blendAttachmentStates[0].colorWriteMask = 0xf;//We want to write to color only for transparents;
					}

					/*render::VulkanPipeline* p = _device->GetPipeline(currentdescayout->m_descriptorSetLayout, l->m_vertexInputBindings, l->m_vertexInputAttributes,
						sm_vertex_file, sm_fragment_file, shadowPass->GetRenderPass(), pipelineCache, props);*/
					render::Pipeline* p = _device->GetPipeline(
						sm_vertex_file, "VSMain", sm_fragment_file, "PSMain",
						l, currentdescayout, props, shadowPass);
					sro->AddPipeline(p);

					/*render::VulkanDescriptorSet* set = _device->GetDescriptorSet(descriptorPool, buffersDescriptors, {},
						currentdescayout->m_descriptorSetLayout, currentdescayout->m_setLayoutBindings);*/
					render::DescriptorSet* set = _device->GetDescriptorSet(currentdescayout, descriptorPool, buffersDescriptors, {});
					sro->AddDescriptor(set);

					vlayouts.push_back(l);
					shadow_objects.push_back(sro);
				}

			}
		}

		void SceneLoaderGltf::DrawShadowsInSeparatePass(render::CommandBuffer* command_buffer)
		{
			if (!useShadows)
				return;
			float depthBiasConstant = 0.25f;
			// Slope depth bias factor, applied depending on polygon's slope
			float depthBiasSlope = 0.75f;
			shadowPass->Begin(command_buffer, 0);

			// Set depth bias (aka "Polygon offset")
			// Required to avoid shadow mapping artefacts
			/*vkCmdSetDepthBias(
				command_buffer,
				depthBiasConstant,
				0.0f,
				depthBiasSlope);*/
			descriptorPool->Draw(command_buffer);
			for (auto obj : shadow_objects)
				obj->Draw(command_buffer);

			shadowPass->End(command_buffer);
		}

		/*render::VulkanDescriptorSetLayout* SceneLoaderGltf::GetDescriptorSetlayout(std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> layoutBindigs)
		{
			for (auto dl : descriptorSetlayouts)
			{
				if (dl->m_setLayoutBindings.size() != layoutBindigs.size())
					continue;

				bool match = true;
				for (uint32_t i = 0; i < dl->m_setLayoutBindings.size(); i++)
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
		}*/
		float time = 0.0f;
		void SceneLoaderGltf::UpdateLights(int index, glm::vec4 position, glm::vec4 color, float timer)
		{
			glm::vec3 ll = lightPositions.size() > 0 ? lightPositions[0] : glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
			ll += position;
			uniform_manager.UpdateGlobalParams(UNIFORM_LIGHT0_POSITION, &ll, 0, sizeof(ll));
			//glm::vec4 lcolor = glm::vec4(flicker, flicker, flicker, 1.0f);
			uniform_manager.UpdateGlobalParams(UNIFORM_LIGHT0_COLOR, &color, 0, sizeof(color));
			uniform_manager.Update(nullptr);
		}

		void SceneLoaderGltf::Update(float timer)
		{
			time += timer;
			//float flicker = 8 + 2 * sin(3.0 * time) + 1 * glm::fract(sin(time * 12.9898) * 43758.5453);

			glm::vec3 ll = glm::vec4(20.0f, 20.0f, 0.0f, 1.0f);//lightPositions.size() > 0 ? lightPositions[0] : glm::vec4(0.0f,1.0f,0.0f,1.0f);
			float zNear = 10.0f;
			float zFar = 100.0f;

		/*	glm::vec3 offset = glm::vec3(
				0.1 * sin(time * 2.0) + 0.05 * glm::fract(sin(time * 5.0) * 100.0),
				0.1 * sin(time * 3.5) + 0.05 * glm::fract(sin(time * 7.0) * 100.0),
				0.1 * sin(time * 1.8) + 0.05 * glm::fract(sin(time * 6.0) * 100.0)
			);
			ll += offset;*/

			// Matrix from light's point of view
			glm::mat4 depthProjectionMatrix = glm::perspective(glm::radians(lightFOV), 1.0f, zNear, zFar);
			if (m_camera->GetFlipY())
				depthProjectionMatrix[1][1] *= -1;
			glm::mat4 depthViewMatrix = glm::lookAt(glm::vec3(ll), glm::vec3(0.0f), glm::vec3(0, 1, 0));
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
			uniform_manager.UpdateGlobalParams(UNIFORM_LIGHT0_POSITION, &ll, 0, sizeof(ll));
			glm::vec4 lcolor = glm::vec4(1.0f);//flicker, flicker, flicker, 1.0f);
			uniform_manager.UpdateGlobalParams(UNIFORM_LIGHT0_COLOR, &lcolor, 0, sizeof(lcolor));
			glm::vec3 cucu = m_camera->GetPosition();
			cucu = -cucu;
			uniform_manager.UpdateGlobalParams(UNIFORM_CAMERA_POSITION, &cucu, 0, sizeof(m_camera->GetPosition()));

			uniform_manager.Update(nullptr);
		}

		void SceneLoaderGltf::UpdateView(float timer)
		{
			glm::mat4 viewMatrix = m_camera->GetViewMatrix();
			uniform_manager.UpdateGlobalParams(UNIFORM_VIEW, &viewMatrix, 0, sizeof(viewMatrix));
			glm::vec3 cucu = m_camera->GetPosition();
			cucu = -cucu;
			uniform_manager.UpdateGlobalParams(UNIFORM_CAMERA_POSITION, &cucu, 0, sizeof(m_camera->GetPosition()));

			/*glm::mat4 viewproj = m_camera->matrices.perspective * m_camera->matrices.view;
			uniform_manager.UpdateGlobalParams(UNIFORM_PROJECTION_VIEW, &viewproj, 0, sizeof(viewproj));
			glm::vec4 bias_near_far_pow = glm::vec4(0.002f, m_camera->getNearClip(), m_camera->getFarClip(), 1.0f);
			uniform_manager.UpdateGlobalParams(UNIFORM_LIGHT0_SPACE_BIASED, &bias_near_far_pow, 0, sizeof(bias_near_far_pow));*/

			uniform_manager.Update(nullptr);
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