#pragma once
#include "DescriptorSetLayout.h"
#include <vector>

namespace engine
{
	namespace render
	{
		struct DescriptorPoolSize
		{
			DescriptorType type;
			uint32_t size;
		};

		class DescriptorPool
		{
		protected:
			uint32_t m_maxSets;
			std::vector<DescriptorPoolSize> m_poolSizes;

		public:
			DescriptorPool(std::vector<DescriptorPoolSize> poolSizes, uint32_t maxSets) : m_poolSizes(poolSizes), m_maxSets(maxSets) {}
			virtual void Draw(class CommandBuffer* commandBuffer) {}
			virtual ~DescriptorPool() = default;
		};
	}
}