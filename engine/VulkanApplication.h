/*
* Vulkan Example base class
*
* Copyright (C) by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#pragma once

#ifdef _WIN32
#pragma comment(linker, "/subsystem:windows")
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#include <ShellScalingAPI.h>
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
#include <android/native_activity.h>
#include <android/asset_manager.h>
#include <android_native_app_glue.h>
#include <sys/system_properties.h>
#include "VulkanAndroid.h"
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"
#elif defined(_DIRECT2DISPLAY)
//
#elif defined(VK_USE_PLATFORM_XCB_KHR)
#include <xcb/xcb.h>
#endif

#include <iostream>
#include <chrono>
#include <sys/stat.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <string>
#include <array>
#include <numeric>

#include "vulkan/vulkan.h"

#include "VulkanTools.h"
#include "VulkanDebug.h"

#include "render/VulkanDevice.h"
#include "render/VulkanSwapChain.h"
#include "scene/UIOverlay.h"
#include "scene/Camera.h"

using namespace engine;

class VulkanApplication
{
private:	
	/** brief Indicates that the view (position, rotation) has changed and buffers containing camera matrices need to be updated */
	bool viewUpdated = false;
	// Destination dimensions for resizing the window
	uint32_t destWidth;
	uint32_t destHeight;
	bool resizing = false;
	// Called if the window is resized and some resources have to be recreatesd
	void WindowResize();
	void HandleMouseMove(int32_t x, int32_t y);
protected:
	// Frame counter to display fps
	uint32_t frameCounter = 0;
	uint32_t lastFPS = 0;
	std::chrono::time_point<std::chrono::high_resolution_clock> lastTimestamp;
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
	VkCommandPool cmdPool;
	/** @brief Pipeline stages used to wait at for graphics queue submissions */
	VkPipelineStageFlags submitPipelineStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	// Contains command buffers and semaphores to be presented to the queue
	VkSubmitInfo submitInfo;
	// Command buffers used for rendering
	std::vector <std::vector<VkCommandBuffer>> allDrawCommandBuffers;
	std::vector<VkCommandBuffer> drawCommandBuffers;
	std::vector<VkCommandBuffer> drawUICommandBuffers;
	// Global render pass for frame buffer writes
	//VkRenderPass renderPass;
	// List of available frame buffers (same as number of swap chain images)
	//std::vector<render::VulkanFrameBuffer*> frameBuffers;
	render::VulkanRenderPass *mainRenderPass = nullptr;
	// Active frame buffer index
	uint32_t currentBuffer = 0;
	// Descriptor set pool
	//VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
	// Pipeline cache object
	VkPipelineCache pipelineCache;
	// Wraps the swap chain to present images (framebuffers) to the windowing system
	render::VulkanSwapChain swapChain;
	// Synchronization semaphores
	struct {
		// Swap chain image presentation
		VkSemaphore presentComplete;
		// Command buffer submission and execution
		VkSemaphore renderComplete;
	} semaphores;

	struct
	{
		bool n = false;
		bool o = false;
		bool t = false;
	} pressedkeys;

	//graphical resources
	//std::vector<Geometry *> m_geometries;
