#include "Buffer.h"
#include <assert.h>

namespace engine
{
	namespace render
	{
		void Buffer::MemCopy(void* data, size_t size, size_t offset)
		{
			assert(m_mapped);
			memcpy((char*)m_mapped + offset, data, size);
		}
	}
}
