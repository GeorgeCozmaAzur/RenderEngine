#include "CommandPool.h"
#include "CommandBuffer.h"
#include <assert.h>

namespace engine
{
	namespace render
	{
		CommandPool::~CommandPool() { for (auto cmd : m_commandBuffers) delete cmd; }
	}
}
