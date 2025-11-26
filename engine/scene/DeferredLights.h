#pragma once

#include "SimpleModel.h"
#include "render/vulkan/VulkanDevice.h"

namespace engine
{
	namespace scene
	{
		struct PointLight
		{
			glm::vec4 PositionAndRadius;
			glm::vec4 Color;
		};

		class DeferredLights : public SimpleModel
		{
			render::GraphicsDevice* vulkanDevice;
		public:
			std::vector<glm::vec4> m_pointLights;
			class render::CommandBuffer* _commandBuffer = nullptr;
			std::string shadersPath;
			std::string vertext;
			std::string fragext;

			void Init(render::Buffer *ub, render::GraphicsDevice*device, render::DescriptorPool* descriptorPool, render::RenderPass* renderPass, int lightsNumber,
				render::Texture *positions, render::Texture* normals, render::Texture* roughnessMetallic = nullptr, render::Texture* albedo = nullptr);
			void Update();
			~DeferredLights();
		};

		class DeferredLightDirectional : public RenderObject
		{
		public:
			glm::vec4 m_light;
			std::string shadersPath;
			std::string vertext;
			std::string fragext;

			class render::CommandBuffer* _commandBuffer = nullptr;
			void Init(render::Buffer* ub, render::GraphicsDevice* device, render::DescriptorPool* descriptorPool, render::RenderPass* renderPass,
				render::Texture* positions, render::Texture* normals, render::Texture* roughnessMetallic = nullptr, render::Texture* albedo = nullptr, render::Texture* shadowmap = nullptr);
		};
	}
}