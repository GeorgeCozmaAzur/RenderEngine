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
			for (int i = 0; i < m_children.size(); i++)
			{
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
			if (obj->position_in_tree.size() > 0)
			{
				SpacePartitionTree* objectSpace = this;
				for (int i : obj->position_in_tree)
				{
					objectSpace = objectSpace->m_children[i];
				}
				if (objectSpace->IsObjectInside(obj))
					return;
			}

			std::vector<int> depth;
			GetObjectLocationInTree(obj, depth);

			if (obj->position_in_tree.size() > 0)
				RemoveObject(obj, 0);

			obj->position_in_tree.clear();
			obj->position_in_tree.insert(obj->position_in_tree.end(), depth.begin(), depth.end());

			if (depth.size() > 0)
				AddObject(obj, 0);

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
				for(auto obj : m_objects)
				{
					objects.push_back(obj);
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

				for(auto obj : m_objects)
				{
					for (int i = 0; i < m_children.size(); i++)
					{
						if (m_children[i]->IsObjectInside(obj))
						{
							m_children[i]->AddObject(obj, current_depth + 1);
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

		void SpacePartitionTree::CreateChildren(int divisions)
		{
			float sizeonx = abs(m_boundries[0].x - m_boundries[1].x) / divisions;
			float sizeony = abs(m_boundries[0].y - m_boundries[1].y) / divisions;
			float sizeonz = abs(m_boundries[0].z - m_boundries[1].z) / divisions;

			for (int x = 0; x < divisions; x++)
			{
				for (int y = 0; y < divisions; y++)
				{
					for (int z = 0; z < divisions; z++)
					{
						SpacePartitionTree* tree = new SpacePartitionTree;

						glm::vec3 min(m_boundries[0].x + sizeonx * x, 
							m_boundries[0].y + sizeonx * y,
							m_boundries[0].z + sizeonx * z);
						tree->m_boundries.push_back(min);

						glm::vec3 max(m_boundries[0].x + sizeonx * (x+1),
							m_boundries[0].y + sizeonx * (y+1),
							m_boundries[0].z + sizeonx * (z+1));
						tree->m_boundries.push_back(max);

						m_children.push_back(tree);
					}
				}
			}
		}

		void SpacePartitionTree::GatherAllBoundries(std::vector<std::vector<glm::vec3>>& out_boundries)
		{
			out_boundries.push_back(m_boundries);
			for (int i = 0; i < m_children.size(); i++)
			{
				m_children[i]->GatherAllBoundries(out_boundries);
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
				{
					//std::list<BoundingObject*>::iterator it;
					//std::list<BoundingObject*>::iterator otherit;
					//for(it=m_objects.begin();it!=m_objects.end();++it)
					for (int i = 0; i < m_objects.size(); i++)
					{
						for (int j = i + 1; j < m_objects.size(); j++)
						//for (otherit = std::next(it); otherit != m_objects.end(); ++otherit)
						{
							if (m_objects[i]->IsClose(m_objects[j]))
							{
								m_objects[i]->Collide(m_objects[j]);
								m_objects[j]->Collide(m_objects[i]);
							}
							/*if ((*it)->IsClose(*otherit))
							{
								(*it)->Collide((*otherit));
								(*otherit)->Collide((*it));
							}*/
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
				for (auto obj : m_objects)
				{
					obj->SetVisibility(false);
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
				for (auto obj : m_objects)
				{
					if (obj->FrustumIntersect(frustum_planes))
						obj->SetVisibility(true);
				}
			}
		}

		void SpacePartitionTree::DestroyChildren()
		{
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