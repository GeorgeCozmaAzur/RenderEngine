#pragma once

#include <VulkanTools.h>
#include "VertexLayout.h"
#include <vector>

namespace engine
{
	namespace render
	{
		/** @brief Stores vertex layout components for model loading and Vulkan vertex input and atribute bindings  */
		struct VulkanVertexLayout : public VertexLayout {

			std::vector<VkVertexInputBindingDescription> m_vertexInputBindings;

			std::vector<VkVertexInputAttributeDescription> m_vertexInputAttributes;

			VulkanVertexLayout()
			{

			}

			VulkanVertexLayout(std::initializer_list<Component> vComponents, std::initializer_list<Component> iComponents) : VertexLayout(vComponents, iComponents)
			{
				CreateVertexDescription();
			}

			VkFormat GetVkFormat(Component component)
			{
				switch (component)
				{
				case VERTEX_COMPONENT_UV:
				case VERTEX_COMPONENT_POSITION2D:	return VK_FORMAT_R32G32_SFLOAT;

				case VERTEX_COMPONENT_DUMMY_FLOAT:	return VK_FORMAT_R32_SFLOAT;

				case VERTEX_COMPONENT_DUMMY_INT:	return VK_FORMAT_R32_SFLOAT;

				case VERTEX_COMPONENT_COLOR_UINT:	return VK_FORMAT_R8G8B8A8_UNORM;

				case VERTEX_COMPONENT_POSITION4D:
				case VERTEX_COMPONENT_COLOR4:
				case VERTEX_COMPONENT_TANGENT4:
				case VERTEX_COMPONENT_DUMMY_VEC4:	return VK_FORMAT_R32G32B32A32_SFLOAT;

				default:							return VK_FORMAT_R32G32B32_SFLOAT;
				}
			}

			void CreateVertexDescription();

			uint32_t GetVertexInputBinding(VkVertexInputRate inputRate);
		};
	}
}
