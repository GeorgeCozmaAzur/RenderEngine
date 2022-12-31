#include "VertexLayout.h"

namespace engine
{
	namespace render
	{
		uint32_t VertexLayout::GetVertexInputBinding(VkVertexInputRate inputRate)
		{
			uint32_t binding = 0;
			for (auto inputBinding : m_vertexInputBindings)
			{
				if (inputRate == inputBinding.inputRate)
					return inputBinding.binding;
			}
			return binding;
		}

		void VertexLayout::CreateVertexDescription()
		{
			uint32_t location = 0;
			for (int i = 0;i < m_components.size();i++)
			{
				uint32_t offset = 0;
				
				for (auto component : m_components[i])
				{
					uint32_t current_offset = 0;
					current_offset += GetComponentSize(component);
					m_vertexInputAttributes.push_back(engine::initializers::vertexInputAttributeDescription(i, location++, GetVkFormat(component), offset));
					offset += current_offset;
				}
				if (offset > 0)
				{
					VkVertexInputRate vertex_input_rate = (i == 0) ? VK_VERTEX_INPUT_RATE_VERTEX : VK_VERTEX_INPUT_RATE_INSTANCE;
					m_vertexInputBindings.push_back(engine::initializers::vertexInputBindingDescription((uint32_t)m_vertexInputBindings.size(), offset, vertex_input_rate));
				}
			}
		}
	}
}