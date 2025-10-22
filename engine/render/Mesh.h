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

			uint32_t m_indexCount = 0;
			uint64_t m_vertexCount = 0;
			uint64_t m_vertexSize = 0;
			uint64_t m_verticesSize = 0;

			uint32_t m_instanceNo = 1;
			size_t m_instanceBufferSize = 0;
			bool m_instanceFrequentUpdate = true;

			~MeshData();
		};

		class Mesh
		{
		public:
			VertexLayout* m_vlayout = nullptr;
			//class Buffer* _instanceBuffer = nullptr;
			uint32_t m_instanceNo = 1;
			uint32_t m_indexCount = 0;
		public:
			//uint32_t m_indexCount = 0;//TODO make it private or remove
			//uint64_t m_vertexCount = 0;
			//render::Buffer* _vertexBuffer = nullptr;
			//render::Buffer* _indexBuffer = nullptr;
			virtual void UpdateInstanceBuffer(void* data, size_t offset, size_t size) = 0;
			virtual ~Mesh() {}
			virtual void Draw(CommandBuffer *commandBuffer) = 0;
		};
	}
}