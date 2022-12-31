#pragma once
#include "Geometry.h"
#include "BoundingObject.h"
#include <glm/glm.hpp>

namespace engine
{
	namespace scene
	{
		class SpacePartitionTree
		{
		public://TODO maybe not

			std::vector<SpacePartitionTree*> m_children;
			//int elements_no = 0;
			int m_totalObjectsNo = 0;
			int m_ownObjectsNo = 0;
			Geometry* m_debug_geometry = nullptr;

			std::vector<glm::vec3> m_boundries;
			std::vector<glm::vec3> m_boundries_extra;

			std::vector<glm::vec3> m_vertices;

			std::vector<BoundingObject*> m_objects;

			bool m_hasActiveChildren = false;

			SpacePartitionTree();

			bool IsObjectInside(BoundingObject* obj);

			bool FrustumIntersect(glm::vec4* planes);

			void GetObjectLocationInTree(BoundingObject* obj, std::vector<int>& depth);

			void AdvanceObject(BoundingObject* obj);

			void collectAndClearAllObjects(std::vector<BoundingObject*>& objects);

			bool RemoveObject(BoundingObject* obj, int current_depth);

			void AddObject(BoundingObject* obj, int current_depth);

			void CreateAABB(std::vector<glm::vec3>& vertices, glm::vec3 point1, glm::vec3 point2);

			void CreateDebugGeometry3D();

			void DivideGeometry3D(Geometry* geo, std::vector<Geometry*>& out_geometries, std::vector<glm::vec3>& boundries);

			void DivideGeometry(Geometry* geo, std::vector<Geometry*>& out_geometries, std::vector<glm::vec3>& boundries);

			void SetDebugGeometry(Geometry* geo);
			Geometry* GetDebugGeometry();

			void CreateChildren();

			void GatherAllGeometries(std::vector<Geometry*>& out_geometries);

			void TestCollisions();

			void ResetVisibility();

			void TestVisibility(glm::vec4* frustum_planes);

			void UpdateDebugGeometries();

			void DestroyChildren();

			~SpacePartitionTree();
		};
	}
}
