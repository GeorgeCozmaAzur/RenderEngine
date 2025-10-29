#pragma once
#include "RenderObject.h"
#include "UniformBuffersManager.h"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
//#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>  
//#include <glm/gtx/euler_angles.hpp>

namespace tinygltf
{
	class Node;
	class Model;
}

namespace engine
{
	namespace scene
	{
		class VulkanDevice;
		class Camera;
		class VulkanRenderPass;
		class Texture;

		class SceneLoaderGltf
		{
		public:
			struct Dimension
			{
				glm::vec3 min = glm::vec3(FLT_MAX);
				glm::vec3 max = glm::vec3(-FLT_MAX);
				glm::vec3 size;
			} dim;
			render::VulkanDevice* _device;
			Camera* m_camera = nullptr;
			UniformBuffersManager uniform_manager;
			std::vector<render::VulkanDescriptorSetLayout*> descriptorSetlayouts;
			render::DescriptorPool* descriptorPool;
			std::vector<RenderObject*> render_objects;

			std::string forwardShadersFolder = "scene";
			std::string deferredShadersFolder = "scenedeferred";
			std::string lightingVS = "pbr.vert.spv";
			std::string lightingFS = "pbrtextured.frag.spv";
			std::string normalmapVS = "pbrnormalmap.vert.spv";
			std::string normalmapFS = "pbrtexturednormalmap.frag.spv";

			struct {
				glm::mat4 depthMVP;
			} uboShadowOffscreenVS;
			render::Buffer* shadow_uniform_buffer;
			render::VulkanRenderPass* shadowPass;
			render::VulkanTexture* shadowmap;
			render::VulkanTexture* shadowmapColor;
			std::vector <RenderObject*> shadow_objects;

			VkRenderPass modelsVkRenderPass;

			render::VulkanTexture* m_placeholder;
			std::vector<render::VulkanTexture*> globalTextures;
			std::vector<render::VulkanTexture*> modelsTextures;
			std::vector<int> modelsTexturesIds;
			VkPipelineCache vKpipelineCache;

			render::Buffer* sceneVertexUniformBuffer;
			render::Buffer* sceneFragmentUniformBuffer = nullptr;
			std::vector<render::VulkanBuffer*> individualFragmentUniformBuffers;

			std::vector<bool> areTransparents;//TODO store all uniform data in an array because maybe we want to modify it at runtime

			float lightFOV = 45.0f;
			std::vector<glm::vec4> lightPositions;// = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

			render::VulkanVertexLayout vertexlayout = render::VulkanVertexLayout({
			render::VERTEX_COMPONENT_POSITION,
			render::VERTEX_COMPONENT_NORMAL,
			render::VERTEX_COMPONENT_UV
				}, {});
			render::VulkanVertexLayout vertexlayoutNormalmap = render::VulkanVertexLayout({
			render::VERTEX_COMPONENT_POSITION,
			render::VERTEX_COMPONENT_NORMAL,
			render::VERTEX_COMPONENT_UV,
			render::VERTEX_COMPONENT_TANGENT4
				}, {});

			SceneLoaderGltf::~SceneLoaderGltf();

			void SetCamera(Camera* cam) { m_camera = cam; }

			void CreateShadow(VkQueue copyQueue);
			void CreateShadowObjects(VkPipelineCache pipelineCache);
			void DrawShadowsInSeparatePass(render::CommandBuffer* command_buffer);

			render::VulkanDescriptorSetLayout* GetDescriptorSetlayout(std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> layoutBindigs);

			void LoadImages(tinygltf::Model& input, VkQueue queue);

			void LoadMaterials(tinygltf::Model& input, VkQueue queue, bool deferred = false);

			void LoadNode(const tinygltf::Node& inputNode, glm::mat4 parentMatrix, const tinygltf::Model& input);

			std::vector<RenderObject*> LoadFromFile(const std::string& foldername, const std::string& filename, float scale,
				engine::render::VulkanDevice* device
				, VkQueue copyQueue
				, VkRenderPass renderPass
				, VkPipelineCache pipelineCache
				, bool deferred = false
			);

			void UpdateLights(int index, glm::vec4 position, glm::vec4 color, float timer, VkQueue queue);

			void Update(float timer, VkQueue queue);

			void UpdateView(float timer, VkQueue queue);
		};
	}
}