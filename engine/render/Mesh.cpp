#include "Mesh.h"
#include <assert.h>

namespace engine
{
	namespace render
	{
		MeshData::~MeshData()
		{
			if (m_vertices)
				delete[]m_vertices;
			if (m_indices)
				delete[]m_indices;
		}
	}
}
