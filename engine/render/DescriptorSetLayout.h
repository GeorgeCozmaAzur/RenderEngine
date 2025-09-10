#pragma once
#include <vector>

namespace engine
{
	namespace render
	{
		enum ShaderStage
		{
			VERTEX,
			FRAGMANET,
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
			uint32_t binding;
			ShaderStage stage;
			DescriptorType descriptorType;
		};

		struct DescriptorSetLayout
		{
			void Create(std::vector<LayoutBinding> bindings);
		};
	}
}