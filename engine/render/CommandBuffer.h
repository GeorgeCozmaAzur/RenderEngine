#pragma once
#include <vector>

namespace engine
{
	namespace render
	{
		class CommandBuffer
		{
		public:
			virtual ~CommandBuffer() {}

			virtual void Begin() = 0;
			virtual void End() = 0;
		};
	}
}