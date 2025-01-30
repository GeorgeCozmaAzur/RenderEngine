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
		/** @brief Used to parametrize model loading */
		struct ModelCreateInfo2 {
			glm::vec3 center;
			glm::vec3 scale;
			glm::vec2 uvscale;
			VkMemoryPropertyFlags memoryPropertyFlags = 0;

			ModelCreateInfo2() : center(glm::vec3(0.0f)), scale(glm::vec3(1.0f)), uvscale(glm::vec2(1.0f)) {};

			ModelCreateInfo2(glm::vec3 scale, glm::vec2 uvscale, glm::vec3 center)
			{
				this->center = center;
				this->scale = scale;
				this->uvscale = uvscale;
			}

			ModelCreateInfo2(float scale, float uvscale, float center)
			{
				this->center = glm::vec3(center);
				this->scale = glm::vec3(scale);
				this->uvscale = glm::vec2(uvscale);
			}

			ModelCreateInfo2(float scale, float uvscale, glm::vec3 center)
			{
				this->center = center;
				this->scale = glm::vec3(scale);
				this->uvscale = glm::vec2(uvscale);
			}

		};

		class SceneLoaderGltf
		{
		public:
			/** @brief Stores vertex and index base and counts for each part of a model */
			struct ModelPart {
				uint32_t vertexBase;
				uint32_t vertexCount;
				uint32_t indexBase;
				uint32_t indexCount;
			};
			std::vector<ModelPart> parts;

			static const int defaultFlags = 0;//aiProcess_FlipWindingOrder | aiProcess_Triangulate | aiProcess_PreTransformVertices | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals;

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
			std::vector<RenderObject*> render_objects;

			std::string forwardShadersFolder = "basic";
			std::string deferredShadersFolder = "basicdeferred";
			std::string lightingVS = "phong.vert.spv";
			std::string lightingFS = "phong.frag.spv";
			std::string lightingTexturedFS = "phongtextured.frag.spv";
			std::string normalmapVS = "normalmap.vert.spv";
			std::string normalmapFS = "normalmap.frag.spv";

			struct {
				glm::mat4 depthMVP;
			} uboShadowOffscreenVS;
			render::VulkanBuffer* shadow_uniform_buffer;
			render::VulkanRenderPass* shadowPass;
			render::VulkanTexture* shadowmap;
			render::VulkanTexture* shadowmapColor;
			std::vector <RenderObject*> shadow_objects;

			VkRenderPass modelsVkRenderPass;

			render::VulkanTexture* m_placeholder;
			std::vector<render::VulkanTexture*> globalTextures;
			std::vector<render::VulkanTexture*> modelsTextures;
			VkPipelineCache vKpipelineCache;

			render::VulkanBuffer* sceneVertexUniformBuffer;

			render::VulkanBuffer* sceneFragmentUniformBuffer = nullptr;

			std::vector<render::VulkanBuffer*> individualFragmentUniformBuffers;
			std::vector<bool> areTransparents;//TODO store all uniform data in an array because maybe we want to modify it at runtime

			float lightFOV = 45.0f;
			glm::vec4 light_pos = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
			/** @brief Release all Vulkan resources of this model */
			render::VertexLayout vslayout = render::VertexLayout({
			render::VERTEX_COMPONENT_POSITION,
			render::VERTEX_COMPONENT_NORMAL
				}, {});

			render::VertexLayout vlayout = render::VertexLayout({
			render::VERTEX_COMPONENT_POSITION,
			render::VERTEX_COMPONENT_NORMAL,
			render::VERTEX_COMPONENT_UV
				}, {});
			render::VertexLayout vnlayout = render::VertexLayout({
			render::VERTEX_COMPONENT_POSITION,
			render::VERTEX_COMPONENT_NORMAL,
			render::VERTEX_COMPONENT_UV,
			render::VERTEX_COMPONENT_TANGENT,
			render::VERTEX_COMPONENT_BITANGENT
				}, {});

			SceneLoaderGltf::~SceneLoaderGltf();

			void SetCamera(Camera* cam) { m_camera = cam; }

			void CreateShadow(VkQueue copyQueue);
			void CreateShadowObjects(VkPipelineCache pipelineCache);
			void DrawShadowsInSeparatePass(VkCommandBuffer command_buffer);

			render::VulkanDescriptorSetLayout* GetDescriptorSetlayout(std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> layoutBindigs);

			void LoadImages(tinygltf::Model& input, VkQueue queue);

			void LoadMaterials(tinygltf::Model& input, VkQueue queue, bool deferred = false);

			void LoadNode(const tinygltf::Node& inputNode, glm::mat4 parentMatrix, const tinygltf::Model& input);

			std::vector<RenderObject*> LoadFromFile2(const std::string& foldername, const std::string& filename, ModelCreateInfo2* createInfo,
				engine::render::VulkanDevice* device,
				VkQueue copyQueue, VkRenderPass renderPass, VkPipelineCache pipelineCache, bool deferred = false);

			std::vector<RenderObject*> LoadFromFile(const std::string& foldername, const std::string& filename, float scale,
				engine::render::VulkanDevice* device
				, VkQueue copyQueue
				, VkRenderPass renderPass
				, VkPipelineCache pipelineCache
				, bool deferred = false
			);

			void Update(float timer);

			void UpdateView(float timer);

			void destroy()
			{

			}
		};
	}
}