#include "UniformBuffersManager.h"
#include "render/VulkanDevice.hpp"

namespace engine
{
	namespace scene
	{
		size_t GetSize(UniformKey key)
		{
			switch (key)
			{
			case UNIFORM_PROJECTION:		return sizeof(float) * 16;
			case UNIFORM_VIEW:				return sizeof(float) * 16;
			case UNIFORM_CAMERA_POSITION:	return sizeof(float) * 3;
			case UNIFORM_LIGHT0_SPACE:		return sizeof(float) * 16;
			case UNIFORM_LIGHT0_POSITION:	return sizeof(float) * 4;
			default:						return 0;
			}
		}

		render::VulkanBuffer* UniformBuffersManager::GetGlobalUniformBuffer(std::vector<UniformKey> keys)
		{
			std::map<int, render::VulkanBuffer*>::iterator it;

			int mask = 0;
			for (auto key : keys)
			{
				mask |= (1 << key);
			}

			it = m_buffers.find(mask);
			if (it != m_buffers.end())
			{
				return it->second;
			}

			size_t total_buffer_size = 0;

			for (auto key : keys)
			{
				total_buffer_size += GetSize(key);
			}

			render::VulkanBuffer* buffer = _engine_device->GetBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				total_buffer_size);

			VK_CHECK_RESULT(buffer->map());
			m_buffers.insert(std::pair<int, render::VulkanBuffer*>(mask, buffer));

			return buffer;
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

		void UniformBuffersManager::Update()
		{
			for (auto buffer : m_buffers)
			{
				size_t old_size = 0;
				for (auto value : m_uniforms)
				{
					int KK = 1 << value.first;
					if (buffer.first & KK)
					{
						//if (value.second->dirty)//TODO do dirty functionality
						{
							memcpy((char*)buffer.second->m_mapped + old_size, value.second->data, value.second->size);
							value.second->dirty = false;
						}
						old_size += value.second->size;
					}
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