#pragma once
#include <vector>
#include "CommandPool.h"

namespace engine
{
	namespace render
	{
		class CommandBuffer
		{
			CommandPool* m_pool = nullptr;
		public:
			virtual ~CommandBuffer() {}
			CommandBuffer(CommandPool* pool) : m_pool(pool) {};
			virtual void Begin() = 0;
			virtual void End() = 0;
		};
	}
}