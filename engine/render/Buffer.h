#pragma once

#include <vector>

namespace engine
{
	namespace render
	{
		class Buffer
		{
		protected:
			size_t m_size = 0;
			void* m_mapped = nullptr;
		public:
			virtual ~Buffer() {};
			void MemCopy(void* data, size_t size, size_t offset = 0);
		};
	}
}