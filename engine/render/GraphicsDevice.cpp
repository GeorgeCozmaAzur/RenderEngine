#include "GraphicsDevice.h"

namespace engine
{
	namespace render
	{
		GraphicsDevice::~GraphicsDevice()
		{
            for (auto layout : m_descriptorSetLayouts)
                delete layout;
            for (auto desc : m_descriptorSets)
                delete desc;
            for (auto desc : m_descriptorPools)
                delete desc;
            for (auto pipeline : m_pipelines)
                delete pipeline;
            for (auto buffer : m_buffers)
                delete buffer;
            for (auto m : m_meshes)
                delete m;
            for (auto tex : m_textures)
                delete tex;
            for (auto pass : m_renderPasses)
                delete pass;
            for (auto cb : m_commandBuffers)
                delete cb;
		}

        void GraphicsDevice::FreeLoadStaggingBuffers()
        {
            for (auto buffer : m_loadStaggingBuffers)
                delete buffer;
        }
	}
}
