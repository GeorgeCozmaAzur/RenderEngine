#include "SpacePartitionTree.h"

#define MAX_BALLS_NO 2
#define MIN_BALLS_NO 1
#define TREE_DEPTH 3

namespace engine
{
	namespace scene
	{

		//void Outputkk(const char* szFormat, ...)
		//{
		//	char szBuff[1024];
		//	va_list arg;
		//	va_start(arg, szFormat);
		//	_vsnprintf(szBuff, sizeof(szBuff), szFormat, arg);
		//	va_end(arg);
		//
		//	OutputDebugString(szBuff);
		//}

		SpacePartitionTree::SpacePartitionTree()
		{
			m_boundries.reserve(2);
			m_objects.reserve(MAX_BALLS_NO);
		}

		bool SpacePartitionTree::IsObjectInside(BoundingObject* obj)
		{
			/*bool between_x = ((obj->pos.x - m_boundries[0].x) * (obj->pos.x - m_boundries[1].x) <= 0);
			bool between_y = ((obj->pos.y - m_boundries[0].y) * (obj->pos.y - m_boundries[1].y) <= 0);
			bool between_z = ((obj->pos.z - m_boundries[0].z) * (obj->pos.z - m_boundries[1].z) <= 0);
			bool ret = (between_x && between_y && between_z);*/
			//Outputkk("pos: %g %g \n", obj->pos.x, obj->pos.y);
			//Outputkk("bounds: %g %g %g %g \n", m_boundries[0].x, m_boundries[0].y, m_boundries[1].x, m_boundries[1].y);
			//Outputkk("inside: %d\n", ret);
			return obj->isInsideSpace(m_boundries[0], m_boundries[1]);
		}

		bool SpacePartitionTree::FrustumIntersect(glm::vec4* planes)
		{
			bool    ret = true;
			//glm::vec3 vmin, vmax;

			for (int i = 0; i < 6; ++i)
			{
				bool isSomeInside = false;
				bool isSomeOutside = false;

				glm::vec3 normal = glm::vec3(planes[i].x, planes[i].y, planes[i].z);
				for (int j = 0; j < m_vertices.size(); j++)
				{
					float res = m_vertices[j].x * planes[i].x + m_vertices[j].y * planes[i].y + m_vertices[j].z * planes[i].z + planes[i].w;
					if (res <= 0)
						isSomeOutside |= true;
					else
						isSomeInside |= true;

					if (isSomeOutside && isSomeInside)
						return  true;
				}
				if (isSomeOutside && !isSomeInside)
					return false;

				ret |= isSomeInside;
			}

			return ret;
		}


		void SpacePartitionTree::GetObjectLocationInTree(BoundingObject* obj, std::vector<int>& depth)
		{
			//Outputkk("GetObjectLocationInTree\n");
			//if (has_active_children)//george
			for (int i = 0; i < m_children.size(); i++)
			{
				//Outputkk("GetObjectLocationInTree child\n");
				if (m_children[i]->IsObjectInside(obj))
				{
					depth.push_back(i);
					m_children[i]->GetObjectLocationInTree(obj, depth);
					break;
				}
			}
		}

		void SpacePartitionTree::AdvanceObject(BoundingObject* obj)
		{
			std::vector<int> depth;
			GetObjectLocationInTree(obj, depth);
			bool location_moved = false;
			if (depth.size() != obj->position_in_tree.size())
			{
				location_moved = true;
			}
			else
			{
				for (int i = 0; i < depth.size(); i++)
				{
					if (obj->position_in_tree[i] != depth[i])
					{
						location_moved = true;
						break;
					}
				}
			}

			if (location_moved)
			{
				if (obj->position_in_tree.size() > 0)
					RemoveObject(obj, 0);

				obj->position_in_tree.clear();
				obj->position_in_tree.insert(obj->position_in_tree.end(), depth.begin(), depth.end());

				if (depth.size() > 0)
					AddObject(obj, 0);
			}

		}
		void SpacePartitionTree::collectAndClearAllObjects(std::vector<BoundingObject*>& objects)
		{
			if (m_hasActiveChildren)
			{
				for (int i = 0; i < m_children.size(); i++)
				{
					m_children[i]->collectAndClearAllObjects(objects);
				}
			}
			else
			{
				for (int j = 0; j < m_objects.size(); j++)
				{
					objects.push_back(m_objects[j]);
				}
				m_objects.clear();
			}
		}

