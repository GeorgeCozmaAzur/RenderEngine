#include "SceneLoader.h"
#include <assimp/Importer.hpp> 
#include <assimp/scene.h>  
#include <assimp/cimport.h>
#include "render/VulkanDevice.h"
#include "scene/Timer.h"
#include "camera.hpp"
#include "render/VulkanRenderPass.h"
//#include "fbxsdk.h"
#include <algorithm>


/**
		* Loads a 3D model from a file into Vulkan buffers
		*
		* @param device Pointer to the Vulkan device used to generated the vertex and index buffers on
		* @param filename File to load (must be a model format supported by ASSIMP)
		* @param layout Vertex layout components (position, normals, tangents, etc.)
		* @param createInfo MeshCreateInfo structure for load time settings like scale, center, etc.
		* @param copyQueue Queue used for the memory staging copy commands (must support transfer)
		*/
namespace engine
{
	namespace scene
	{
		#define SHADOWMAP_DIM 1024
		/*void PrintNode(FbxNode* pNode) {
			//PrintTabs();
			const char* nodeName = pNode->GetName();
			FbxDouble3 translation = pNode->LclTranslation.Get();
			FbxDouble3 rotation = pNode->LclRotation.Get();
			FbxDouble3 scaling = pNode->LclScaling.Get();

			// Print the contents of the node.
			printf("<node name='%s' translation='(%f, %f, %f)' rotation='(%f, %f, %f)' scaling='(%f, %f, %f)'>\n",
				nodeName,
				translation[0], translation[1], translation[2],
				rotation[0], rotation[1], rotation[2],
				scaling[0], scaling[1], scaling[2]
			);
			//numTabs++;

			// Print the node's attributes.
			//for (int i = 0; i < pNode->GetNodeAttributeCount(); i++)
			//	PrintAttribute(pNode->GetNodeAttributeByIndex(i));

			// Recursively print the children.
			for (int j = 0; j < pNode->GetChildCount(); j++)
				PrintNode(pNode->GetChild(j));

			//numTabs--;
			//PrintTabs();
			printf("</node>\n");
		}*/

