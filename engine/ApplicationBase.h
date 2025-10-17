#pragma once

#ifdef _WIN32
#pragma comment(linker, "/subsystem:windows")
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#include <ShellScalingAPI.h>
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

#include "render/GraphicsDevice.h"
#include "scene/Camera.h"

using namespace engine;

class ApplicationBase
{
protected:
	/** brief Indicates that the view (position, rotation) has changed and buffers containing camera matrices need to be updated */
	bool viewUpdated = false;
	// Destination dimensions for resizing the window
	uint32_t destWidth;
	uint32_t destHeight;
	bool resizing = false;
	// Called if the window is resized and some resources have to be recreatesd
	// 
	// Frame counter to display fps
	uint32_t frameCounter = 0;
	uint32_t lastFPS = 0;
	std::chrono::time_point<std::chrono::high_resolution_clock> lastTimestamp;

	render::GraphicsDevice* m_device = nullptr;
	render::CommandPool* primaryCmdPool;
	std::vector<render::CommandBuffer*> m_drawCommandBuffers;
	std::vector<std::vector<render::CommandBuffer*>> m_allDrawCommandBuffers;
	render::CommandBuffer* m_loadingCommandBuffer = nullptr;
	render::RenderPass* m_mainRenderPass = nullptr;

	struct
	{
		bool n = false;
		bool o = false;
		bool t = false;
	} pressedkeys;

	//graphical resources
	//std::vector<Geometry *> m_geometries;
	// Active frame buffer index
	uint32_t currentBuffer = 0;
public: 
	bool prepared = false;
	uint32_t width = 1280;
	uint32_t height = 720;

	//engine::scene::UIOverlay UIOverlay;

	/** @brief Last frame time measured using a high performance timer (if available) */
	float frameTimer = 1.0f;

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

	float zoom = 0;

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
	ApplicationBase(bool enableValidation = false);

	virtual bool InitAPI() = 0;

#if defined(_WIN32)
	void SetupConsole(std::string title);
	HWND SetupWindow(HINSTANCE hinstance, WNDPROC wndproc);
	void HandleMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif
	virtual void WindowResize() = 0;

	// Prepare commonly used Vulkan functions
	virtual void Prepare() = 0;

	// Pure virtual render function (override in derived class)
	virtual void Render() = 0;

	virtual void update(float dt) = 0;
	// Called when view change occurs
	// Can be overriden in derived class to e.g. update uniform buffers 
	// Containing view dependant matrices
	virtual void ViewChanged() = 0;
	/** @brief (Virtual) Called after a key was pressed, can be used to do custom key handling */
	virtual void KeyPressed(uint32_t) = 0;
	virtual void HandleMouseMove(int32_t x, int32_t y);
	/** @brief (Virtual) Called after th mouse cursor moved and before internal events (like camera rotation) is handled */
	virtual void MouseMoved(double x, double y, bool &handled) = 0;
	// Called when the window has been resized
	// Can be overriden in derived class to recreate or rebuild resources attached to the frame buffer / swapchain
	virtual void WindowResized() = 0;

	virtual std::vector<render::CommandBuffer*> CreateCommandBuffers() = 0;
	virtual void CreateAllCommandBuffers() = 0;
	// Pure virtual function to be overriden by the dervice class
	// Called in case of an event where e.g. the framebuffer has to be rebuild and thus
	// all command buffers that may reference this
	virtual void BuildCommandBuffers() = 0;

	virtual void SubmitOnQueue(class render::CommandBuffer* commandBuffer) = 0;

	/** @brief (Virtual) Called after the physical device features have been read, can be used to set features to enable on the device */
	virtual void GetEnabledFeatures() = 0;
	
	// Start the main render loop
	void MainLoop();

	// Render one frame of a render loop on platforms that sync rendering
	virtual void UpdateFrame();

	virtual void UpdateOverlay()=0;

	virtual void WaitForDevice() = 0;

	virtual void DrawFullScreenQuad(render::CommandBuffer* commandBuffer) = 0;
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
	vulkanExample = new VulkanExample();															\
	vulkanExample->SetupWindow(hInstance, WndProc);													\
	vulkanExample->InitAPI();																	\
	vulkanExample->Prepare();																		\
	vulkanExample->MainLoop();																	\
	delete(vulkanExample);																			\
	_CrtDumpMemoryLeaks();																			\
	return 0;																						\
}																									
#endif
