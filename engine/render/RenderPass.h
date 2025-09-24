#pragma once
#include "CommandBuffer.h"

namespace engine
{
	namespace render
	{
		class RenderPass
		{
			virtual void Begin(CommandBuffer *commandBuffer) = 0;
			virtual void End(CommandBuffer *commandBuffer) = 0;
		};
	}
}