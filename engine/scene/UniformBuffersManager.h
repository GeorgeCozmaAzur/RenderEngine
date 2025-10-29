#pragma once
#include "render/vulkan/VulkanDevice.h"
#include "render/vulkan/VulkanBuffer.h"

#include <vector>
#include <unordered_map>

namespace engine
{
	namespace scene
	{
		enum UniformKey
		{
			UNIFORM_PROJECTION,
			UNIFORM_VIEW,
			UNIFORM_PROJECTION_VIEW,
			UNIFORM_CAMERA_POSITION,
			UNIFORM_CAMERA_NEAR_FAR,
			UNIFORM_LIGHT0_SPACE,
			UNIFORM_LIGHT0_SPACE_BIASED,
			UNIFORM_LIGHT0_POSITION,
			UNIFORM_LIGHT0_COLOR,
			UNIFORM_LIGHT1_SPACE,
			UNIFORM_LIGHT1_SPACE_BIASED,
			UNIFORM_LIGHT1_POSITION,
			UNIFORM_LIGHT1_COLOR,
		};

		struct UniformDataEntry
		{
			void* data = nullptr;
			size_t current_offset = 0;
			size_t size = 0;
			bool dirty = true;
		};

		struct BufferEntry
		{
			int mask = 0;
			render::Buffer *staging = nullptr;
			render::Buffer *deviceLocal = nullptr;
		};

		class VulkanDevice;

		class UniformBuffersManager
		{
			//VkDevice _device = VK_NULL_HANDLE;
			render::GraphicsDevice* _device;
			render::DescriptorPool* _descriptorPool;
			//render::CommandBuffer* _copyCommand = nullptr;
			std::vector<BufferEntry> m_buffers;
			std::unordered_map<UniformKey, UniformDataEntry*> m_uniforms;

		public:
			void SetDescriptorPool(render::DescriptorPool* pool) { _descriptorPool = pool; }
			void SetEngineDevice(render::GraphicsDevice* device);
			render::Buffer* GetGlobalUniformBuffer(std::vector<UniformKey>);
			void UpdateGlobalParams(UniformKey, void* value, size_t offset, size_t size);
			void Update(render::CommandBuffer *commandBuffer);
			void Destroy();
			~UniformBuffersManager() { Destroy(); }
		};
	}
}