#include "Terrain.h"

#include <stb_image.h>


namespace engine
{
	namespace scene
	{
		bool Terrain::LoadHeightmap(const std::string& filename, float scale)
		{
			int texWidth, texHeight, texChannels;
			stbi_uc* pixels = stbi_load(filename.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

			if (!pixels)
				return false;

			int m_imageSize = texWidth * texHeight * 4;

			m_width = static_cast<uint32_t>(texWidth);
			m_length = static_cast<uint32_t>(texHeight);

			char* m_ram_data = new char[m_imageSize];
			memcpy(m_ram_data, pixels, m_imageSize);

			m_heights = new float* [m_length];
			for (int i = 0; i < m_length; i++) {
				m_heights[i] = new float[m_width];
			}

			for (int y = 0; y < m_length; y++) {
				for (int x = 0; x < m_width; x++) {
					unsigned char color =
						(unsigned char)m_ram_data[4 * (y * m_width + x)];
					float h = scale * (((color) / 255.0f));
					SetHeight(x, y, h);
				}
			}

			m_normals = new glm::vec3 * [m_length];
			for (int i = 0; i < m_length; i++) {
				m_normals[i] = new glm::vec3[m_width];
			}

			computedNormals = false;

			ComputeNormals();

			return true;
		}

		void Terrain::ComputeNormals() {
			if (computedNormals) {
				return;
			}

			//Compute the rough version of the normals
			glm::vec3** normals2 = new glm::vec3 * [m_length];
			for (int i = 0; i < m_length; i++) {
				normals2[i] = new glm::vec3[m_width];
			}

			for (int z = 0; z < m_length; z++) {
				for (int x = 0; x < m_width; x++) {
					glm::vec3 sum(0.0f, 0.0f, 0.0f);

					glm::vec3 out;
					if (z > 0) {
						out = glm::vec3(0.0f, m_heights[z - 1][x] - m_heights[z][x], -1.0f);
					}
					glm::vec3 in;
					if (z < m_length - 1) {
						in = glm::vec3(0.0f, m_heights[z + 1][x] - m_heights[z][x], 1.0f);
					}
					glm::vec3 left;
					if (x > 0) {
						left = glm::vec3(-1.0f, m_heights[z][x - 1] - m_heights[z][x], 0.0f);
					}
					glm::vec3 right;
					if (x < m_width - 1) {
						right = glm::vec3(1.0f, m_heights[z][x + 1] - m_heights[z][x], 0.0f);
					}


					if (x > 0 && z > 0) {
						sum += glm::normalize(glm::cross(out, left));
					}
					if (x > 0 && z < m_length - 1) {
						sum += glm::normalize(glm::cross(left, in));
					}
					if (x < m_width - 1 && z < m_length - 1) {
						sum += glm::normalize(glm::cross(in, right));
					}
					if (x < m_width - 1 && z > 0) {
						sum += glm::normalize(glm::cross(right, out));
					}

					normals2[z][x] = glm::normalize(sum);
				}
			}

			//Smooth out the normals
			const float FALLOUT_RATIO = 0.5f;
			for (int z = 0; z < m_length; z++) {
				for (int x = 0; x < m_width; x++) {
					glm::vec3 sum = normals2[z][x];

					if (x > 0) {
						sum += normals2[z][x - 1] * FALLOUT_RATIO;
					}
					if (x < m_width - 1) {
						sum += normals2[z][x + 1] * FALLOUT_RATIO;
					}
					if (z > 0) {
						sum += normals2[z - 1][x] * FALLOUT_RATIO;
					}
					if (z < m_length - 1) {
						sum += normals2[z + 1][x] * FALLOUT_RATIO;
					}

					if (glm::length(sum) == 0) {
						sum = glm::vec3(0.0f, 1.0f, 0.0f);
					}
					m_normals[z][x] = glm::normalize(sum);
				}
			}

			for (int i = 0; i < m_length; i++) {
				delete[] normals2[i];
			}
			delete[] normals2;

			computedNormals = true;
		}

		std::vector<render::MeshData*> Terrain::LoadGeometry(const std::string& filename, render::VulkanVertexLayout* vertex_layout, float scale, int instance_no, glm::vec3 atPos)
		{
			std::vector<render::MeshData*> returnVector;
			_vertexLayout = vertex_layout;

			LoadHeightmap(filename, scale);

			render::MeshData* geometry = new render::MeshData();
			geometry->m_instanceNo = 1;
			geometry->m_vertexCount = Length() * Width();
			geometry->m_indexCount = 0;

			const int k_noVerts = Length() * Width();
			geometry->m_verticesSize = k_noVerts * vertex_layout->GetVertexSize(0) / sizeof(float);
			geometry->m_vertices = new float[geometry->m_verticesSize];

			glm::vec3 min = glm::vec3(FLT_MAX);
			glm::vec3 max = glm::vec3(-FLT_MAX);
			glm::vec3 size;

			//these will be used to compute tangents
			std::vector<glm::vec3> positions;
			std::vector<glm::vec3> normals;
			std::vector<glm::vec3> tangents;
			std::vector<glm::vec3> bitangents;
			std::vector<glm::vec2> uvs;
			bool hastangents = false;

			int vindex = 0;
			for (int wid = 0; wid < Width(); ++wid)
			{
				float v = (Length() - ((float)wid)) / (float)Width();

				for (int len = 0; len < Length(); ++len)
				{
					float u = ((float)len) / (float)Length();

					int current = wid * Length() + len;
					float height = GetHeight(wid, len);
					glm::vec3 pPos = glm::vec3((float)wid * 5.0 + atPos.x, -height + atPos.y, (float)len * 5.0 + atPos.z);

					for (auto& component : vertex_layout->m_components[0])
					{
						switch (component) {
						case render::VERTEX_COMPONENT_POSITION:
							geometry->m_vertices[vindex++] = pPos.x;
							geometry->m_vertices[vindex++] = pPos.y;
							geometry->m_vertices[vindex++] = pPos.z;
							positions.push_back(pPos);
							break;
						case render::VERTEX_COMPONENT_NORMAL:
						{
							glm::vec3 normal = GetNormal(wid, len);
							geometry->m_vertices[vindex++] = normal[0];
							geometry->m_vertices[vindex++] = -normal[1];
							geometry->m_vertices[vindex++] = normal[2];
							normals.push_back(normal);
							break;
						}
						case render::VERTEX_COMPONENT_UV:
							geometry->m_vertices[vindex++] = v;
							geometry->m_vertices[vindex++] = u;
							uvs.push_back(glm::vec2(u, v));
							break;
						case render::VERTEX_COMPONENT_COLOR:
						{
							glm::vec3 brown = glm::vec3(0.58, 0.39, 0.0);
							glm::vec3 green = glm::vec3(0.0, 0.9, 0.0);

							glm::vec3 color = height < 4.0f ? brown : green;
							geometry->m_vertices[vindex++] = color[0];
							geometry->m_vertices[vindex++] = color[1];
							geometry->m_vertices[vindex++] = color[2];
							break;
						}
						case render::VERTEX_COMPONENT_TANGENT:
							geometry->m_vertices[vindex++] = 0;
							geometry->m_vertices[vindex++] = 0;
							geometry->m_vertices[vindex++] = 0;
							tangents.push_back(glm::vec3(0.0f));
							hastangents = true;
							break;
						case render::VERTEX_COMPONENT_BITANGENT:
							geometry->m_vertices[vindex++] = 0;
							geometry->m_vertices[vindex++] = 0;
							geometry->m_vertices[vindex++] = 0;
							bitangents.push_back(glm::vec3(0.0f));
							break;
						}
					}

					max.x = fmax(pPos.x, max.x);
					max.y = fmax(pPos.y, max.y);
					max.z = fmax(pPos.z, max.z);

					min.x = fmin(pPos.x, min.x);
					min.y = fmin(pPos.y, min.y);
					min.z = fmin(pPos.z, min.z);

				}

			}

			size = max - min;
			BoundingBox* box = new BoundingBox(min, max, glm::length(size));

			int sizeofindices = 0;
			geometry->m_indices = BuildPatchIndices(0, 0, Length(), Width(), sizeofindices);
			geometry->m_indexCount = sizeofindices;

			if (hastangents)
			{
				//tangents
				for (int i = 0; i < sizeofindices - 2; i += 3)//screw the last two vertices
				{
					
					int first = geometry->m_indices[i];
					int second = geometry->m_indices[i+1];
					int third = geometry->m_indices[i+2];

					if (tangents[first] == glm::vec3(0.0f))
					{
						glm::vec3 tangent, bitangent;
						ComputeTangents(positions[first], positions[second], positions[third],
							uvs[geometry->m_indices[i]], uvs[geometry->m_indices[i + 1]], uvs[geometry->m_indices[i + 2]],
							tangent, bitangent);

						/*glm::vec3 normal = normals[first];
						float dot = glm::dot(normal, tangent);

						tangent = glm::normalize(tangent - glm::dot(tangent, normal) * normal);
						bitangent = glm::cross(normal, tangent);*/

						tangents[first] = tangent;
						bitangents[first] = bitangent;
					}

					if (tangents[second] == glm::vec3(0.0f))
					{
						glm::vec3 tangent, bitangent;
					ComputeTangents(positions[second], positions[first], positions[third],
						uvs[geometry->m_indices[i+1]], uvs[geometry->m_indices[i]], uvs[geometry->m_indices[i + 2]], 
						tangent, bitangent);

					tangents[second] = tangent;
					bitangents[second] = bitangent;
					}

					if (tangents[third] == glm::vec3(0.0f))
					{
						glm::vec3 tangent, bitangent;
					ComputeTangents(positions[geometry->m_indices[i+2]], positions[geometry->m_indices[i]], positions[geometry->m_indices[i + 1]],
						uvs[geometry->m_indices[i+2]], uvs[geometry->m_indices[i]], uvs[geometry->m_indices[i + 1]], 
						tangent, bitangent);

					tangents[geometry->m_indices[i+2]] = tangent;
					bitangents[geometry->m_indices[i+2]] = bitangent;
					}

				}
				uint32_t tangent_offset = 0;
				for (auto& component : vertex_layout->m_components[0])
				{
					if (component == render::VERTEX_COMPONENT_TANGENT)
						break;
					tangent_offset += vertex_layout->GetComponentSize(component);
				}
				tangent_offset /= sizeof(float);
				int j = 0;
				for (int i = tangent_offset; i < geometry->m_verticesSize; i += tangent_offset)
				{
					geometry->m_vertices[i] = tangents[j].x;
					geometry->m_vertices[++i] = tangents[j].y;
					geometry->m_vertices[++i] = tangents[j].z;
					geometry->m_vertices[++i] = bitangents[j].x;
					geometry->m_vertices[++i] = bitangents[j].y;
					geometry->m_vertices[++i] = bitangents[j].z;
					++i;
					j++;
				}
			}

			m_boundingBoxes.push_back(box);
			returnVector.push_back(geometry);

			return returnVector;
		}

		void Terrain::Destroy()
		{
			if (m_heights)
			{
				for (int i = 0; i < m_length; i++) {
					delete[] m_heights[i];
				}
				delete[] m_heights;
				m_heights = NULL;
			}

			if (m_normals)
			{
			
				for (int i = 0; i < m_length; i++) {
					delete[] m_normals[i];
				}
				delete[] m_normals;
				m_normals = NULL;
			}
		}

		Terrain::~Terrain() {
			Destroy();
		}

		uint32_t* Terrain::BuildPatchIndices(int offsetX, int offsetY, int width, int height, int& size)
		{
			size = (width - 1) * (height - 1) * 6;
			uint32_t* indices = new uint32_t[size];

			for (int y = 0; y < height - 1; y++)
			{
				for (int x = 0; x < width - 1; x++)
				{
					int i = x + y * (width - 1);
					int vi = x + y * width;

					int x0y0 = vi;
					int x1y0 = vi + 1;
					int x0y1 = vi + width;
					int x1y1 = vi + width + 1;


					/*   0___
					 *   |  /|
					 *   | / |
					 *   |/__|
					 *
					 */
					/*indices[i * 6 + 0] = x1y0;
					indices[i * 6 + 1] = x0y0;
					indices[i * 6 + 2] = x0y1;*/
					indices[i * 6 + 0] = x1y0;
					indices[i * 6 + 1] = x0y1;
					indices[i * 6 + 2] = x0y0;
					indices[i * 6 + 3] = x0y1;
					indices[i * 6 + 4] = x1y0;
					indices[i * 6 + 5] = x1y1;

				}
			}

			return indices;
		}
	}
}
