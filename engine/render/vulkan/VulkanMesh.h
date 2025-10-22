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
            uint64_t m_vertexCount = 0;
			uint32_t m_instanceNo = 1;

			render::VulkanBuffer* _vertexBuffer = nullptr;
			render::VulkanBuffer* _indexBuffer = nullptr;
			render::VulkanBuffer* _instanceBuffer = nullptr;

            uint32_t m_vertexInputBinding = 0;
            uint32_t m_instanceInputBinding = 0;

            VulkanMesh(VkDevice device, uint32_t vertexInputBinding, uint32_t instanceInputBinding) :
                _device(device), m_vertexInputBinding(vertexInputBinding), m_instanceInputBinding(instanceInputBinding) {}

            virtual void UpdateInstanceBuffer(void* data, size_t offset, size_t size);

            void Draw(VkCommandBuffer commandBuffer);
            virtual void Draw(CommandBuffer* commandBuffer);
        };
    }
}