#pragma once
#include "RenderObject.h"

#include <assimp/Importer.hpp> 
#include <assimp/scene.h>     
#include <assimp/postprocess.h>
#include <assimp/cimport.h>

#include <glm/glm.hpp>


namespace engine
{
	namespace scene
	{
		/** @brief Used to parametrize model loading */
		struct ModelCreateInfo {
			glm::vec3 center;
			glm::vec3 scale;
			glm::vec2 uvscale;
			VkMemoryPropertyFlags memoryPropertyFlags = 0;

			ModelCreateInfo() : center(glm::vec3(0.0f)), scale(glm::vec3(1.0f)), uvscale(glm::vec2(1.0f)) {};

			ModelCreateInfo(glm::vec3 scale, glm::vec2 uvscale, glm::vec3 center)
			{
				this->center = center;
				this->scale = scale;
				this->uvscale = uvscale;
			}

			ModelCreateInfo(float scale, float uvscale, float center)
			{
				this->center = glm::vec3(center);
				this->scale = glm::vec3(scale);
				this->uvscale = glm::vec2(uvscale);
			}

		};

		struct GeometryRT : public Geometry
		{
			// Structure holding the material
			struct MaterialObj
			{
				glm::vec3 diffuse = glm::vec3(0.7f, 0.7f, 0.7f);
				int textureID = -1;
			};
			std::vector<MaterialObj> m_materials;
			std::vector<std::string> m_textures;
			std::vector<int32_t>     m_matIndx;

			render::VulkanBuffer* materialsBuffer;
			render::VulkanBuffer* materialsIndexBuffer;
		};

		class SimpleModelRT : public RenderObject
		{
		public:
			/** @brief Stores vertex and index base and counts for each part of a model */
			struct ModelPart {
				uint32_t vertexBase;
				uint32_t vertexCount;
				uint32_t indexBase;
				uint32_t indexCount;
			};
			std::vector<ModelPart> parts;

			static const int defaultFlags = aiProcess_FlipWindingOrder | aiProcess_Triangulate | aiProcess_PreTransformVertices | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals;

			struct Dimension
			{
				glm::vec3 min = glm::vec3(FLT_MAX);
				glm::vec3 max = glm::vec3(-FLT_MAX);
				glm::vec3 size;
			} dim;

			/** @brief Release all Vulkan resources of this model */
			void destroy()
			{
				//assert(m_device);

			}
			bool LoadGeometry(const std::string& filename, render::VulkanVertexLayout* vertex_layout, float scale, int instance_no = 1, glm::vec3 atPos = glm::vec3(0.0f), glm::vec3 normalsCoefficient = glm::vec3(1.0f), glm::vec2 uvCoefficient = glm::vec2(1.0f));
		};
	}
}