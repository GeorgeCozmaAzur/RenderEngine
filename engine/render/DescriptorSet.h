#pragma once
#include <DescriptorSetLayout.h>
#include <DescriptorPool.h>

namespace engine
{
	namespace render
	{
		class DescriptorSet
		{
			virtual void Create(DescriptorSetLayout& layout, DescriptorPool pool) {};
			//virtual void Draw() = 0;
		};
	}
}