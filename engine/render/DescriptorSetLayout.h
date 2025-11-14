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
			UNIFORM_BUFFER,
			UNIFORM_BUFFER_DYNAMIC,
			IMAGE_SAMPLER,
			INPUT_STORAGE_BUFFER,
			OUTPUT_STORAGE_BUFFER,
			STORAGE_IMAGE,
			INPUT_ATTACHMENT,
			DSV,
			RTV
		};

		struct LayoutBinding
		{
			DescriptorType descriptorType;
			ShaderStage stage;
		};

		struct DescriptorSetLayout
		{
			std::vector<LayoutBinding> m_bindings;
			DescriptorSetLayout() {}
			DescriptorSetLayout(std::vector<LayoutBinding> bindings) : m_bindings(bindings) {}
			virtual ~DescriptorSetLayout() {}
		};
	}
}