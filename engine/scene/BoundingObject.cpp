#include "BoundingObject.h"

namespace engine
{
	namespace scene
	{
		bool BoundingBox::FrustumIntersect(glm::vec4* planes)
		{
			int    ret = true;
			glm::vec3 vmin, vmax;

			for (int i = 0; i < 6; ++i) {

				glm::vec3 normal = glm::vec3(planes[i].x, planes[i].y, planes[i].z);

				// X axis 
				if (normal.x > 0) {
					vmin.x = m_point1.x;
					vmax.x = m_point2.x;
				}
				else {
					vmin.x = m_point2.x;
					vmax.x = m_point1.x;
				}
				// Y axis 
				if (normal.y > 0) {
					vmin.y = m_point1.y;
					vmax.y = m_point2.y;
				}
				else {
					vmin.y = m_point2.y;
					vmax.y = m_point1.y;
				}
				// Z axis 
				if (normal.z > 0) {
					vmin.z = m_point1.z;
					vmax.z = m_point2.z;
				}
				else {
					vmin.z = m_point2.z;
					vmax.z = m_point1.z;
				}
				/*if (glm::dot(normal, vmin) + planes[i].w < 0)
					ret &= false;
				if (glm::dot(normal, vmax) + planes[i].w <= 0)
					ret &= true;*/
				float res1 = vmin.x * planes[i].x + vmin.y * planes[i].y + vmin.z * planes[i].z + planes[i].w;
				float res2 = vmax.x * planes[i].x + vmax.y * planes[i].y + vmax.z * planes[i].z + planes[i].w;
				if (res1 <= 0.0f && res2 <= 0.0f)
				{
					return false;
				}
			}

			return ret;
		}
	}
}