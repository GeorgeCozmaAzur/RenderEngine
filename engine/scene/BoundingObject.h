#pragma once

#include <glm/glm.hpp>
#include <vector>

namespace engine
{
	namespace scene
	{
		class BoundingObject
		{
		protected:
			bool m_visible = true;
			glm::vec3 m_center = glm::vec3(0.0f);
			glm::vec3 m_velocity = glm::vec3(0.0f);
			float m_size;
		public:
			std::vector<int> position_in_tree;
			glm::vec3 GetCenter() { return m_center; }
			glm::vec3 GetVelocity() { return m_velocity; }
			float GetSize() { return m_size; }
			bool IsVisible() { return m_visible; }
			void SetVisibility(bool value) { m_visible = value; }
			virtual bool isInsideSpace(glm::vec3 point1, glm::vec3 point2) = 0;
			virtual bool FrustumIntersect(glm::vec4* planes) = 0;
			virtual bool IsClose(BoundingObject*) = 0;
			virtual void Collide(BoundingObject*) = 0;
		};

		class BoundingBox : public BoundingObject
		{
			glm::vec3 m_point1;
			glm::vec3 m_point2;
			std::vector<glm::vec3> m_vertices;
		public:
			BoundingBox(glm::vec3 point1, glm::vec3 point2, float size) : m_point1(point1), m_point2(point2)
			{
				m_size = size;
				CreateVertices();
			}
			BoundingBox(std::vector<float> vertices, uint32_t positionOffset)
			{
				m_point1 = glm::vec3(vertices[0], vertices[1], vertices[2]);//min
				m_point2 = glm::vec3(0.0f);//max
				for (int i = 0; i < vertices.size(); i += 3 + positionOffset)
				{
					if (m_point1.x > vertices[i]) m_point1.x = vertices[i];
					if (m_point1.y > vertices[i + 1]) m_point1.y = vertices[i + 1];
					if (m_point1.z > vertices[i + 2]) m_point1.z = vertices[i + 2];

					if (m_point2.x < vertices[i]) m_point2.x = vertices[i];
					if (m_point2.y < vertices[i + 1]) m_point2.y = vertices[i + 1];
					if (m_point2.z < vertices[i + 2]) m_point2.z = vertices[i + 2];
				}
				CreateVertices();
			}

			const glm::vec3 GetPoint1() { return m_point1; }
			const glm::vec3 GetPoint2() { return m_point2; }

			bool isInsideSpace(glm::vec3 point1, glm::vec3 point2)
			{
				for (int i = 0; i < m_vertices.size();i++)
				{
					bool between_x = ((m_vertices[i].x - point1.x) * (m_vertices[i].x - point2.x) <= 0);
					bool between_y = ((m_vertices[i].y - point1.y) * (m_vertices[i].y - point2.y) <= 0);
					bool between_z = ((m_vertices[i].z - point1.z) * (m_vertices[i].z - point2.z) <= 0);
					bool ret = (between_x && between_y && between_z);//TODO what if it's so large that excedes the size of the partition
					if (ret)
						return true;
				}
				return false;
			}

			bool FrustumIntersect(glm::vec4* planes);

			void CreateVertices()
			{
				glm::vec3 point12 = m_point2; point12.y = m_point1.y;
				glm::vec3 point21 = m_point1; point21.y = m_point2.y;

				//make square with point1 and point12

				glm::vec3 point_aux1(m_point1.y);
				glm::vec3 point_aux2(m_point1.y);

				point_aux1.x = m_point1.x;
				point_aux1.z = point12.z;
				point_aux2.x = point12.x;
				point_aux2.z = m_point1.z;

				//make a square with point21 and point2

				glm::vec3 point_aux11(m_point2.y);
				glm::vec3 point_aux22(m_point2.y);

				point_aux11.x = point21.x;
				point_aux11.z = m_point2.z;
				point_aux22.x = m_point2.x;
				point_aux22.z = point21.z;

				m_vertices.push_back(m_point1);
				m_vertices.push_back(point_aux1);
				m_vertices.push_back(point12);
				m_vertices.push_back(point_aux2);

				m_vertices.push_back(point21);
				m_vertices.push_back(point_aux11);
				m_vertices.push_back(m_point2);
				m_vertices.push_back(point_aux22);
			}

			void CreateVertexBuffer(float* vBuffer, glm::vec3 color)
			{
				int vindex = 0;
				for (int i = 0; i < m_vertices.size(); i++)
				{
					vBuffer[vindex++] = m_vertices[i].x;
					vBuffer[vindex++] = m_vertices[i].y;
					vBuffer[vindex++] = m_vertices[i].z;

					vBuffer[vindex++] = color.x;
					vBuffer[vindex++] = color.y;
					vBuffer[vindex++] = color.z;
				}
			}

			bool IsClose(BoundingObject*)
			{
				return false;
			}
			void Collide(BoundingObject*)
			{

			}
		};

		class BoundingSphere : public BoundingObject
		{
			float m_radius = 0.0f;

		public:

			BoundingSphere(glm::vec3 center, glm::vec3 velocity, float radius) : m_radius(radius)
			{
				m_center = center;
				m_velocity = velocity;
				m_size = radius;
			}

			void AddToVelocity(glm::vec3 v)
			{
				m_velocity += v;
			}
			void AddToPosition(glm::vec3 p)
			{
				m_center += p;
			}

			bool FrustumIntersect(glm::vec4* planes);

			bool isInsideSpace(glm::vec3 point1, glm::vec3 point2)
			{
				bool between_x = ((m_center.x - point1.x) * (m_center.x - point2.x) <= 0);
				if (!between_x)
					return false;
				bool between_y = ((m_center.y - point1.y) * (m_center.y - point2.y) <= 0);
				if (!between_y)
					return false;
				bool between_z = ((m_center.z - point1.z) * (m_center.z - point2.z) <= 0);
				if (!between_z)
					return false;
				return true;//(between_x && between_y && between_z);
			}
			bool IsClose(BoundingObject* other)
			{
				if (glm::distance(m_center, other->GetCenter()) < m_radius + other->GetSize())//TODO 
				{
					glm::vec3 netVelocity = m_velocity - other->GetVelocity();
					glm::vec3 displacement = m_center - other->GetCenter();
					return glm::dot(netVelocity, displacement) < 0;
				}
				return false;
			}
			void Collide(BoundingObject* other)
			{
				glm::vec3 disp = glm::normalize(m_center - other->GetCenter());
				m_velocity -= disp * (glm::dot(m_velocity, disp) * 2);
			}
		};
	}
}