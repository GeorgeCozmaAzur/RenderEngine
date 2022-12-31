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
			if (!keepRAMData)
			{
				delete[]m_vertices;
				m_vertices = nullptr;
			}
		}
		void Geometry::SetIndexBuffer(render::VulkanBuffer* buf, bool keepRAMData)
		{
			_indexBuffer = buf;
			if (!keepRAMData)
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
			memcpy(_indexBuffer->m_mapped, data, size);
		}
		void Geometry::UpdateInstanceBuffer(void* data, size_t offset, size_t size)
		{
			memcpy(_instanceBuffer->m_mapped, data, size);
		}

		void Geometry::Draw(VkCommandBuffer* commandBuffer, uint32_t vertexInputBinding, uint32_t instanceInputBinding)
		{
			if (!m_isVisible)
				return;

			VkDeviceSize offsets[1] = { 0 };
			vkCmdBindVertexBuffers(*commandBuffer, vertexInputBinding, 1, &_vertexBuffer->m_buffer, offsets);
			vkCmdBindIndexBuffer(*commandBuffer, _indexBuffer->m_buffer, 0, VK_INDEX_TYPE_UINT32);
			if (_instanceBuffer && _instanceBuffer->m_buffer && instanceInputBinding > 0)
				vkCmdBindVertexBuffers(*commandBuffer, instanceInputBinding, 1, &_instanceBuffer->m_buffer, offsets);

			vkCmdDrawIndexed(*commandBuffer, m_indexCount, m_instanceNo, 0, 0, 0);
		}
	}
}
