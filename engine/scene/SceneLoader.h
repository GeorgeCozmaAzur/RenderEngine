#include "RenderObject.h"
#include "UniformBuffersManager.h"
#include <glm/glm.hpp>   
#include <assimp/postprocess.h>


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

		class SceneLoader
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

			static const int defaultFlags = aiProcess_FlipWindingOrder | aiProcess_Triangulate | aiProcess_PreTransformVertices | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals;

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
			VkDescriptorPool descriptorPool;
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

			std::vector<render::VulkanTexture*> globalTextures;

			render::VulkanBuffer* sceneVertexUniformBuffer;

			render::VulkanBuffer* sceneFragmentUniformBuffer = nullptr;

			std::vector<render::VulkanBuffer*> individualFragmentUniformBuffers;
			std::vector<bool> areTransparents;//TODO store all uniform data in an array because maybe we want to modify it at runtime

			float lightFOV = 45.0f;
			glm::vec4 light_pos = glm::vec4(0.0f, 10.0f, 0.0f, 1.0f);
			/** @brief Release all Vulkan resources of this model */

			render::VulkanVertexLayout vlayout = render::VulkanVertexLayout({
			render::VERTEX_COMPONENT_POSITION,
			render::VERTEX_COMPONENT_NORMAL,
			render::VERTEX_COMPONENT_UV
				}, {});
			render::VulkanVertexLayout vnlayout = render::VulkanVertexLayout({
			render::VERTEX_COMPONENT_POSITION,
			render::VERTEX_COMPONENT_NORMAL,
			render::VERTEX_COMPONENT_UV,
			render::VERTEX_COMPONENT_TANGENT,
			render::VERTEX_COMPONENT_BITANGENT
				}, {});

			SceneLoader::~SceneLoader();

			void SetCamera(Camera* cam) { m_camera = cam; }

			void CreateShadow(VkQueue copyQueue);
			void CreateShadowObjects(VkPipelineCache pipelineCache);
			void DrawShadowsInSeparatePass(render::CommandBuffer* command_buffer);

			render::VulkanDescriptorSetLayout* GetDescriptorSetlayout(std::vector<std::pair<VkDescriptorType, VkShaderStageFlags>> layoutBindigs);

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

			void Update(float timer, VkQueue copyQueue);

			void UpdateView(float timer, VkQueue copyQueue);

			void destroy()
			{

			}
		};
	}
}