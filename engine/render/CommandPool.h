#pragma once
#include <vector>
namespace engine
{
	namespace render
	{
		class CommandPool
		{
		protected:
			std::vector<class CommandBuffer*> m_commandBuffers;
		public:
			uint32_t m_queueIndex;
			bool m_primary;
			virtual ~CommandPool();
			virtual class CommandBuffer* GetCommandBuffer() = 0;
			virtual void DestroyCommandBuffer(class CommandBuffer* cmd) = 0;
			virtual void Reset() = 0;
		};
	}
}