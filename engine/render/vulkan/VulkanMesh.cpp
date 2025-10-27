#include "VulkanMesh.h"
#include "VulkanCommandBuffer.h"

namespace engine
{
    namespace render
    {
        void VulkanMesh::Draw(VkCommandBuffer commandBuffer, const std::vector<MeshPart>& parts)
        {
			if (!m_isVisible)
				return;

			VkDeviceSize offsets[1] = { 0 };
			const VkBuffer vertexBuffer = _vertexBuffer->GetVkBuffer();
			vkCmdBindVertexBuffers(commandBuffer, m_vertexInputBinding, 1, &vertexBuffer, offsets);
			VkIndexType indexType = m_indexSize > 2 ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16;//TODO make a switch
			vkCmdBindIndexBuffer(commandBuffer, _indexBuffer->GetVkBuffer(), 0, indexType);
			if (_instanceBuffer && _instanceBuffer->GetVkBuffer() && m_instanceInputBinding > 0)
			{
				const VkBuffer instanceBuffer = _instanceBuffer->GetVkBuffer();
				vkCmdBindVertexBuffers(commandBuffer, m_instanceInputBinding, 1, &instanceBuffer, offsets);
			}

			if(parts.size() == 0)
				vkCmdDrawIndexed(commandBuffer, m_indexCount, m_instanceNo, 0, 0, 0);
			else
			{
				for (const MeshPart& part : parts)
				{
					vkCmdDrawIndexed(commandBuffer, part.indexCount, part.instanceCount, part.firstIndex, part.vertexOffset, part.firstInstance);
				}
			}
        }

        void VulkanMesh::Draw(CommandBuffer* commandBuffer,const std::vector<MeshPart>& parts)
        {
			VulkanCommandBuffer* cb = static_cast<VulkanCommandBuffer*>(commandBuffer);
			Draw(cb->m_vkCommandBuffer, parts);
        }

		void VulkanMesh::UpdateIndexBuffer(void* data, size_t size, size_t offset)
		{
			_indexBuffer->MemCopy(data, size, offset);
		}

		void VulkanMesh::UpdateVertexBuffer(void* data, size_t size, size_t offset)
		{
			_vertexBuffer->MemCopy(data, size, offset);
		}

		void VulkanMesh::UpdateInstanceBuffer(void* data, size_t size, size_t offset)
		{
			_instanceBuffer->MemCopy(data, size);
		}

		void VulkanMesh::FlushIndexBuffer()
		{
			_indexBuffer->Flush();
		}

		void VulkanMesh::FlushVertexBuffer()
		{
			_vertexBuffer->Flush();
		}
    }
}