		std::vector<RenderObject*> SceneLoader::LoadFromFile2(const std::string& foldername, const std::string& filename, ModelCreateInfo2* createInfo,
			render::VulkanDevice* device,
			VkQueue copyQueue, VkRenderPass renderPass, VkPipelineCache pipelineCache, bool deferred)
		{

			render::VertexLayout* vertex_layout = nullptr;

			uniform_manager.SetDevice(device->logicalDevice);
			uniform_manager.SetEngineDevice(device);
			_device = device;
			sceneVertexUniformBuffer = uniform_manager.GetGlobalUniformBuffer({ UNIFORM_PROJECTION ,UNIFORM_VIEW, UNIFORM_LIGHT0_SPACE_BIASED ,UNIFORM_LIGHT0_POSITION, UNIFORM_CAMERA_POSITION });
			shadow_uniform_buffer = uniform_manager.GetGlobalUniformBuffer({ UNIFORM_LIGHT0_SPACE });

			std::string shaderfolder = engine::tools::getAssetPath() + "shaders/" + (deferred ? deferredShadersFolder : forwardShadersFolder) + "/";

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

			Assimp::Importer Importer;
			const aiScene* pScene;

			// Load file
#if defined(__ANDROID__)
			// Meshes are stored inside the apk on Android (compressed)
			// So they need to be loaded via the asset manager

			AAsset* asset = AAssetManager_open(androidApp->activity->assetManager, filename.c_str(), AASSET_MODE_STREAMING);
			if (!asset) {
				LOGE("Could not load mesh from \"%s\"!", filename.c_str());
				return render_objects;
			}
			assert(asset);
			size_t size = AAsset_getLength(asset);

			assert(size > 0);

			void* meshData = malloc(size);
			AAsset_read(asset, meshData, size);
			AAsset_close(asset);

			pScene = Importer.ReadFileFromMemory(meshData, size, defaultFlags);

			free(meshData);
#else
			static const int ldefaultFlags = aiProcess_FlipWindingOrder | aiProcess_Triangulate | aiProcess_PreTransformVertices | aiProcess_CalcTangentSpace;
			pScene = Importer.ReadFile((foldername + filename).c_str(), ldefaultFlags);
			if (!pScene) {
				std::string error = Importer.GetErrorString();
				engine::tools::exitFatal(error + "\n\nThe file may be part of the additional asset pack.\n\nRun \"download_assets.py\" in the repository root to download the latest version.", -1);
			}
#endif

			//FbxManager *lSdkManager = FbxManager::Create();
			//// create an IOSettings object
			//FbxIOSettings * ios = FbxIOSettings::Create(lSdkManager, IOSROOT);
			//// set some IOSettings options 
			//ios->SetBoolProp(IMP_FBX_MATERIAL, true);
			//ios->SetBoolProp(IMP_FBX_TEXTURE, true);
			//// create an empty scene
			//FbxScene* lScene = FbxScene::Create(lSdkManager, "");
			//// Create an importer.
			//FbxImporter* lImporter = FbxImporter::Create(lSdkManager, "");
			//// Initialize the importer by providing a filename and the IOSettings to use
			//if (!lImporter->Initialize((engine::tools::getAssetPath() + "models/Minimal_Default.fbx").c_str(), -1, ios))
			//{
			//	engine::tools::exitFatal("\n\cannot load fbx", -1);
			//}
			//// Import the scene.
			//lImporter->Import(lScene);

			//FbxNode* lRootNode = lScene->GetRootNode();
			//if (lRootNode) {
			//	//for (int i = 0; i < lRootNode->GetChildCount(); i++)
			//		//PrintNode(lRootNode->GetChild(i));
			//}

			// Destroy the importer.
			//lImporter->Destroy();
			/*struct UniformData {
				glm::mat4 projection;
				glm::mat4 view;
				glm::mat4 model;
			} uniformData;*/

			//light_pos = glm::vec4(24.0f, -165.0f, -36.0f, 1.0f);
			light_pos = glm::vec4(-19.0f, 70.0f, -120.0f, 1.0f);

			if (pScene)
			{
				parts.clear();
				parts.resize(pScene->mNumMeshes);

				glm::vec3 scale(1.0f);
				glm::vec2 uvscale(1.0f);
				glm::vec3 center(0.0f);
				if (createInfo)
				{
					scale = createInfo->scale;
					uvscale = createInfo->uvscale;
					center = createInfo->center;
				}

				// Textures
				std::string texFormatSuffix;
				VkFormat CompressedTexFormat;
				// Get supported compressed texture format
				if (device->m_enabledFeatures.textureCompressionBC) {
					texFormatSuffix = "_bc3_unorm";
					CompressedTexFormat = VK_FORMAT_BC3_UNORM_BLOCK;
				}
				else if (device->m_enabledFeatures.textureCompressionASTC_LDR) {
					texFormatSuffix = "_astc_8x8_unorm";
					CompressedTexFormat = VK_FORMAT_ASTC_8x8_UNORM_BLOCK;
				}
				else if (device->m_enabledFeatures.textureCompressionETC2) {
					texFormatSuffix = "_etc2_unorm";
					CompressedTexFormat = VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;
				}
				else {
					engine::tools::exitFatal("Device does not support any compressed texture format!", VK_ERROR_FEATURE_NOT_PRESENT);
				}

				render_objects.resize(pScene->mNumMaterials);//TODO

				individualFragmentUniformBuffers.resize(pScene->mNumMaterials);
				std::fill(individualFragmentUniformBuffers.begin(), individualFragmentUniformBuffers.end(), nullptr);
				areTransparents.resize(pScene->mNumMaterials);

				//TODO see exactly how many descriptors are needed
				std::vector<VkDescriptorPoolSize> poolSizes =
				{
					VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 5 * static_cast<uint32_t>(render_objects.size())},
					VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5 * static_cast<uint32_t>(render_objects.size() + globalTextures.size())},
					VkDescriptorPoolSize {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 2}
				};

