#pragma once
#include <VulkanTools.h>
#include "render/vulkan/VulkanBuffer.h"

namespace engine
{
	namespace scene
	{
		struct Geometry
		{
			VkDevice _device = VK_NULL_HANDLE;

			bool m_isVisible = true;

			float* m_vertices = nullptr;
			uint32_t* m_indices = nullptr;//TODO what we do with these when copying the geometry

			uint32_t m_indexCount = 0;
			uint64_t m_vertexCount = 0;
			uint64_t m_vertexSize = 0;
			uint64_t m_verticesSize = 0;
			uint32_t m_instanceNo = 1;

			render::VulkanBuffer* _vertexBuffer = nullptr;
			render::VulkanBuffer* _indexBuffer = nullptr;
			render::VulkanBuffer* _instanceBuffer = nullptr;

			void SetVertexBuffer(render::VulkanBuffer* buf, bool keepRAMData = false);
			void SetIndexBuffer(render::VulkanBuffer* buf, bool keepRAMData = false);
			void SetInstanceBuffer(render::VulkanBuffer* buf, bool keepRAMData = false);

			void UpdateIndexBuffer(void* data, size_t offset, size_t size);
			void UpdateInstanceBuffer(void* data, size_t offset, size_t size);

			void Draw(VkCommandBuffer* commandBuffer, uint32_t vertexInputBinding, uint32_t instanceInputBinding);
			void Destroy();
			~Geometry() { Destroy(); }

			Geometry& operator=(const Geometry& other);
		};
	}
}
