#pragma once
#include "CommandBuffer.h"

namespace engine
{
	namespace render
	{
		class RenderPass
		{
		public:
			virtual ~RenderPass() {};
			virtual void Begin(CommandBuffer *commandBuffer, uint32_t frameBufferIndex = 0) = 0;
			virtual void End(CommandBuffer *commandBuffer, uint32_t frameBufferIndex = 0) = 0;
		};
	}
}