		bool SpacePartitionTree::RemoveObject(BoundingObject* obj, int current_depth)
		{
			//if (total_objects_no == 0) return;
			bool removed = false;
			if (m_hasActiveChildren)
			{
				removed = m_children[obj->position_in_tree[current_depth]]->RemoveObject(obj, current_depth + 1);
			}
			else
			{
				std::vector<BoundingObject*>::iterator it;
				it = find(m_objects.begin(), m_objects.end(), obj);
				if (it != m_objects.end())
				{
					m_objects.erase(it);
					removed = true;
				}
			}
			if (removed)
				m_totalObjectsNo--;

			if ((m_totalObjectsNo) < MIN_BALLS_NO && m_hasActiveChildren && m_children.size() > 0)
			{
				std::vector<BoundingObject*> objects;
				collectAndClearAllObjects(objects);
				m_objects.insert(m_objects.end(), objects.begin(), objects.end());//george
				m_hasActiveChildren = false;
			}

			return removed;


		}

		void SpacePartitionTree::AddObject(BoundingObject* obj, int current_depth)
		{
			if ((m_totalObjectsNo + 1) > MAX_BALLS_NO && !m_hasActiveChildren && m_children.size() > 0)
			{
				m_hasActiveChildren = true;

				for (int j = 0; j < m_objects.size(); j++)
				{
					for (int i = 0; i < m_children.size(); i++)
					{
						if (m_children[i]->IsObjectInside(m_objects[j]))
						{
							m_children[i]->AddObject(m_objects[j], current_depth + 1);
							break;//TODO what if we want duplicates
						}
					}
				}
				m_objects.clear();
			}

			if (m_hasActiveChildren)
			{
				m_children[obj->position_in_tree[current_depth]]->AddObject(obj, current_depth + 1);
			}
			else
			{
				m_objects.push_back(obj);
			}
			m_totalObjectsNo++;

		}

		void SpacePartitionTree::CreateAABB(std::vector<glm::vec3>& vertices, glm::vec3 point1, glm::vec3 point2)
		{
			glm::vec3 point12 = point2; point12.y = point1.y;
			glm::vec3 point21 = point1; point21.y = point2.y;

			//make square with point1 and point12

			glm::vec3 point_aux1(point1.y);
			glm::vec3 point_aux2(point1.y);

			point_aux1.x = point1.x;
			point_aux1.z = point12.z;
			point_aux2.x = point12.x;
			point_aux2.z = point1.z;

			//make a square with point21 and point2

			glm::vec3 point_aux11(point2.y);
			glm::vec3 point_aux22(point2.y);

			point_aux11.x = point21.x;
			point_aux11.z = point2.z;
			point_aux22.x = point2.x;
			point_aux22.z = point21.z;

			vertices.push_back(point1);
			vertices.push_back(point_aux1);
			vertices.push_back(point12);
			vertices.push_back(point_aux2);

			vertices.push_back(point21);
			vertices.push_back(point_aux11);
			vertices.push_back(point2);
			vertices.push_back(point_aux22);

		}

		void SpacePartitionTree::CreateDebugGeometry3D()
		{

			CreateAABB(m_vertices, m_boundries[0], m_boundries[1]);

			Geometry* newgeo = new Geometry;

			std::vector<int> list = { 0,1,1,2,2,3,3,0, 0,4,3,7,2,6,1,5,4,5,5,6,6,7,7,4 };
			newgeo->m_indices = new uint32_t[list.size()];
			for (int i = 0; i < list.size(); i++)
				newgeo->m_indices[i] = list[i];
			newgeo->m_indexCount = static_cast<uint32_t>(list.size());
			newgeo->m_vertexCount = 8;
			newgeo->m_instanceNo = 1;

			newgeo->m_verticesSize = m_vertices.size() * 6;
			newgeo->m_vertices = new float[newgeo->m_verticesSize];
			int vindex = 0;
			for (int i = 0; i < m_vertices.size(); i++)
			{
				newgeo->m_vertices[vindex++] = m_vertices[i].x;
				newgeo->m_vertices[vindex++] = m_vertices[i].y;
				newgeo->m_vertices[vindex++] = m_vertices[i].z;

				newgeo->m_vertices[vindex++] = 1.0f;
				newgeo->m_vertices[vindex++] = 1.0f;
				newgeo->m_vertices[vindex++] = 1.0f;
			}

			m_debug_geometry = newgeo;
		}

