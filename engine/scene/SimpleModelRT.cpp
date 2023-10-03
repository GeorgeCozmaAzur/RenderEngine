#include "SimpleModelRT.h"

namespace engine
{
	namespace scene
	{
		bool SimpleModelRT::LoadGeometry(const std::string& filename, render::VertexLayout* vertex_layout, float scale, int instance_no, glm::vec3 atPos, glm::vec3 normalsCoefficient, glm::vec2 uvCoefficient)
		{
			//m_device = device->logicalDevice;

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

				

				GeometryRT* geometry = new GeometryRT();
				geometry->m_instanceNo = instance_no;
				geometry->m_vertexCount = 0;
				geometry->m_indexCount = 0;

				for (size_t i = 0; i < pScene->mNumMaterials; i++)
				{
					aiString texturefile;
					pScene->mMaterials[i]->GetTexture(aiTextureType_DIFFUSE, 0, &texturefile);
					aiColor3D pColor(0.f, 0.f, 0.f);
					pScene->mMaterials[i]->Get(AI_MATKEY_COLOR_DIFFUSE, pColor);

					GeometryRT::MaterialObj m;
					m.diffuse.x = pColor.r;
					m.diffuse.y = pColor.g;
					m.diffuse.z = pColor.b;
					if (texturefile.length > 0)
					{
						geometry->m_textures.push_back(texturefile.C_Str());
						m.textureID = static_cast<int>(geometry->m_textures.size()) - 1;
					}
					geometry->m_materials.push_back(m);
				}

				for (unsigned int i = 0; i < pScene->mNumMeshes; i++)
				{
					geometry->m_vertexCount += pScene->mMeshes[i]->mNumVertices;
					geometry->m_indexCount += pScene->mMeshes[i]->mNumFaces * 3;					
					
				}
				geometry->m_verticesSize = geometry->m_vertexCount * vertex_layout->GetVertexSize(0) / sizeof(float);
				geometry->m_vertices = new float[geometry->m_verticesSize];

				geometry->m_indices = new uint32_t[geometry->m_indexCount];

				int vertex_index = 0;
				int index_index = 0;
				int faces_index = 0;

				uint32_t indexBase = 0;//static_cast<uint32_t>(geometry->m_indices.size());

				geometry->m_matIndx.resize(geometry->m_indexCount/3);

				// Load meshes
				for (unsigned int i = 0; i < pScene->mNumMeshes; i++)
				{
					
					const aiMesh* paiMesh = pScene->mMeshes[i];

					aiColor3D pColor(0.f, 0.f, 0.f);
					pScene->mMaterials[paiMesh->mMaterialIndex]->Get(AI_MATKEY_COLOR_DIFFUSE, pColor);

					const aiVector3D Zero3D(0.0f, 0.0f, 0.0f);

					//geometry->m_matIndx.push_back(paiMesh->mMaterialIndex);//george

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

					
					
					
					for (unsigned int j = 0; j < paiMesh->mNumFaces; j++)
					{
						const aiFace& Face = paiMesh->mFaces[j];
						if (Face.mNumIndices != 3)
							continue;
						geometry->m_indices[index_index++] = indexBase + Face.mIndices[0];
						geometry->m_indices[index_index++] = indexBase + Face.mIndices[1];
						geometry->m_indices[index_index++] = indexBase + Face.mIndices[2];

						geometry->m_matIndx[faces_index++] = paiMesh->mMaterialIndex;
						//parts[i].indexCount += 3;
						//geometry->m_indexCount += 3;
					}
					indexBase += pScene->mMeshes[i]->mNumVertices;

					m_boundingBoxes.push_back(box);
				}
				
				m_geometries.push_back(geometry);

				return true;
			}
			else
			{
				printf("Error parsing '%s': '%s'\n", filename.c_str(), Importer.GetErrorString());
#if defined(__ANDROID__)
				LOGE("Error parsing '%s': '%s'", filename.c_str(), Importer.GetErrorString());
#endif
				return false;
			}
		};
	}
}