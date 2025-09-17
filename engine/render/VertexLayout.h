#pragma once

#include <vector>

namespace engine
{
	namespace render
	{
		/** @brief Vertex layout components */
		typedef enum Component {
			VERTEX_COMPONENT_POSITION = 0x0,
			VERTEX_COMPONENT_NORMAL = 0x1,
			VERTEX_COMPONENT_COLOR = 0x2,
			VERTEX_COMPONENT_UV = 0x3,
			VERTEX_COMPONENT_TANGENT = 0x4,
			VERTEX_COMPONENT_TANGENT4 = 0x5,
			VERTEX_COMPONENT_BITANGENT = 0x6,
			VERTEX_COMPONENT_DUMMY_FLOAT = 0x7,
			VERTEX_COMPONENT_DUMMY_INT = 0x8,
			VERTEX_COMPONENT_DUMMY_VEC4 = 0x9
		} Component;

		/** @brief Stores vertex layout components for model loading and Vulkan vertex input and atribute bindings  */
		struct VertexLayout {

			/** @brief Components used to generate vertices from */
			std::vector<std::vector<Component>> m_components;

			VertexLayout()
			{

			}

			VertexLayout(std::initializer_list<Component> vComponents, std::initializer_list<Component> iComponents)
			{
				m_components.push_back(std::move(vComponents));
				if (iComponents.size() > 0)
					m_components.push_back(std::move(iComponents));
			}

			uint32_t GetComponentSize(Component component)
			{
				switch (component)
				{
				case VERTEX_COMPONENT_UV:			return 2 * sizeof(float);

				case VERTEX_COMPONENT_DUMMY_FLOAT:	return sizeof(float);

				case VERTEX_COMPONENT_DUMMY_INT:	return sizeof(int);

				case VERTEX_COMPONENT_TANGENT4:		return 4 * sizeof(float);

				case VERTEX_COMPONENT_DUMMY_VEC4:	return 4 * sizeof(float);

				default:							return 3 * sizeof(float);
				}
			}

			uint32_t GetVertexSize(int index)
			{
				uint32_t res = 0;

				for (auto& component : m_components[index])
				{
					res += GetComponentSize(component);
				}

				return res;
			}
		};
	}
}
