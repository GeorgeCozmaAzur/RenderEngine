#pragma once
#include <vector>

namespace engine
{
	namespace render
	{
		enum ShaderStage
		{
			VERTEX,
			FRAGMENT,
			COMPUTE
		};

		enum DescriptorType
		{
			TEXTURE,
			UNIFORM_BUFFER,
			STORAGE_BUFFER,
			INPUT_ATTACHMENT
		};

		struct LayoutBinding
		{
			DescriptorType descriptorType;
			ShaderStage stage;
		};

		struct DescriptorSetLayout
		{
			std::vector<LayoutBinding> m_bindings;
			DescriptorSetLayout(std::vector<LayoutBinding> bindings) : m_bindings(bindings) {};
		};
	}
}