				device->CreateDescriptorSetsPool(poolSizes, static_cast<uint32_t>(2 * render_objects.size()) + 5);

				for (size_t i = 0; i < render_objects.size(); i++)
				{
					render_objects[i] = new RenderObject();

					aiString texturefile;
					aiString texturefilen;
					aiString texturefiled;
					pScene->mMaterials[i]->GetTexture(aiTextureType_DIFFUSE, 0, &texturefile);
					pScene->mMaterials[i]->GetTexture(aiTextureType_NORMALS, 0, &texturefilen);
					pScene->mMaterials[i]->GetTexture(aiTextureType_DISPLACEMENT, 0, &texturefiled);

					std::vector<VkDescriptorBufferInfo*> buffersDescriptors;
					buffersDescriptors.push_back(&sceneVertexUniformBuffer->m_descriptor);

					std::vector<VkDescriptorImageInfo*> texturesDescriptors;

					render::VulkanDescriptorSetLayout* currentDesclayout = nullptr;
					render::VulkanPipeline* currentPipeline = nullptr;
					render::VertexLayout* currentVertexLayout = nullptr;				

					if (pScene->mMaterials[i]->GetTextureCount(aiTextureType_DIFFUSE) > 0)
					{
						VkFormat texFormat;

						std::string texfilename = std::string(texturefile.C_Str());
						std::replace(texfilename.begin(), texfilename.end(), '\\', '/');

						size_t index = texfilename.find(".ktx");
						if (index != std::string::npos)
						{
							texfilename.insert(index, texFormatSuffix);
							texFormat = CompressedTexFormat;
						}
						else
						{
							texFormat = VK_FORMAT_R8G8B8A8_UNORM;//TODO make format more flexible
						}

						render::VulkanTexture* tex = device->GetTexture(foldername + texfilename, texFormat, copyQueue);
						
						texturesDescriptors.push_back(&tex->m_descriptor);

						if (pScene->mMaterials[i]->GetTextureCount(aiTextureType_NORMALS) > 0)
						{
							std::string texfilenamen = std::string(texturefilen.C_Str());
							tex = device->GetTexture(foldername + texfilenamen, texFormat, copyQueue);
							texturesDescriptors.push_back(&tex->m_descriptor);
							for(auto tex : globalTextures)
								texturesDescriptors.push_back(&tex->m_descriptor);

							std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> bindings
							{ { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT } };
							if (sceneFragmentUniformBuffer)
							{
								bindings.push_back({ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT });
								buffersDescriptors.push_back(&sceneFragmentUniformBuffer->m_descriptor);
							}

							for (auto td : texturesDescriptors)
								bindings.push_back({ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT });

							currentDesclayout = GetDescriptorSetlayout(bindings);

							currentVertexLayout = &vnlayout;
						
							currentPipeline = device->GetPipeline(currentDesclayout->m_descriptorSetLayout, vnlayout.m_vertexInputBindings, vnlayout.m_vertexInputAttributes,
								shaderfolder+normalmapVS, shaderfolder+normalmapFS, renderPass, pipelineCache, false, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, nullptr, static_cast<uint32_t>(blendAttachmentStates.size()), blendAttachmentStates.data());
							
						}
						else
						{
							if (!render_objects[i])
								continue;

							for (auto tex : globalTextures)
								texturesDescriptors.push_back(&tex->m_descriptor);

							std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> bindings
							{ { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT } };
							if (sceneFragmentUniformBuffer)
							{
								bindings.push_back({ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT });
								buffersDescriptors.push_back(&sceneFragmentUniformBuffer->m_descriptor);
							}

							for (auto td : texturesDescriptors)
								bindings.push_back({ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT });

							currentDesclayout = GetDescriptorSetlayout(bindings);

							currentVertexLayout = &vlayout;

							currentPipeline = device->GetPipeline(currentDesclayout->m_descriptorSetLayout, vlayout.m_vertexInputBindings, vlayout.m_vertexInputAttributes,
								shaderfolder + lightingVS, shaderfolder + lightingTexturedFS, renderPass, pipelineCache,
									false, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, nullptr, static_cast<uint32_t>(blendAttachmentStates.size()), blendAttachmentStates.data());

						}
					}
					else
					{
						if (!render_objects[i])
							continue;

						struct frag_data
						{
							aiColor3D diffuse = aiColor3D(0.3f, 0.3f, 0.3f);;
							float shininess = 1.0f;
							float transparency = 1.0f;
						} fdata;
						pScene->mMaterials[i]->Get(AI_MATKEY_OPACITY, fdata.transparency);
						pScene->mMaterials[i]->Get(AI_MATKEY_COLOR_DIFFUSE, fdata.diffuse);
						pScene->mMaterials[i]->Get(AI_MATKEY_SHININESS, fdata.shininess);

						render::VulkanBuffer* frag_buffer = device->GetUniformBuffer(sizeof(fdata), false, copyQueue, &fdata);

						individualFragmentUniformBuffers[i] = frag_buffer;

						for (auto tex : globalTextures)
							texturesDescriptors.push_back(&tex->m_descriptor);

						std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> bindings
						{ 
							{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT }
						};

						if (sceneFragmentUniformBuffer)
						{
							bindings.push_back({ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT });
							buffersDescriptors.push_back(&sceneFragmentUniformBuffer->m_descriptor);
						}

						bindings.push_back({ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT });
						buffersDescriptors.push_back(&frag_buffer->m_descriptor);

						for (auto td : texturesDescriptors)
							bindings.push_back({ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT });

						currentDesclayout = GetDescriptorSetlayout(bindings);

						currentVertexLayout = &vlayout;

						if (fdata.transparency != 1.0f)
						{
							currentPipeline = device->GetPipeline(currentDesclayout->m_descriptorSetLayout, vlayout.m_vertexInputBindings, vlayout.m_vertexInputAttributes,
									shaderfolder + lightingVS, shaderfolder + lightingFS, renderPass, pipelineCache, true,
									VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, nullptr, static_cast<uint32_t>(blendAttachmentStatestrans.size()), blendAttachmentStatestrans.data());
							areTransparents[i] = true;
						}
						else
						{
							currentPipeline = device->GetPipeline(currentDesclayout->m_descriptorSetLayout, vlayout.m_vertexInputBindings, vlayout.m_vertexInputAttributes,
									shaderfolder + lightingVS, shaderfolder + lightingFS, renderPass, pipelineCache,
									false, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, nullptr, static_cast<uint32_t>(blendAttachmentStates.size()), blendAttachmentStates.data());
							areTransparents[i] = false;
						}
					}

					if (render_objects[i])
					{
						render_objects[i]->SetDescriptorSetLayout(currentDesclayout);
						render_objects[i]->_vertexLayout = currentVertexLayout;
						render_objects[i]->AddPipeline(currentPipeline);
						render::VulkanDescriptorSet* desc = device->GetDescriptorSet(buffersDescriptors, texturesDescriptors,
							currentDesclayout->m_descriptorSetLayout, currentDesclayout->m_setLayoutBindings);
						render_objects[i]->AddDescriptor(desc);
					}
				}

