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
			void* _instanceExternalData = nullptr;

			VertexLayout* m_vlayout = nullptr;

			uint32_t m_indexCount = 0;
			uint16_t m_indexSize = 4;//index size in bytes
			uint64_t m_vertexCount = 0;
			uint64_t m_vertexSize = 0;
			uint64_t m_verticesSize = 0;

			uint32_t m_instanceNo = 1;
			size_t m_instanceBufferSize = 0;
			bool m_instanceFrequentUpdate = true;

			~MeshData();
		};

		struct MeshPart
		{
			uint32_t    indexCount;
			uint32_t    instanceCount;
			uint32_t    firstIndex;
			int32_t     vertexOffset;
			uint32_t    firstInstance;
		};

		class Mesh
		{
		public:
			VertexLayout* m_vlayout = nullptr;
			uint32_t m_indexCount = 0;
			uint16_t m_indexSize = 4;//index size in bytes
			uint64_t m_vertexCount = 0;
			uint32_t m_instanceNo = 1;

		public:

			virtual void UpdateIndexBuffer(void* data, size_t size, size_t offset) = 0;
			virtual void FlushIndexBuffer() {};
			virtual void UpdateVertexBuffer(void* data, size_t size, size_t offset) = 0;
			virtual void FlushVertexBuffer() {};
			virtual void UpdateInstanceBuffer(void* data, size_t size, size_t offset) = 0;
			virtual void SetVertexBuffer(class Buffer* buffer) = 0;
			virtual ~Mesh() {}
			virtual void Draw(CommandBuffer *commandBuffer, const std::vector<MeshPart>& parts = std::vector<MeshPart>()) = 0;
		};
	}
}