/*
* View frustum culling class
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include <array>
#include <math.h>
#include <glm/glm.hpp>

namespace engine
{
	namespace scene
	{
		class Frustum
		{
		public:
			enum side { LEFT = 0, RIGHT = 1, TOP = 2, BOTTOM = 3, BACK = 4, FRONT = 5 };
			std::array<glm::vec4, 6> m_planes;

			void update(glm::mat4 matrix)
			{
				m_planes[LEFT].x = matrix[0].w + matrix[0].x;
				m_planes[LEFT].y = matrix[1].w + matrix[1].x;
				m_planes[LEFT].z = matrix[2].w + matrix[2].x;
				m_planes[LEFT].w = matrix[3].w + matrix[3].x;

				m_planes[RIGHT].x = matrix[0].w - matrix[0].x;
				m_planes[RIGHT].y = matrix[1].w - matrix[1].x;
				m_planes[RIGHT].z = matrix[2].w - matrix[2].x;
				m_planes[RIGHT].w = matrix[3].w - matrix[3].x;

				m_planes[TOP].x = matrix[0].w - matrix[0].y;
				m_planes[TOP].y = matrix[1].w - matrix[1].y;
				m_planes[TOP].z = matrix[2].w - matrix[2].y;
				m_planes[TOP].w = matrix[3].w - matrix[3].y;

				m_planes[BOTTOM].x = matrix[0].w + matrix[0].y;
				m_planes[BOTTOM].y = matrix[1].w + matrix[1].y;
				m_planes[BOTTOM].z = matrix[2].w + matrix[2].y;
				m_planes[BOTTOM].w = matrix[3].w + matrix[3].y;

				m_planes[BACK].x = matrix[0].w + matrix[0].z;
				m_planes[BACK].y = matrix[1].w + matrix[1].z;
				m_planes[BACK].z = matrix[2].w + matrix[2].z;
				m_planes[BACK].w = matrix[3].w + matrix[3].z;

				m_planes[FRONT].x = matrix[0].w - matrix[0].z;
				m_planes[FRONT].y = matrix[1].w - matrix[1].z;
				m_planes[FRONT].z = matrix[2].w - matrix[2].z;
				m_planes[FRONT].w = matrix[3].w - matrix[3].z;

				for (auto i = 0; i < m_planes.size(); i++)
				{
					float length = sqrtf(m_planes[i].x * m_planes[i].x + m_planes[i].y * m_planes[i].y + m_planes[i].z * m_planes[i].z);
					m_planes[i] /= length;
				}
			}

			bool checkSphere(glm::vec3 pos, float radius)
			{
				for (auto i = 0; i < m_planes.size(); i++)
				{
					if ((m_planes[i].x * pos.x) + (m_planes[i].y * pos.y) + (m_planes[i].z * pos.z) + m_planes[i].w <= -radius)
					{
						return false;
					}
				}
				return true;
			}
		};
	}
}
