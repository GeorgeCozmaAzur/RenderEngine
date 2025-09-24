#pragma once
#include "VertexLayout.h"
#include "Buffer.h"
#include "Texture.h"
#include "DescriptorPool.h"
#include "DescriptorSetLayout.h"
#include "DescriptorSet.h"
#include "RenderPass.h"
#include "Pipeline.h"
#include "CommandBuffer.h"
#include "Mesh.h"
#include <vector>

namespace engine
{
	namespace render
	{
		class GraphicsDevice
		{
			virtual Buffer* GetBuffer(size_t size, void* data, DescriptorPool* descriptorPool) = 0;

			virtual Texture* GetTexture(TextureData* data, DescriptorPool *descriptorPool) = 0;

			virtual DescriptorPool* GetDescriptorPool() = 0;

			virtual DescriptorSetLayout* GetDescriptorSetLayout(std::vector<LayoutBinding> bindings) = 0;

			virtual DescriptorSet* GetDescriptorSet(DescriptorSetLayout* layout, Buffer** buffers, Texture** textures) = 0;

			virtual RenderPass* GetRenderPass(uint32_t width, uint32_t height, Texture *colorTexture, Texture *depthTexture) = 0;

			virtual Pipeline* GetPipeLine(std::string vertexFile, std::string fragmentFile, VertexLayout vertexLayout, DescriptorSetLayout descriptorSetlayout, PipelineProperties properties, RenderPass* renderPass) = 0;

			virtual CommandBuffer* GetCommandBuffer() = 0;

			virtual Mesh* GetMesh(MeshData* data) = 0;
		};
	}
}