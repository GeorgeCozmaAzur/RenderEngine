#pragma once
#include <DescriptorSetLayout.h>
#include <DescriptorPool.h>
#include <Pipeline.h>

namespace engine
{
	namespace render
	{
		class DescriptorSet
		{
		public:
			virtual ~DescriptorSet() {}
			virtual void Create(DescriptorSetLayout& layout, DescriptorPool pool) {};
			virtual void Draw(class CommandBuffer* commandBuffer, Pipeline* pipeline = nullptr) = 0;
		};
	}
}