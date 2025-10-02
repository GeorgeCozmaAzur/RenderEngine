#include "VulkanMesh.h"

namespace engine
{
    namespace render
    {
        void VulkanMesh::Draw(VkCommandBuffer* commandBuffer)
        {
			if (!m_isVisible)
				return;

			VkDeviceSize offsets[1] = { 0 };
			const VkBuffer vertexBuffer = _vertexBuffer->GetVkBuffer();
			vkCmdBindVertexBuffers(*commandBuffer, m_vertexInputBinding, 1, &vertexBuffer, offsets);
			vkCmdBindIndexBuffer(*commandBuffer, _indexBuffer->GetVkBuffer(), 0, VK_INDEX_TYPE_UINT32);
			if (_instanceBuffer && _instanceBuffer->GetVkBuffer() && m_instanceInputBinding > 0)
			{
				const VkBuffer instanceBuffer = _instanceBuffer->GetVkBuffer();
				vkCmdBindVertexBuffers(*commandBuffer, m_instanceInputBinding, 1, &instanceBuffer, offsets);
			}

			vkCmdDrawIndexed(*commandBuffer, m_indexCount, m_instanceNo, 0, 0, 0);
        }

        void VulkanMesh::Draw(CommandBuffer* commandBuffer)
        {

        }
    }
}