				Timer timer;

				size_t total_vertices = 0;
				// Load meshes
				for (unsigned int i = 0; i < pScene->mNumMeshes; i++)
				{
					const aiMesh* paiMesh = pScene->mMeshes[i];
					//if (pScene->mMaterials[paiMesh->mMaterialIndex]->GetTextureCount(aiTextureType_DIFFUSE) > 0)
					//	continue;

					Geometry* geometry = new Geometry();
					geometry->_device = device->logicalDevice;
					geometry->m_instanceNo = 1;//TODO what if we want multiple instances
					geometry->m_vertexCount = 0;
					geometry->m_indexCount = 0;

					//TODO alocate vertices and indices vectors
					geometry->m_vertexCount = pScene->mMeshes[i]->mNumVertices;

					/*aiColor3D mColor(0.3f, 0.3f, 0.3f);
					pScene->mMaterials[paiMesh->mMaterialIndex]->Get(AI_MATKEY_COLOR_DIFFUSE, mColor);
					float shininess = 1.0f;
					pScene->mMaterials[paiMesh->mMaterialIndex]->Get(AI_MATKEY_SHININESS, shininess);
					float transparency = 1.0f;
					pScene->mMaterials[paiMesh->mMaterialIndex]->Get(AI_MATKEY_OPACITY, transparency);*/

					if (pScene->mMaterials[paiMesh->mMaterialIndex]->GetTextureCount(aiTextureType_NORMALS) > 0)
						vertex_layout = &vnlayout;
					else
						vertex_layout = &vlayout;

					const aiVector3D Zero3D(0.0f, 0.0f, 0.0f);
					const aiColor4D One4D(1.0f, 1.0f, 1.0f, 1.0f);

					geometry->m_verticesSize = geometry->m_vertexCount * vertex_layout->GetVertexSize(0) / sizeof(float);
					geometry->m_vertices = new float[geometry->m_verticesSize];

					total_vertices += paiMesh->mNumVertices;
					int vertex_index = 0;

					for (unsigned int j = 0; j < paiMesh->mNumVertices; j++)
					{
						const aiVector3D* pPos = &(paiMesh->mVertices[j]);
						const aiVector3D* pNormal = paiMesh->HasNormals() ? &(paiMesh->mNormals[j]) : &Zero3D;
						const aiColor4D* pColor = paiMesh->HasVertexColors(0) ? &(paiMesh->mColors[0][j]) : &One4D;
						const aiVector3D* pTexCoord = (paiMesh->HasTextureCoords(0)) ? &(paiMesh->mTextureCoords[0][j]) : &Zero3D;
						const aiVector3D* pTangent = (paiMesh->HasTangentsAndBitangents()) ? &(paiMesh->mTangents[j]) : &Zero3D;
						const aiVector3D* pBiTangent = (paiMesh->HasTangentsAndBitangents()) ? &(paiMesh->mBitangents[j]) : &Zero3D;

						if (paiMesh->HasVertexColors(0))
						{
							int x = 0;
						}

						for (auto& component : vertex_layout->m_components[0])
						{
							switch (component) {
							case render::VERTEX_COMPONENT_POSITION:
								geometry->m_vertices[vertex_index++] = pPos->x * scale.x + center.x;
								geometry->m_vertices[vertex_index++] = -pPos->y * scale.y + center.y;
								geometry->m_vertices[vertex_index++] = pPos->z * scale.z + center.z;
								break;
							case render::VERTEX_COMPONENT_NORMAL:
								geometry->m_vertices[vertex_index++] = pNormal->x;
								geometry->m_vertices[vertex_index++] = -pNormal->y;
								geometry->m_vertices[vertex_index++] = pNormal->z;
								break;
							case render::VERTEX_COMPONENT_UV:
								geometry->m_vertices[vertex_index++] = pTexCoord->x * uvscale.s;
								geometry->m_vertices[vertex_index++] = pTexCoord->y * uvscale.t;
								break;
							case render::VERTEX_COMPONENT_COLOR:
								geometry->m_vertices[vertex_index++] = pColor->r;
								geometry->m_vertices[vertex_index++] = pColor->g;
								geometry->m_vertices[vertex_index++] = pColor->b;
								break;
							case render::VERTEX_COMPONENT_TANGENT:
								geometry->m_vertices[vertex_index++] = pTangent->x;
								geometry->m_vertices[vertex_index++] = pTangent->y;
								geometry->m_vertices[vertex_index++] = pTangent->z;
								break;
							case render::VERTEX_COMPONENT_BITANGENT:
								geometry->m_vertices[vertex_index++] = pBiTangent->x;
								geometry->m_vertices[vertex_index++] = pBiTangent->y;
								geometry->m_vertices[vertex_index++] = pBiTangent->z;
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

						dim.max.x = fmax(pPos->x, dim.max.x);
						dim.max.y = fmax(pPos->y, dim.max.y);
						dim.max.z = fmax(pPos->z, dim.max.z);

						dim.min.x = fmin(pPos->x, dim.min.x);
						dim.min.y = fmin(pPos->y, dim.min.y);
						dim.min.z = fmin(pPos->z, dim.min.z);
					}

					dim.size = dim.max - dim.min;

					//parts[i].vertexCount = paiMesh->mNumVertices;
					geometry->m_indexCount = paiMesh->mNumFaces * 3;
					geometry->m_indices = new uint32_t[geometry->m_indexCount];
					uint32_t indexBase = 0;//static_cast<uint32_t>(geometry->m_indices.size());
					int index_index = 0;
					for (unsigned int j = 0; j < paiMesh->mNumFaces; j++)
					{
						const aiFace& Face = paiMesh->mFaces[j];
						if (Face.mNumIndices != 3)
							continue;
						geometry->m_indices[index_index++] = indexBase + Face.mIndices[0];
						geometry->m_indices[index_index++] = indexBase + Face.mIndices[1];
						geometry->m_indices[index_index++] = indexBase + Face.mIndices[2];
						//parts[i].indexCount += 3;
						//geometry->m_indexCount += 3;
					}

					//if (pScene->mMaterials[paiMesh->mMaterialIndex]->GetTextureCount(aiTextureType_DIFFUSE) == 0)
					if(render_objects[paiMesh->mMaterialIndex])
					render_objects[paiMesh->mMaterialIndex]->AddGeometry(geometry);
				}

				timer.start();
				for (auto robj : render_objects)
				{
					if(robj)
					for (auto geo : robj->m_geometries)
					{
						geo->SetIndexBuffer(device->GetGeometryBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, copyQueue, geo->m_indexCount * sizeof(uint32_t), geo->m_indices));
						geo->SetVertexBuffer(device->GetGeometryBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, copyQueue, geo->m_verticesSize * sizeof(float), geo->m_vertices));
					}
				}
				timer.stop();
				uint64_t time = timer.elapsedMilliseconds();
				std::cout << "------time uploading: " << time << "\n";

