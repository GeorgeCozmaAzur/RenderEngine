#include "UniformBuffersManager.h"
#include "render/vulkan/VulkanDevice.h"

namespace engine
{
	namespace scene
	{
		size_t GetSize(UniformKey key)
		{
			switch (key)
			{
			case UNIFORM_PROJECTION:			return sizeof(float) * 16;
			case UNIFORM_VIEW:					return sizeof(float) * 16;
			case UNIFORM_PROJECTION_VIEW:		return sizeof(float) * 16;
			case UNIFORM_CAMERA_POSITION:		return sizeof(float) * 3;
			case UNIFORM_CAMERA_NEAR_FAR:		return sizeof(float) * 4;
			case UNIFORM_LIGHT0_SPACE:			return sizeof(float) * 16;
			case UNIFORM_LIGHT0_SPACE_BIASED:	return sizeof(float) * 16;
			case UNIFORM_LIGHT0_POSITION:		return sizeof(float) * 4;
			case UNIFORM_LIGHT0_COLOR:			return sizeof(float) * 4;
			default:							return 0;
			}
		}
		void UniformBuffersManager::SetEngineDevice(render::VulkanDevice* device) {
			_engine_device = device; _copyCommand = _engine_device->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, false);
		}

		render::VulkanBuffer* UniformBuffersManager::GetGlobalUniformBuffer(std::vector<UniformKey> keys)
		{
			std::map<int, render::VulkanBuffer*>::iterator it;

			int mask = 0;
			for (auto key : keys)
			{
				mask |= (1 << key);
			}

			for (BufferEntry entry : m_buffers)
			{
				if (entry.mask == mask)
					return entry.deviceLocal;
			}

			for (auto key : keys)
			{
				UniformDataEntry* data = new UniformDataEntry;
				m_uniforms.insert(std::pair<UniformKey, UniformDataEntry*>(key, data));
			}

			size_t total_buffer_size = 0;

			for (auto key : keys)
			{
				total_buffer_size += GetSize(key);
			}

			BufferEntry bentry; bentry.mask = mask;
			bentry.staging = _engine_device->GetBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				total_buffer_size);
			bentry.deviceLocal = _engine_device->GetBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				total_buffer_size);

			VK_CHECK_RESULT(bentry.staging->Map());
			
			m_buffers.push_back(bentry);

			return bentry.deviceLocal;
		}

		void UniformBuffersManager::UpdateGlobalParams(UniformKey key, void* value, size_t offset, size_t size)
		{
			auto kk = m_uniforms.find(key);
			if (kk != m_uniforms.end())
			{
				kk->second->current_offset = offset;
				kk->second->data = value;
				kk->second->size = size;
				kk->second->dirty = true;
			}
			else
			{
				UniformDataEntry* data = new UniformDataEntry; data->data = value; data->current_offset = offset; data->size = size; data->dirty = true;
				m_uniforms.insert(std::pair<UniformKey, UniformDataEntry*>(key, data));
			}
		}

		void UniformBuffersManager::Update(VkQueue queue)
		{
			for (auto buffer : m_buffers)
			{
				bool alldirty = false;
				size_t old_size = 0;
				for (auto value : m_uniforms)
				{
					int KK = 1 << value.first;
					if (buffer.mask & KK)
					{
						if (value.second->dirty)//TODO do dirty functionality
						{
							buffer.staging->MemCopy(value.second->data, value.second->size, old_size);
							value.second->dirty = false;
							alldirty = true;
						}
						old_size += value.second->size;
					}
				}
				if (alldirty)
				{
					VkCommandBufferBeginInfo cmdBufInfo{};
					cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
					VK_CHECK_RESULT(vkBeginCommandBuffer(_copyCommand, &cmdBufInfo));
					_engine_device->CopyBuffer(buffer.staging, buffer.deviceLocal, _copyCommand);
					_engine_device->FlushCommandBuffer(_copyCommand, queue, false);
				}
			}
		}

		void UniformBuffersManager::Destroy()
		{
			for (auto value : m_uniforms)
			{
				delete value.second;
			}
			//TODO don't rely on outside data so much
		}
	}
}