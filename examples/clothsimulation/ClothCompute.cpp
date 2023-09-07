#include "ClothCompute.h"


namespace engine
{
	namespace scene
	{
		void ClothComputeObject::PrepareStorageBuffers(glm::vec2 size, glm::uvec2 gridSize, render::VulkanDevice *vulkanDevice, VkQueue queue)
		{
			m_gridSize = gridSize;
			m_size = size;
			vertexBuffer.resize(m_gridSize.x * m_gridSize.y);

			float dx = size.x / (m_gridSize.x - 1);
			float dy = size.y / (m_gridSize.y - 1);
			float du = 1.0f / (m_gridSize.x - 1);
			float dv = 1.0f / (m_gridSize.y - 1);

			glm::mat4 transM = glm::translate(glm::mat4(1.0f), glm::vec3(-size.x / 2.0f + 0.0f, -5.0f, -size.y / 2.0f));
			for (uint32_t i = 0; i < m_gridSize.y; i++) {
				for (uint32_t j = 0; j < m_gridSize.x; j++) {
					vertexBuffer[i + j * m_gridSize.y].position = transM * glm::vec4(dx * j, 0.0f, dy * i, 1.0f);
					vertexBuffer[i + j * m_gridSize.y].velocity = glm::vec4(0.0f);
					vertexBuffer[i + j * m_gridSize.y].normal = glm::vec4(0.0f);
					vertexBuffer[i + j * m_gridSize.y].uv = glm::vec4(1.0f - du * i, dv * j, 0.0f, 0.0f);
				}
			}	
			
			VkDeviceSize storageBufferSize = vertexBuffer.size() * sizeof(ClothVertex);

			render::VulkanBuffer* stagingBuffer;
			stagingBuffer = vulkanDevice->CreateStagingBuffer(storageBufferSize, vertexBuffer.data());

			storageBuffers.inbuffer = vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
				, queue, storageBufferSize, nullptr);
			storageBuffers.outbuffer = vulkanDevice->GetGeometryBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
				, queue, storageBufferSize, nullptr);

			VkCommandBuffer copyCmd = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
			VkBufferCopy copyRegion = {};
			copyRegion.size = storageBufferSize;
			vkCmdCopyBuffer(copyCmd, stagingBuffer->m_buffer, storageBuffers.inbuffer->m_buffer, 1, &copyRegion);
			vkCmdCopyBuffer(copyCmd, stagingBuffer->m_buffer, storageBuffers.outbuffer->m_buffer, 1, &copyRegion);
			vulkanDevice->flushCommandBuffer(copyCmd, queue, true);
			vulkanDevice->DestroyBuffer(stagingBuffer);

			m_indices.resize((m_gridSize.y - 1) * (m_gridSize.x * 2 + 1));
			int index = 0;
			for (uint32_t y = 0; y < m_gridSize.y - 1; y++) {
				for (uint32_t x = 0; x < m_gridSize.x; x++) {
					m_indices[index++] = (y + 1) * m_gridSize.x + x;
					m_indices[index++] = (y)*m_gridSize.x + x;
				}
				m_indices[index++] = 0xFFFFFFFF;
			}
		}

		void ClothComputeObject::PrepareUniformBuffer(render::VulkanDevice* vulkanDevice, float projectionWidth, float projectionDepth)
		{
			m_uniformBuffer = vulkanDevice->GetUniformBuffer(sizeof(ubo));
			VK_CHECK_RESULT(m_uniformBuffer->map());

			float dx = m_size.x / (m_gridSize.x - 1);
			float dy = m_size.y / (m_gridSize.y - 1);

			ubo.restDistH = dx;
			ubo.restDistV = dy;
			ubo.restDistD = sqrtf(dx * dx + dy * dy);
			ubo.particleCount = m_gridSize;
			ubo.projectionSizes.x = 1.0f / (projectionWidth * 0.5f);
			ubo.projectionSizes.y = projectionDepth;
		}

		void ClothComputeObject::UpdateUniformBuffer()
		{
			memcpy(m_uniformBuffer->m_mapped, &ubo, sizeof(ubo));
		}
	}
}