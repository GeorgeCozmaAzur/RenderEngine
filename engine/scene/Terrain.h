#pragma once
#include "scene/RenderObject.h" 
#include <glm/glm.hpp>

namespace engine
{
	namespace scene
	{
		class Terrain : public RenderObject
		{
		protected:
			int m_width; 
			int m_length; 
			float** m_heights; 
			glm::vec3** m_normals;
			bool computedNormals; //Whether normals is up-to-date
		public:
			Terrain() {}

			~Terrain();

			void Destroy();

			int Width() {
				return m_width;
			}

			int Length() {
				return m_length;
			}

			void SetHeight(int x, int z, float y) {
				m_heights[z][x] = y;
				computedNormals = false;
			}

			float GetHeight(int x, int z) {
				return m_heights[z][x];
			}

			void ComputeNormals();

			uint32_t* BuildPatchIndices(int offsetX, int offsetY, int width, int heights, int& size);

			glm::vec3 GetNormal(int x, int z) {
				if (!computedNormals) {
					ComputeNormals();
				}
				return m_normals[z][x];
			}

			void LoadHeightmap(const std::string& filename, float scale);

			//static Terrain* LoadTerrain(std::string filename, float height, engine::EngineDevice *vdevice, VkQueue copyQueue);
			bool LoadGeometry(const std::string& filename, render::VertexLayout* vertex_layout, float scale, int instance_no, glm::vec3 atPos = glm::vec3(0.0f));
		};
	}
}