#pragma once
#include <VulkanTools.h>
#include <Mesh.h>
#include "render/vulkan/VulkanBuffer.h"

namespace engine
{
    namespace render
    {
        class VulkanMesh : public Mesh
        {
        public:
			VkDevice _device = VK_NULL_HANDLE;

			bool m_isVisible = true;

			//uint32_t m_indexCount = 0;
            //uint64_t m_vertexCount = 0;
			uint32_t m_instanceNo = 1;

			render::VulkanBuffer* _vertexBuffer = nullptr;
			render::VulkanBuffer* _indexBuffer = nullptr;
			render::VulkanBuffer* _instanceBuffer = nullptr;

            uint32_t m_vertexInputBinding = 0;
            uint32_t m_instanceInputBinding = 0;

            VulkanMesh(VkDevice device, uint32_t vertexInputBinding, uint32_t instanceInputBinding) :
                _device(device), m_vertexInputBinding(vertexInputBinding), m_instanceInputBinding(instanceInputBinding) {}

            virtual void UpdateIndexBuffer(void* data, size_t size, size_t offset);
            virtual void FlushIndexBuffer();
            virtual void UpdateVertexBuffer(void* data, size_t size, size_t offset);
            virtual void FlushVertexBuffer();
            virtual void UpdateInstanceBuffer(void* data, size_t size, size_t offset);
            virtual void SetVertexBuffer(class render::Buffer* buffer);

            void Draw(VkCommandBuffer commandBuffer, const std::vector<MeshPart>& parts);
            virtual void Draw(CommandBuffer* commandBuffer, const std::vector<MeshPart>& parts = std::vector<MeshPart>());
        };
    }
}