public: 
	bool prepared = false;
	uint32_t width = 1280;
	uint32_t height = 720;

	engine::scene::UIOverlay UIOverlay;

	/** @brief Last frame time measured using a high performance timer (if available) */
	float frameTimer = 1.0f;

	/** @brief Encapsulated physical and logical vulkan device */
	engine::render::VulkanDevice* vulkanDevice = nullptr;

	/** @brief Example settings that can be changed e.g. by command line arguments */
	struct Settings {
		/** @brief Activates validation layers (and message output) when set to true */
		bool validation = false;
		/** @brief Set to true if fullscreen mode has been requested via command line */
		bool fullscreen = false;
		/** @brief Set to true if v-sync will be forced for the swapchain */
		bool vsync = false;
		/** @brief Enable UI overlay */
		bool overlay = false;
	} settings;

	VkClearColorValue defaultClearColor = { { 0.25f, 0.25f, 0.25f, 1.0f } };

	float zoom = 0;

	static std::vector<const char*> args;

	// Defines a frame rate independent timer value clamped from -1.0...1.0
	// For use in animations, rotations, etc.
	float timer = 0.0f;
	// Multiplier for speeding up (or slowing down) the global timer
	float timerSpeed = 0.25f;
	
	bool paused = false;

	// Use to adjust mouse rotation speed
	float rotationSpeed = 1.0f;
	// Use to adjust mouse zoom speed
	float zoomSpeed = 1.0f;

	scene::Camera camera;

	glm::vec3 rotation = glm::vec3();
	glm::vec3 cameraPos = glm::vec3();
	glm::vec2 mousePos;

	std::string title = "Vulkan Example";
	std::string name = "vulkanExample";
	uint32_t apiVersion = VK_API_VERSION_1_2;

	class engine::render::VulkanTexture* depthStencil;

	struct {
		glm::vec2 axisLeft = glm::vec2(0.0f);
		glm::vec2 axisRight = glm::vec2(0.0f);
	} gamePadState;

	struct {
		bool left = false;
		bool right = false;
		bool middle = false;
	} mouseButtons;

	// OS specific 
#if defined(_WIN32)
	HWND window;
	HINSTANCE windowInstance;
#endif

	// Default ctor
	VulkanApplication(bool enableValidation = false);

#if defined(_WIN32)
	void SetupConsole(std::string title);
	HWND SetupWindow(HINSTANCE hinstance, WNDPROC wndproc);
	void HandleMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif
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
	virtual void CreateCommandBuffers();
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
	bool InitVulkan();

	//Prepare ui elements
	void PrepareUI();

	// Prepare commonly used Vulkan functions
	virtual void Prepare() = 0;

	// Pure virtual render function (override in derived class)
	virtual void Render();

	virtual void update(float dt) = 0;
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
	// Pure virtual function to be overriden by the dervice class
	// Called in case of an event where e.g. the framebuffer has to be rebuild and thus
	// all command buffers that may reference this
	virtual void BuildCommandBuffers();

	/** @brief (Virtual) Called after the physical device features have been read, can be used to set features to enable on the device */
	virtual void GetEnabledFeatures();
	
	// Start the main render loop
	void MainLoop();

	// Render one frame of a render loop on platforms that sync rendering
	void UpdateFrame();

	void UpdateOverlay();
	void DrawUI(const VkCommandBuffer commandBuffer);

	// Prepare the frame for workload submission
	// - Acquires the next image from the swap chain 
	// - Sets the default wait and signal semaphores
	void PrepareFrame();

	// Submit the frames' workload 
	void PresentFrame();

	/** @brief (Virtual) Called when the UI overlay is updating, can be used to add custom elements to the overlay */
	virtual void OnUpdateUIOverlay(engine::scene::UIOverlay *overlay);

	virtual ~VulkanApplication();
};

// OS specific macros for the example main entry points
#if defined(_WIN32)
// Windows entry point
#define VULKAN_EXAMPLE_MAIN()																		\
VulkanExample *vulkanExample;																		\
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)						\
{																									\
	if (vulkanExample != NULL)																		\
	{																								\
		vulkanExample->HandleMessages(hWnd, uMsg, wParam, lParam);									\
	}																								\
	return (DefWindowProc(hWnd, uMsg, wParam, lParam));												\
}																									\
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)									\
{																									\
	for (int32_t i = 0; i < __argc; i++) { VulkanExample::args.push_back(__argv[i]); };  			\
	vulkanExample = new VulkanExample();															\
	vulkanExample->SetupWindow(hInstance, WndProc);													\
	vulkanExample->InitVulkan();																	\
	vulkanExample->Prepare();																		\
	vulkanExample->MainLoop();																	\
	delete(vulkanExample);																			\
	_CrtDumpMemoryLeaks();																			\
	return 0;																						\
}																									
#endif
