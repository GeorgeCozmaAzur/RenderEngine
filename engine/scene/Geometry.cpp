#include "Geometry.h"

namespace engine
{
	namespace scene
	{
		void Geometry::Destroy()
		{
			if (!m_vertices)
				delete[]m_vertices;
			if (!m_indices)
				delete[]m_indices;
		}

		void Geometry::SetVertexBuffer(render::VulkanBuffer* buf, bool keepRAMData)
		{
			_vertexBuffer = buf;
			if (!keepRAMData && m_vertices)
			{
				delete[]m_vertices;
				m_vertices = nullptr;
			}
		}
		void Geometry::SetIndexBuffer(render::VulkanBuffer* buf, bool keepRAMData)
		{
			_indexBuffer = buf;
			if (!keepRAMData && m_indices)
			{
				delete[]m_indices;
				m_indices = nullptr;
			}
		}
		void Geometry::SetInstanceBuffer(render::VulkanBuffer* buf, bool keepRAMData)
		{
			_instanceBuffer = buf;//TODO what do we do with keepramdata
		}

		void Geometry::UpdateIndexBuffer(void* data, size_t offset, size_t size)
		{
			_indexBuffer->MemCopy(data, size);
		}
		void Geometry::UpdateInstanceBuffer(void* data, size_t offset, size_t size)
		{
			_instanceBuffer->MemCopy(data, size);
		}

		void Geometry::Draw(VkCommandBuffer* commandBuffer, uint32_t vertexInputBinding, uint32_t instanceInputBinding)
		{
			if (!m_isVisible)
				return;

			VkDeviceSize offsets[1] = { 0 };
			const VkBuffer vertexBuffer = _vertexBuffer->GetVkBuffer();
			vkCmdBindVertexBuffers(*commandBuffer, vertexInputBinding, 1, &vertexBuffer, offsets);
			vkCmdBindIndexBuffer(*commandBuffer, _indexBuffer->GetVkBuffer(), 0, VK_INDEX_TYPE_UINT32);
			if (_instanceBuffer && _instanceBuffer->GetVkBuffer() && instanceInputBinding > 0)
			{
				const VkBuffer instanceBuffer = _instanceBuffer->GetVkBuffer();
				vkCmdBindVertexBuffers(*commandBuffer, instanceInputBinding, 1, &instanceBuffer, offsets);
			}

			vkCmdDrawIndexed(*commandBuffer, m_indexCount, m_instanceNo, 0, 0, 0);
		}

		Geometry& Geometry::operator=(const Geometry& other)
		{
			m_indexCount = other.m_indexCount;
			m_instanceNo = other.m_instanceNo;
			_indexBuffer = other._indexBuffer;
			_vertexBuffer = other._vertexBuffer;
			_instanceBuffer = other._instanceBuffer;
			return *this;
		}
	}
}
