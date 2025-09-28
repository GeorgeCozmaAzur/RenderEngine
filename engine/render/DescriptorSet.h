#pragma once
#include <DescriptorSetLayout.h>
#include <DescriptorPool.h>

namespace engine
{
	namespace render
	{
		class DescriptorSet
		{
		public:
			virtual void Create(DescriptorSetLayout& layout, DescriptorPool pool) {};
			virtual void Draw(class CommandBuffer* commandBuffer) = 0;
		};
	}
}