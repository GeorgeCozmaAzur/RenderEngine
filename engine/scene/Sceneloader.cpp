#include "SceneLoader.h"
#include <assimp/Importer.hpp> 
#include <assimp/scene.h>  
#include <assimp/cimport.h>
#include "render/VulkanDevice.hpp"
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
		#define SHADOWMAP_DIM 2048
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
			render::VulkanBuffer* vub = uniform_manager.GetGlobalUniformBuffer({ UNIFORM_PROJECTION ,UNIFORM_VIEW ,UNIFORM_LIGHT0_POSITION, UNIFORM_CAMERA_POSITION });
			render::VulkanBuffer* smvub = uniform_manager.GetGlobalUniformBuffer({ UNIFORM_PROJECTION ,UNIFORM_VIEW, UNIFORM_LIGHT0_SPACE ,UNIFORM_LIGHT0_POSITION, UNIFORM_CAMERA_POSITION });
			scene_uniform_buffer = _device->GetUniformBuffer(sizeof(uboVSscene));
			VK_CHECK_RESULT(scene_uniform_buffer->map());

			std::string shaderfolder = deferred ? std::string("shaders/basicdeferred/") : std::string("shaders/basic/");

			std::string basic_vertex_file(engine::tools::getAssetPath() + shaderfolder + std::string("phong.vert.spv"));
			std::string basic_fragment_file(engine::tools::getAssetPath() + shaderfolder + std::string("phong.frag.spv"));
			std::string textured_fragment_file(engine::tools::getAssetPath() + shaderfolder + std::string("phongtextured.frag.spv"));
			std::string normalmap_vertex_file(engine::tools::getAssetPath() + shaderfolder + std::string("normalmap.vert.spv"));
			std::string normalmap_fragment_file(engine::tools::getAssetPath() + shaderfolder + std::string("normalmap.frag.spv"));

			std::string normalmap_shadowmap_vertex_file(engine::tools::getAssetPath() + shaderfolder + std::string("normalmapshadowmap.vert.spv"));
			std::string normalmap_shadowmap_fragment_file(engine::tools::getAssetPath() + shaderfolder + std::string("normalmapshadowmap.frag.spv"));

			render::VulkanPipeline* textured_pipeline = nullptr;
			render::VulkanPipeline* simple_pipeline = nullptr;
			render::VulkanPipeline* simple_pipeline_trans = nullptr;
			render::VulkanPipeline* normalmap_pipeline = nullptr;
			render::VulkanPipeline* normalmap_shadowmap_pipeline = nullptr;

			std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStates;
			blendAttachmentStates.push_back(engine::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE));
			if (deferred)
				blendAttachmentStates.push_back(engine::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE));

			std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStatestrans;
			blendAttachmentStatestrans.push_back(engine::initializers::pipelineColorBlendAttachmentState(0xf, VK_TRUE));
			if (deferred)
				blendAttachmentStatestrans.push_back(engine::initializers::pipelineColorBlendAttachmentState(0xf, VK_TRUE));

			std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> simple_bindings
			{
				{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
				{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT}
			};
			std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> textured_bindings
			{
				{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
				{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
			};
			std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> doubled_textured_bindings
			{
				{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
				{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
				{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
			};
			std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> triple_textured_bindings
			{
				{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
				{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
				{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT},
				{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
			};

			render::VulkanDescriptorSetLayout* simple_layout = device->GetDescriptorSetLayout(simple_bindings);
			render::VulkanDescriptorSetLayout* textured_layout = device->GetDescriptorSetLayout(textured_bindings);
			render::VulkanDescriptorSetLayout* doubled_textured_layout = device->GetDescriptorSetLayout(doubled_textured_bindings);
			render::VulkanDescriptorSetLayout* triple_textured_layout = device->GetDescriptorSetLayout(triple_textured_bindings);

			CreateShadow();

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
				if (device->features.textureCompressionBC) {
					texFormatSuffix = "_bc3_unorm";
					CompressedTexFormat = VK_FORMAT_BC3_UNORM_BLOCK;
				}
				else if (device->features.textureCompressionASTC_LDR) {
					texFormatSuffix = "_astc_8x8_unorm";
					CompressedTexFormat = VK_FORMAT_ASTC_8x8_UNORM_BLOCK;
				}
				else if (device->features.textureCompressionETC2) {
					texFormatSuffix = "_etc2_unorm";
					CompressedTexFormat = VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;
				}
				else {
					engine::tools::exitFatal("Device does not support any compressed texture format!", VK_ERROR_FEATURE_NOT_PRESENT);
				}

				//descriptorSets.resize(pScene->mNumMaterials);//TODO

				render_objects.resize(pScene->mNumMaterials);//TODO

				//TODO see exactly how many descriptors are needed
				std::vector<VkDescriptorPoolSize> poolSizes =
				{
					engine::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2 * static_cast<uint32_t>(render_objects.size())),
					engine::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2 * static_cast<uint32_t>(render_objects.size())),
					engine::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 2)
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

						std::vector<VkDescriptorImageInfo*> tex_descriptors;
						tex_descriptors.push_back(&tex->m_descriptor);

						if (pScene->mMaterials[i]->GetTextureCount(aiTextureType_NORMALS) > 0)
						{
							std::string texfilenamen = std::string(texturefilen.C_Str());
							tex = device->GetTexture(foldername + texfilenamen, texFormat, copyQueue);
							tex_descriptors.push_back(&tex->m_descriptor);

							tex_descriptors.push_back(&shadowmap->m_descriptor);
							if (!normalmap_shadowmap_pipeline)
								normalmap_shadowmap_pipeline = device->GetPipeline(triple_textured_layout->m_descriptorSetLayout, vnlayout.m_vertexInputBindings, vnlayout.m_vertexInputAttributes,
									normalmap_shadowmap_vertex_file, normalmap_shadowmap_fragment_file, renderPass, pipelineCache, false, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, nullptr, blendAttachmentStates.size(), blendAttachmentStates.data());
							render_objects[i]->SetDescriptorSetLayout(triple_textured_layout);
							render_objects[i]->_vertexLayout = &vnlayout;
							render_objects[i]->AddPipeline(normalmap_shadowmap_pipeline);
							render::VulkanDescriptorSet* desc = device->GetDescriptorSet({ &scene_uniform_buffer->m_descriptor }, tex_descriptors,
								triple_textured_layout->m_descriptorSetLayout, triple_textured_layout->m_setLayoutBindings);

							render_objects[i]->AddDescriptor(desc);
						}
						else
						{
							if (!textured_pipeline)
								textured_pipeline = device->GetPipeline(textured_layout->m_descriptorSetLayout, vlayout.m_vertexInputBindings, vlayout.m_vertexInputAttributes,
									basic_vertex_file, textured_fragment_file, renderPass, pipelineCache,
									false, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, nullptr, blendAttachmentStates.size(), blendAttachmentStates.data());
							render_objects[i]->AddPipeline(textured_pipeline);
							render::VulkanDescriptorSet* desc = device->GetDescriptorSet({ &vub->m_descriptor }, { &tex->m_descriptor },
								textured_layout->m_descriptorSetLayout, textured_layout->m_setLayoutBindings);
							render_objects[i]->SetDescriptorSetLayout(textured_layout);
							render_objects[i]->_vertexLayout = &vlayout;
							render_objects[i]->AddDescriptor(desc);
						}
					}
					else
					{
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

						if (fdata.transparency != 1.0f)
						{
							if (!simple_pipeline_trans)
								simple_pipeline_trans = device->GetPipeline(simple_layout->m_descriptorSetLayout, vlayout.m_vertexInputBindings, vlayout.m_vertexInputAttributes,
									basic_vertex_file, basic_fragment_file, renderPass, pipelineCache, true,
									VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, nullptr, blendAttachmentStatestrans.size(), blendAttachmentStatestrans.data());
							render_objects[i]->AddPipeline(simple_pipeline_trans);
						}
						else
						{
							if (!simple_pipeline)
								simple_pipeline = device->GetPipeline(simple_layout->m_descriptorSetLayout, vlayout.m_vertexInputBindings, vlayout.m_vertexInputAttributes,
									basic_vertex_file, basic_fragment_file, renderPass, pipelineCache,
									false, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, nullptr, blendAttachmentStates.size(), blendAttachmentStates.data());
							render_objects[i]->AddPipeline(simple_pipeline);
						}

						render::VulkanDescriptorSet* desc = device->GetDescriptorSet({ &vub->m_descriptor, &frag_buffer->m_descriptor }, { },
							simple_layout->m_descriptorSetLayout, simple_layout->m_setLayoutBindings);
						render_objects[i]->AddDescriptor(desc);
						render_objects[i]->SetDescriptorSetLayout(simple_layout);
						render_objects[i]->_vertexLayout = &vlayout;
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
					render_objects[paiMesh->mMaterialIndex]->AddGeometry(geometry);
				}

				timer.start();
				for (auto robj : render_objects)
				{
					for (auto geo : robj->m_geometries)
					{
						geo->SetIndexBuffer(device->GetGeometryBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, copyQueue, geo->m_indexCount * sizeof(uint32_t), geo->m_indices));
						geo->SetVertexBuffer(device->GetGeometryBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, copyQueue, geo->m_verticesSize * sizeof(float), geo->m_vertices));
					}
				}
				timer.stop();
				uint64_t time = timer.elapsedMilliseconds();
				std::cout << "------time uploading: " << time << "\n";

				CreateShadowObjects(pipelineCache);

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

		void SceneLoader::CreateShadow()
		{
			shadowmap = _device->GetDepthRenderTarget(SHADOWMAP_DIM, SHADOWMAP_DIM, true);
			shadowPass = _device->GetRenderPass({ { shadowmap->m_format, shadowmap->m_descriptor.imageLayout} });
			render::VulkanFrameBuffer* fb = _device->GetFrameBuffer(shadowPass->GetRenderPass(), SHADOWMAP_DIM, SHADOWMAP_DIM, { shadowmap->m_descriptor.imageView });
			shadowPass->AddFrameBuffer(fb);

			shadow_uniform_buffer = _device->GetBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				sizeof(uboShadowOffscreenVS));
			VK_CHECK_RESULT(shadow_uniform_buffer->map());
		}

		void SceneLoader::CreateShadowObjects(VkPipelineCache pipelineCache)
		{
			std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> offscreenbindings
			{
				{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT}
			};
			render::VulkanDescriptorSetLayout* desc_layout = _device->GetDescriptorSetLayout(offscreenbindings);
			std::vector<render::VertexLayout*> vlayouts;

			for (auto ro : render_objects)
			{
				render::VertexLayout* l = ro->_vertexLayout;
				//it = std::find(vlayouts.begin(), vlayouts.end(), l);
				int index = -1;
				for (int i = 0; i < vlayouts.size(); i++)
				{
					if (vlayouts[i] == l)
						index = i;
				}

				if (index != -1)
				{
					//shadow_objects[index]->m_geometries.insert(shadow_objects[index]->m_geometries.end(), ro->m_geometries.begin(), ro->m_geometries.end());
					shadow_objects[index]->AdoptGeometriesFrom(*ro);
				}
				else
				{
					RenderObject* sro = new RenderObject;
					sro->_vertexLayout = l;
					sro->SetDescriptorSetLayout(desc_layout);

					//sro->m_geometries.insert(sro->m_geometries.end(), ro->m_geometries.begin(), ro->m_geometries.end());
					sro->AdoptGeometriesFrom(*ro);

					std::string sm_vertex_file(engine::tools::getAssetPath() + std::string("shaders/shadowmapping/offscreen.vert.spv"));
					render::VulkanPipeline* p = _device->GetPipeline(desc_layout->m_descriptorSetLayout, l->m_vertexInputBindings, l->m_vertexInputAttributes,
						sm_vertex_file, "", shadowPass->GetRenderPass(), pipelineCache);
					sro->AddPipeline(p);

					render::VulkanDescriptorSet* set = _device->GetDescriptorSet({ &shadow_uniform_buffer->m_descriptor }, {},
						desc_layout->m_descriptorSetLayout, desc_layout->m_setLayoutBindings);
					sro->AddDescriptor(set);

					vlayouts.push_back(l);
					shadow_objects.push_back(sro);
				}

			}
		}

		void SceneLoader::DrawShadowsInSeparatePass(VkCommandBuffer command_buffer)
		{
			float depthBiasConstant = 1.25f;
			// Slope depth bias factor, applied depending on polygon's slope
			float depthBiasSlope = 1.75f;
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

		void SceneLoader::Update(float timer)
		{
			//light_pos.x = cos(glm::radians(timer * 360.0f)) * 40.0f;
			//light_pos.y = -165.0f + sin(glm::radians(timer * 360.0f)) * 20.0f;
			//light_pos.z = 25.0f + sin(glm::radians(timer * 360.0f)) * 5.0f;//george change it back

			float zNear = 1.0f;
			float zFar = 906.0f;
			// Matrix from light's point of view
			glm::mat4 depthProjectionMatrix = glm::perspective(glm::radians(lightFOV), 1.0f, zNear, zFar);
			glm::mat4 depthViewMatrix = glm::lookAt(glm::vec3(light_pos), glm::vec3(0.0f), glm::vec3(0, 1, 0));
			glm::mat4 depthModelMatrix = glm::mat4(1.0f);

			uboShadowOffscreenVS.depthMVP = depthProjectionMatrix * depthViewMatrix * depthModelMatrix;

			memcpy(shadow_uniform_buffer->m_mapped, &uboShadowOffscreenVS, sizeof(uboShadowOffscreenVS));

			uboVSscene.projection = m_camera->matrices.perspective;
			uboVSscene.view = m_camera->matrices.view;
			uboVSscene.depthBiasMVP = uboShadowOffscreenVS.depthMVP;
			uboVSscene.cameraPos = -m_camera->position;
			uboVSscene.lightPos = light_pos;
			memcpy(scene_uniform_buffer->m_mapped, &uboVSscene, sizeof(uboVSscene));


			uniform_manager.UpdateGlobalParams(UNIFORM_PROJECTION, &m_camera->matrices.perspective, 0, sizeof(m_camera->matrices.perspective));
			//uniform_manager.UpdateGlobalParams(UNIFORM_VIEW, &depthViewMatrix, 0, sizeof(depthViewMatrix));
			uniform_manager.UpdateGlobalParams(UNIFORM_VIEW, &m_camera->matrices.view, 0, sizeof(m_camera->matrices.view));
			uniform_manager.UpdateGlobalParams(UNIFORM_LIGHT0_SPACE, &uboShadowOffscreenVS.depthMVP, 0, sizeof(uboShadowOffscreenVS.depthMVP));
			uniform_manager.UpdateGlobalParams(UNIFORM_LIGHT0_POSITION, &light_pos, 0, sizeof(light_pos));
			glm::vec3 cucu = -m_camera->position;
			uniform_manager.UpdateGlobalParams(UNIFORM_CAMERA_POSITION, &cucu, 0, sizeof(m_camera->position));
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