		void SpacePartitionTree::DivideGeometry3D(Geometry* geo, std::vector<Geometry*>& out_geometries, std::vector<glm::vec3>& boundries)
		{
			if (!geo)
				return;

			//hardcoding the vertex layout
			std::vector<glm::vec3> current_vertices;
			for (int i = 0; i < geo->m_verticesSize; i += 6)
			{
				current_vertices.push_back(glm::vec3(geo->m_vertices[i], geo->m_vertices[i + 1], geo->m_vertices[i + 2]));
				//current_vertices.push_back(glm::vec3(geo->m_vertices[i + 3], geo->m_vertices[i + 4], geo->m_vertices[1 + 5]));
			}
			glm::vec3 color = glm::vec3(geo->m_vertices[3], geo->m_vertices[4], geo->m_vertices[5]);
			glm::vec3 newcolor = color * 0.5f;

			glm::vec3 center = (current_vertices[0] + current_vertices[6]) * 0.5f;

			for (int i = 0; i < current_vertices.size(); i++)
			{
				std::vector<glm::vec3> new_vertices;
				new_vertices.reserve(current_vertices.size());

				CreateAABB(new_vertices, current_vertices[i], center);

				boundries.push_back(current_vertices[i]);
				boundries.push_back(center);

				Geometry* newgeo = new Geometry;
				newgeo->m_indexCount = geo->m_indexCount;
				newgeo->m_indices = geo->m_indices;
				newgeo->m_vertexCount = geo->m_vertexCount;

				newgeo->m_verticesSize = new_vertices.size() * 6;
				newgeo->m_vertices = new float[newgeo->m_verticesSize];

				int vindex = 0;
				for (int i = 0; i < new_vertices.size(); i++)
				{
					newgeo->m_vertices[vindex++] = new_vertices[i].x;
					newgeo->m_vertices[vindex++] = new_vertices[i].y;
					newgeo->m_vertices[vindex++] = new_vertices[i].z;

					newgeo->m_vertices[vindex++] = newcolor.x;
					newgeo->m_vertices[vindex++] = newcolor.y;
					newgeo->m_vertices[vindex++] = newcolor.z;
				}

				out_geometries.push_back(newgeo);

			}
		}

		void SpacePartitionTree::DivideGeometry(Geometry* geo, std::vector<Geometry*>& out_geometries, std::vector<glm::vec3>& boundries)
		{
			//hardcoding the vertex layout
			std::vector<glm::vec3> current_vertices;
			for (int i = 0; i < geo->m_verticesSize; i += 6)
			{
				current_vertices.push_back(glm::vec3(geo->m_vertices[i], geo->m_vertices[i + 1], geo->m_vertices[i + 2]));
				//current_vertices.push_back(glm::vec3(geo->m_vertices[i + 3], geo->m_vertices[i + 4], geo->m_vertices[1 + 5]));
			}
			glm::vec3 color = glm::vec3(geo->m_vertices[3], geo->m_vertices[4], geo->m_vertices[5]);
			glm::vec3 newcolor = color * 0.5f;

			glm::vec3 center = (current_vertices[0] + current_vertices[2]) * 0.5f;

			for (int i = 0; i < current_vertices.size(); i++)
			{
				std::vector<glm::vec3> new_vertices;
				new_vertices.reserve(current_vertices.size());


				glm::vec3 point2(0.0f); point2.z = center.z;
				glm::vec3 point4(0.0f);	point4.z = center.z;

				point2.x = current_vertices[i].x;
				point2.y = center.y;

				point4.x = center.x;
				point4.y = current_vertices[i].y;

				new_vertices.push_back(current_vertices[i]);
				new_vertices.push_back(point2);
				new_vertices.push_back(center);
				new_vertices.push_back(point4);

				boundries.push_back(current_vertices[i]);
				boundries.push_back(center);

				Geometry* newgeo = new Geometry;
				newgeo->m_indexCount = geo->m_indexCount;
				newgeo->m_indices = geo->m_indices;//TODO this is dangherosu - it will crash
				newgeo->m_vertexCount = geo->m_vertexCount;

				newgeo->m_verticesSize = new_vertices.size() * 6;
				newgeo->m_vertices = new float[newgeo->m_verticesSize];

				int vindex = 0;
				for (int i = 0; i < new_vertices.size(); i++)
				{
					newgeo->m_vertices[vindex++] = new_vertices[i].x;
					newgeo->m_vertices[vindex++] = new_vertices[i].y;
					newgeo->m_vertices[vindex++] = new_vertices[i].z;

					newgeo->m_vertices[vindex++] = newcolor.x;
					newgeo->m_vertices[vindex++] = newcolor.y;
					newgeo->m_vertices[vindex++] = newcolor.z;
				}

				out_geometries.push_back(newgeo);
			}
		}

