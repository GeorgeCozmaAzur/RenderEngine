#pragma once

#include <vector>
#include <string>

namespace engine
{
	namespace render
	{
		/** @brief Vertex layout components */
		typedef enum Component {
			VERTEX_COMPONENT_POSITION = 0,
			VERTEX_COMPONENT_POSITION2D,
			VERTEX_COMPONENT_NORMAL,
			VERTEX_COMPONENT_COLOR,
			VERTEX_COMPONENT_COLOR_UINT,
			VERTEX_COMPONENT_UV,
			VERTEX_COMPONENT_TANGENT,
			VERTEX_COMPONENT_TANGENT4,
			VERTEX_COMPONENT_BITANGENT,
			VERTEX_COMPONENT_DUMMY_FLOAT,
			VERTEX_COMPONENT_DUMMY_INT,
			VERTEX_COMPONENT_DUMMY_VEC4
		} Component;

		/** @brief Stores vertex layout components for model loading and Vulkan vertex input and atribute bindings  */
		struct VertexLayout {

			/** @brief Components used to generate vertices from */
			std::vector<std::vector<Component>> m_components;

			VertexLayout()
			{

			}

			virtual ~VertexLayout()
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
				case VERTEX_COMPONENT_UV:
				case VERTEX_COMPONENT_POSITION2D:	return 2 * sizeof(float);

				case VERTEX_COMPONENT_DUMMY_FLOAT:	return sizeof(float);

				case VERTEX_COMPONENT_DUMMY_INT:	return sizeof(int);

				case VERTEX_COMPONENT_COLOR_UINT:	return sizeof(uint32_t);

				case VERTEX_COMPONENT_TANGENT4:		return 4 * sizeof(float);

				case VERTEX_COMPONENT_DUMMY_VEC4:	return 4 * sizeof(float);

				default:							return 3 * sizeof(float);
				}
			}

			std::string GetComponentName(Component component)
			{
				switch (component)
				{
					case VERTEX_COMPONENT_POSITION2D:
					case VERTEX_COMPONENT_POSITION:	return "POSITION";

					case VERTEX_COMPONENT_UV:			return "TEXCOORD";
				
					case VERTEX_COMPONENT_NORMAL:	return "NORMAl";

					case VERTEX_COMPONENT_COLOR:
					case VERTEX_COMPONENT_COLOR_UINT:		return "COLOR";

					case VERTEX_COMPONENT_TANGENT:	return "TANGENT";

					case VERTEX_COMPONENT_BITANGENT:	return "BITANGENT";

					default:							return "WHATEVER";
				}
			}

			uint32_t GetVertexSize(int index)
			{
				uint32_t res = 0;

				if(index < m_components.size())
				for (auto& component : m_components[index])
				{
					res += GetComponentSize(component);
				}

				return res;
			}
		};
	}
}
