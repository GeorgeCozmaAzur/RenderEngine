#pragma once

#include "ApplicationBase.h"

#include "vulkan/vulkan.h"

#include "VulkanTools.h"
#include "VulkanDebug.h"

#include "render/vulkan/VulkanDevice.h"
#include "render/vulkan/VulkanSwapChain.h"
#include "scene/UIOverlay.h"

using namespace engine;

class VulkanApplication : public ApplicationBase
{
private:	
	// Called if the window is resized and some resources have to be recreatesd
	
	//void HandleMouseMove(int32_t x, int32_t y);
protected:
	// Vulkan instance, stores all per-application states
	VkInstance instance = VK_NULL_HANDLE;
	/** @brief Set of physical device features to be enabled for this example (must be set in the derived constructor) */
	VkPhysicalDeviceFeatures enabledFeatures{};
	/** @brief Set of device extensions to be enabled for this example (must be set in the derived constructor) */
	std::vector<const char*> enabledDeviceExtensions;
	std::vector<const char*> enabledInstanceExtensions;
	/** @brief Optional pNext structure for passing extension structures to device creation */
	void* deviceCreatepNextChain = nullptr;
	/** @brief Logical device, application's view of the physical device (GPU) */
	VkDevice device;
	// Handle to the device graphics queue that command buffers are submitted to
	VkQueue queue;
	// Handle to the device presentation queue
	VkQueue presentationQueue;
	// Depth buffer format (selected during Vulkan initialization)
	VkFormat depthFormat;
	// Command buffer pool
	//render::CommandPool* cmdPool;
	/** @brief Pipeline stages used to wait at for graphics queue submissions */
	VkPipelineStageFlags submitPipelineStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	// Contains command buffers and semaphores to be presented to the queue
	VkSubmitInfo submitInfo;
	// Command buffers used for rendering
	std::vector <std::vector<VkCommandBuffer>> allvkDrawCommandBuffers;
	//std::vector<VkCommandBuffer> drawCommandBuffers;
	std::vector<VkCommandBuffer> drawUICommandBuffers;
	// Global render pass for frame buffer writes
	//VkRenderPass renderPass;
	// List of available frame buffers (same as number of swap chain images)
	//std::vector<render::VulkanFrameBuffer*> frameBuffers;
	render::VulkanRenderPass *mainRenderPass = nullptr;
	
	// Descriptor set pool
	//VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
	// Pipeline cache object
	VkPipelineCache pipelineCache;
	// Wraps the swap chain to present images (framebuffers) to the windowing system
	render::VulkanSwapChain swapChain;
	// Synchronization semaphores
	//struct {
		// Swap chain image presentation
	std::vector < VkSemaphore> presentCompleteSemaphores;
		// Command buffer submission and execution
	std::vector < VkSemaphore> renderCompleteSemaphores;
	//} semaphores;

	std::vector<VkFence> submitFences;

	//graphical resources
	//std::vector<Geometry *> m_geometries;
public: 
	VulkanApplication::VulkanApplication(bool enableValidation);

	engine::scene::UIOverlay UIOverlay;

	/** @brief Encapsulated physical and logical vulkan device */
	engine::render::VulkanDevice* vulkanDevice = nullptr;

	VkClearColorValue defaultClearColor = { { 0.25f, 0.25f, 0.25f, 1.0f } };

	uint32_t apiVersion = VK_API_VERSION_1_2;

	class engine::render::VulkanTexture* depthStencil;

	/**
	* Create the application wide Vulkan instance
	*
	* @note Virtual, can be overriden by derived example class for custom instance creation
	*/
	virtual VkResult CreateInstance(bool enableValidation);

	// Create swap chain images
	void SetupSwapChain();
	// Creates a new (graphics) command pool object storing command buffers
	void CreateCommandPool();
	// Create command buffers for drawing commands
	virtual void CreateAllCommandBuffers();
	virtual std::vector<render::CommandBuffer*> CreateCommandBuffers();
	// Destroy all command buffers and set their handles to VK_NULL_HANDLE
	// May be necessary during runtime if options are toggled 
	void DestroyCommandBuffers();
	virtual void SetupDepthStencil();
	// Create framebuffers for all requested swap chain images
	// Can be overriden in derived class to setup a custom framebuffer (e.g. for MSAA)
	virtual void SetupFrameBuffer();
	// Setup a default render pass
	// Can be overriden in derived class to setup a custom render pass (e.g. for MSAA)
	virtual void SetupRenderPass();
	// Create a cache pool for rendering pipelines
	void CreatePipelineCache();

	// Setup the vulkan instance, enable required extensions and connect to the physical device (GPU)
	virtual bool InitAPI();

	//Prepare ui elements
	void PrepareUI();

	// Pure virtual render function (override in derived class)
	virtual void Render();

	// Called when view change occurs
	// Can be overriden in derived class to e.g. update uniform buffers 
	// Containing view dependant matrices
	virtual void ViewChanged();
	/** @brief (Virtual) Called after a key was pressed, can be used to do custom key handling */
	virtual void KeyPressed(uint32_t);
	/** @brief (Virtual) Called after th mouse cursor moved and before internal events (like camera rotation) is handled */
	virtual void MouseMoved(double x, double y, bool &handled);
	// Called when the window has been resized
	// Can be overriden in derived class to recreate or rebuild resources attached to the frame buffer / swapchain
	virtual void WindowResized();
	virtual void WindowResize();
	// Pure virtual function to be overriden by the dervice class
	// Called in case of an event where e.g. the framebuffer has to be rebuild and thus
	// all command buffers that may reference this
	virtual void BuildCommandBuffers();

	virtual void SubmitOnQueue(class render::CommandBuffer* commandBuffer);

	/** @brief (Virtual) Called after the physical device features have been read, can be used to set features to enable on the device */
	virtual void GetEnabledFeatures();

	// Render one frame of a render loop on platforms that sync rendering
	//void UpdateFrame();

	virtual void UpdateOverlay();
	void DrawUI(render::CommandBuffer* commandBuffer);

	virtual void WaitForDevice();

	/** @brief (Virtual) Called when the UI overlay is updating, can be used to add custom elements to the overlay */
	virtual void OnUpdateUIOverlay(engine::scene::UIOverlay *overlay);

	virtual void DrawFullScreenQuad(render::CommandBuffer* commandBuffer);

	virtual ~VulkanApplication();
};