		void SpacePartitionTree::SetDebugGeometry(Geometry* geo) { m_debug_geometry = geo; }
		Geometry* SpacePartitionTree::GetDebugGeometry() { return m_debug_geometry; }

		void SpacePartitionTree::CreateChildren()
		{
			//has_active_children = true;//george
			std::vector<Geometry*> geometries_for_kids;
			std::vector<glm::vec3> boundries;
			//DivideGeometry(m_debug_geometry, geometries_for_kids, boundries);
			DivideGeometry3D(m_debug_geometry, geometries_for_kids, boundries);
			int b = 0;
			for (int i = 0; i < geometries_for_kids.size(); i++)
			{
				SpacePartitionTree* tree = new SpacePartitionTree;
				tree->m_boundries.push_back(glm::vec3(boundries[b].x, boundries[b].y, boundries[b].z));
				tree->m_boundries.push_back(glm::vec3(boundries[b + 1].x, boundries[b + 1].y, boundries[b + 1].z));
				//Outputkk("create boundries: %g %g %g %g \n", tree->m_boundries[0].x, tree->m_boundries[0].y, tree->m_boundries[1].x, tree->m_boundries[1].y);
				b += 2;
				tree->m_debug_geometry = geometries_for_kids[i];
				m_children.push_back(tree);
			}
		}

		void SpacePartitionTree::GatherAllGeometries(std::vector<Geometry*>& out_geometries)
		{
			//out_geometries.push_back(m_debug_geometry);
			for (int i = 0; i < m_children.size(); i++)
			{
				//if(children[i]->children.size() !=0)
				out_geometries.push_back(m_children[i]->m_debug_geometry);
			}
			for (int i = 0; i < m_children.size(); i++)
			{
				m_children[i]->GatherAllGeometries(out_geometries);
			}
		}

		void SpacePartitionTree::TestCollisions()
		{
			if (m_hasActiveChildren)
			{
				for (int i = 0; i < m_children.size(); i++)
				{
					m_children[i]->TestCollisions();
				}
			}
			else
			{
				if (m_objects.size() > 0)
					for (int i = 0; i < m_objects.size() - 1; i++)
					{
						for (int j = i + 1; j < m_objects.size(); j++)
						{
							if (m_objects[i]->IsClose(m_objects[j]))
							{
								m_objects[i]->Collide(m_objects[j]);
								m_objects[j]->Collide(m_objects[i]);
							}
						}
					}
			}
		}

		void SpacePartitionTree::ResetVisibility()
		{
			if (m_hasActiveChildren)
			{
				for (int i = 0; i < m_children.size(); i++)
				{
					m_children[i]->ResetVisibility();
				}
			}
			else
			{
				for (int i = 0; i < m_objects.size(); i++)
				{
					m_objects[i]->SetVisibility(false);
				}
			}
		}

		void SpacePartitionTree::TestVisibility(glm::vec4* frustum_planes)
		{
			if (!FrustumIntersect(frustum_planes))
				return;

			if (m_hasActiveChildren)
			{
				for (int i = 0; i < m_children.size(); i++)
				{
					m_children[i]->TestVisibility(frustum_planes);
				}
			}
			else
			{
				for (int i = 0; i < m_objects.size(); i++)
				{
					if (m_objects[i]->FrustumIntersect(frustum_planes))
						m_objects[i]->SetVisibility(true);
				}
			}
		}

		void SpacePartitionTree::UpdateDebugGeometries()
		{
			for (int i = 0; i < m_children.size(); i++)
			{
				if (!m_hasActiveChildren)
				{

					m_children[i]->m_debug_geometry->m_isVisible = false;

				}
				else
				{
					m_children[i]->m_debug_geometry->m_isVisible = true;


				}

				m_children[i]->UpdateDebugGeometries();
			}
		}

		void SpacePartitionTree::DestroyChildren()
		{
			if (m_debug_geometry)
			{
				delete m_debug_geometry;
				m_debug_geometry = nullptr;
			}

			m_hasActiveChildren = false;
			for (int i = 0; i < m_children.size(); i++)
			{
				m_children[i]->DestroyChildren();
				delete m_children[i];
			}
			m_children.clear();
		}

		SpacePartitionTree::~SpacePartitionTree()
		{
			DestroyChildren();
		}
	}
}