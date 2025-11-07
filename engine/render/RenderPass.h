#pragma once
#include "CommandBuffer.h"

namespace engine
{
	namespace render
	{
		struct RenderSubpass
		{
			RenderSubpass() {}
			RenderSubpass(std::vector<uint32_t> i, std::vector<uint32_t> o) :
				inputAttachmanets(i), outputAttachmanets(o) {
			}
			std::vector<uint32_t> inputAttachmanets;
			std::vector<uint32_t> outputAttachmanets;
		};

		class RenderPass
		{
		public:
			virtual ~RenderPass() {};
			virtual void Begin(CommandBuffer *commandBuffer, uint32_t frameBufferIndex = 0) = 0;
			virtual void End(CommandBuffer *commandBuffer, uint32_t frameBufferIndex = 0) = 0;
			virtual void NextSubPass(CommandBuffer* commandBuffer) = 0;
		};
	}
}