				return render_objects;
			}
			else
			{
				printf("Error parsing '%s': '%s'\n", filename.c_str(), Importer.GetErrorString());
#if defined(__ANDROID__)
				LOGE("Error parsing '%s': '%s'", filename.c_str(), Importer.GetErrorString());
#endif
				return render_objects;
			}
		};

		/**
		* Loads a 3D model scene from a file into Vulkan buffers
		*
		* @param device Pointer to the Vulkan device used to generated the vertex and index buffers on
		* @param filename File to load (must be a model format supported by ASSIMP)
		* @param layout Vertex layout components (position, normals, tangents, etc.)
		* @param scale Load time scene scale
		* @param copyQueue Queue used for the memory staging copy commands (must support transfer)
		*/
		std::vector<RenderObject*> SceneLoader::LoadFromFile(const std::string& foldername, const std::string& filename, float scale, engine::render::VulkanDevice* device, VkQueue copyQueue
			, VkRenderPass renderPass, VkPipelineCache pipelineCache, bool deferred)
		{
			ModelCreateInfo2 modelCreateInfo(scale, 1.0f, 0.0f);
			return LoadFromFile2(foldername, filename, &modelCreateInfo, device, copyQueue, renderPass, pipelineCache, deferred);
		}

		void SceneLoader::CreateShadow(VkQueue copyQueue)
		{
			shadowmap = _device->GetDepthRenderTarget(SHADOWMAP_DIM, SHADOWMAP_DIM, true, VK_IMAGE_ASPECT_DEPTH_BIT, false);
			//shadowmap = _device->GetDepthRenderTarget(SHADOWMAP_DIM, SHADOWMAP_DIM, true);
			shadowmapColor = _device->GetColorRenderTarget(SHADOWMAP_DIM, SHADOWMAP_DIM, FB_COLOR_FORMAT);
			shadowPass = _device->GetRenderPass({ { shadowmapColor->m_format, shadowmapColor->m_descriptor.imageLayout}, { shadowmap->m_format, shadowmap->m_descriptor.imageLayout} });
			render::VulkanFrameBuffer* fb = _device->GetFrameBuffer(shadowPass->GetRenderPass(), SHADOWMAP_DIM, SHADOWMAP_DIM, { shadowmapColor->m_descriptor.imageView, shadowmap->m_descriptor.imageView });
			shadowPass->AddFrameBuffer(fb);
			shadowPass->SetClearColor({ 1.0f,1.0f,1.0f }, 0);
		}

		void SceneLoader::CreateShadowObjects(VkPipelineCache pipelineCache)
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

		void SceneLoader::DrawShadowsInSeparatePass(VkCommandBuffer command_buffer)
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

		render::VulkanDescriptorSetLayout* SceneLoader::GetDescriptorSetlayout(std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> layoutBindigs)
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

		void SceneLoader::Update(float timer)
		{
			//light_pos.x = cos(glm::radians(timer * 360.0f)) * 40.0f;
			//light_pos.y = -165.0f + sin(glm::radians(timer * 360.0f)) * 20.0f;
			//light_pos.z = 25.0f + sin(glm::radians(timer * 360.0f)) * 5.0f;//george change it back

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

			uniform_manager.UpdateGlobalParams(UNIFORM_PROJECTION, &m_camera->matrices.perspective, 0, sizeof(m_camera->matrices.perspective));
			uniform_manager.UpdateGlobalParams(UNIFORM_VIEW, &m_camera->matrices.view, 0, sizeof(m_camera->matrices.view));
			uniform_manager.UpdateGlobalParams(UNIFORM_LIGHT0_SPACE_BIASED, &biasedDepthMVP, 0, sizeof(biasedDepthMVP));
			uniform_manager.UpdateGlobalParams(UNIFORM_LIGHT0_POSITION, &light_pos, 0, sizeof(light_pos));
			glm::vec3 cucu = m_camera->position;
			cucu.y = -cucu.y;
			uniform_manager.UpdateGlobalParams(UNIFORM_CAMERA_POSITION, &cucu, 0, sizeof(m_camera->position));

			/*glm::mat4 viewproj = m_camera->matrices.perspective * m_camera->matrices.view;
			uniform_manager.UpdateGlobalParams(UNIFORM_PROJECTION_VIEW, &viewproj, 0, sizeof(viewproj));
			glm::vec4 bias_near_far_pow = glm::vec4(0.002f, m_camera->getNearClip(), m_camera->getFarClip(), 1.0f);
			uniform_manager.UpdateGlobalParams(UNIFORM_LIGHT0_SPACE_BIASED, &bias_near_far_pow, 0, sizeof(bias_near_far_pow));*/
			uniform_manager.Update();
		}

		void SceneLoader::UpdateView(float timer)
		{
			uniform_manager.UpdateGlobalParams(UNIFORM_VIEW, &m_camera->matrices.view, 0, sizeof(m_camera->matrices.view));
			glm::vec3 cucu = m_camera->position;
			cucu.y = -cucu.y;
			uniform_manager.UpdateGlobalParams(UNIFORM_CAMERA_POSITION, &cucu, 0, sizeof(m_camera->position));

			/*glm::mat4 viewproj = m_camera->matrices.perspective * m_camera->matrices.view;
			uniform_manager.UpdateGlobalParams(UNIFORM_PROJECTION_VIEW, &viewproj, 0, sizeof(viewproj));
			glm::vec4 bias_near_far_pow = glm::vec4(0.002f, m_camera->getNearClip(), m_camera->getFarClip(), 1.0f);
			uniform_manager.UpdateGlobalParams(UNIFORM_LIGHT0_SPACE_BIASED, &bias_near_far_pow, 0, sizeof(bias_near_far_pow));*/

			uniform_manager.Update();
		}

		SceneLoader::~SceneLoader()
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