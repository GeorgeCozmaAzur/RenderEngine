#include "SimpleModel.h"

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
		std::vector<render::MeshData*> SimpleModel::LoadGeometry(const std::string& filename, render::VertexLayout* vertex_layout, float scale, int instance_no, glm::vec3 atPos, glm::vec3 normalsCoefficient, glm::vec2 uvCoefficient)
		{
			std::vector<render::MeshData*> returnVector;

			Assimp::Importer Importer;
			const aiScene* pScene;

			// Load file
#if defined(__ANDROID__)
			// Meshes are stored inside the apk on Android (compressed)
			// So they need to be loaded via the asset manager

			AAsset* asset = AAssetManager_open(androidApp->activity->assetManager, filename.c_str(), AASSET_MODE_STREAMING);
			if (!asset) {
				LOGE("Could not load mesh from \"%s\"!", filename.c_str());
				engine::tools::exitFatal("Error: Could not open model file" + filename, -1);
				return false;
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
			pScene = Importer.ReadFile(filename.c_str(), defaultFlags);
			if (!pScene) {
				std::string error = Importer.GetErrorString();
				engine::tools::exitFatal(error + "\n\nThe file may be part of the additional asset pack.\n\nRun \"download_assets.py\" in the repository root to download the latest version.", -1);
			}
#endif

			_vertexLayout = vertex_layout;

			if (pScene)
			{
				parts.clear();
				parts.resize(pScene->mNumMeshes);

				returnVector.resize(pScene->mNumMeshes);

				// Load meshes
				for (unsigned int i = 0; i < pScene->mNumMeshes; i++)
				{
					render::MeshData* geometry = new render::MeshData();
					geometry->m_instanceNo = instance_no;
					geometry->m_vertexCount = 0;
					geometry->m_indexCount = 0;
					geometry->m_vertexCount = pScene->mMeshes[i]->mNumVertices;
					geometry->m_verticesSize = geometry->m_vertexCount * vertex_layout->GetVertexSize(0) / sizeof(float);
					geometry->m_vertices = new float[geometry->m_verticesSize];

					const aiMesh* paiMesh = pScene->mMeshes[i];

					aiColor3D pColor(0.f, 0.f, 0.f);
					pScene->mMaterials[paiMesh->mMaterialIndex]->Get(AI_MATKEY_COLOR_DIFFUSE, pColor);

					const aiVector3D Zero3D(0.0f, 0.0f, 0.0f);

					int vertex_index = 0;

					for (unsigned int j = 0; j < paiMesh->mNumVertices; j++)
					{
						const aiVector3D* pPos = &(paiMesh->mVertices[j]);
						const aiVector3D* pNormal = paiMesh->HasNormals() ? &(paiMesh->mNormals[j]) : &Zero3D;
						const aiVector3D* pTexCoord = (paiMesh->HasTextureCoords(0)) ? &(paiMesh->mTextureCoords[0][j]) : &Zero3D;
						const aiVector3D* pTangent = (paiMesh->HasTangentsAndBitangents()) ? &(paiMesh->mTangents[j]) : &Zero3D;
						const aiVector3D* pBiTangent = (paiMesh->HasTangentsAndBitangents()) ? &(paiMesh->mBitangents[j]) : &Zero3D;

						for (auto& component : vertex_layout->m_components[0])
						{
							switch (component) {
							case render::VERTEX_COMPONENT_POSITION:
								geometry->m_vertices[vertex_index++] = atPos.x + pPos->x * scale;
								geometry->m_vertices[vertex_index++] = atPos.y - pPos->y * scale;
								geometry->m_vertices[vertex_index++] = atPos.z + pPos->z * scale;
								break;
							case render::VERTEX_COMPONENT_NORMAL:
								geometry->m_vertices[vertex_index++] = pNormal->x * normalsCoefficient.x;
								geometry->m_vertices[vertex_index++] = -pNormal->y * normalsCoefficient.y;
								geometry->m_vertices[vertex_index++] = pNormal->z * normalsCoefficient.z;
								break;
							case render::VERTEX_COMPONENT_UV:
								geometry->m_vertices[vertex_index++] = pTexCoord->x * uvCoefficient.s;
								geometry->m_vertices[vertex_index++] = pTexCoord->y * uvCoefficient.t;
								break;
							case render::VERTEX_COMPONENT_COLOR:
								geometry->m_vertices[vertex_index++] = pColor.r;
								geometry->m_vertices[vertex_index++] = pColor.g;
								geometry->m_vertices[vertex_index++] = pColor.b;
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

						dim.max.x = fmax(pPos->x * scale + atPos.x, dim.max.x);
						dim.max.y = fmax(pPos->y * scale + atPos.y, dim.max.y);
						dim.max.z = fmax(pPos->z * scale + atPos.z, dim.max.z);

						dim.min.x = fmin(pPos->x * scale + atPos.x, dim.min.x);
						dim.min.y = fmin(pPos->y * scale + atPos.y, dim.min.y);
						dim.min.z = fmin(pPos->z * scale + atPos.z, dim.min.z);
					}

					dim.size = dim.max - dim.min;

					BoundingBox* box = new BoundingBox(dim.min, dim.max, glm::length(dim.size));

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

					m_boundingBoxes.push_back(box);

					returnVector[i] = geometry;
				}
			}
			else
			{
				printf("Error parsing '%s': '%s'\n", filename.c_str(), Importer.GetErrorString());
#if defined(__ANDROID__)
				LOGE("Error parsing '%s': '%s'", filename.c_str(), Importer.GetErrorString());
#endif
			}
			return returnVector;
		};
	}
}