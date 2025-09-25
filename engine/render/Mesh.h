#pragma once
#include "CommandBuffer.h"
#include "VertexLayout.h"

namespace engine
{
	namespace render
	{
		struct MeshData
		{
			float* m_vertices = nullptr;
			uint32_t* m_indices = nullptr;

			uint32_t m_indexCount = 0;
			uint64_t m_vertexCount = 0;
			uint64_t m_vertexSize = 0;
			uint64_t m_verticesSize = 0;
			uint32_t m_instanceNo = 1;

			~MeshData();
		};

		class Mesh
		{
			VertexLayout* m_vlayout = nullptr;
		public:
			virtual void Draw(CommandBuffer *commandBuffer) = 0;
		};
	}
}