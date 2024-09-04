/*
* Vulkan Example base class
*
* Copyright (C) by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#include "VulkanApplication.h"

#if defined(_WIN32)
#define KEY_ESCAPE VK_ESCAPE 
#define KEY_F1 VK_F1
#define KEY_F2 VK_F2
#define KEY_F3 VK_F3
#define KEY_F4 VK_F4
#define KEY_F5 VK_F5
#define KEY_W 0x57
#define KEY_A 0x41
#define KEY_S 0x53
#define KEY_D 0x44
#define KEY_P 0x50
#define KEY_SPACE 0x20
#define KEY_KPADD 0x6B
#define KEY_KPSUB 0x6D
#define KEY_B 0x42
#define KEY_F 0x46
#define KEY_L 0x4C
#define KEY_N 0x4E
#define KEY_O 0x4F
#define KEY_T 0x54
#endif

std::vector<const char*> VulkanApplication::args;



VulkanApplication::VulkanApplication(bool enableValidation)
{
#if !defined(VK_USE_PLATFORM_ANDROID_KHR)
	// Check for a valid asset path
	struct stat info;
	if (stat(engine::tools::getAssetPath().c_str(), &info) != 0)
	{
#if defined(_WIN32)
		std::string msg = "Could not locate asset path in \"" + engine::tools::getAssetPath() + "\" !";
		MessageBox(NULL, msg.c_str(), "Fatal error", MB_OK | MB_ICONERROR);
#else
		std::cerr << "Error: Could not find asset path in " << getAssetPath() << std::endl;
#endif
		exit(-1);
	}
#endif

	settings.validation = enableValidation;

	char* numConvPtr;

	// Parse command line arguments
	for (size_t i = 0; i < args.size(); i++)
	{
		if (args[i] == std::string("-validation")) {
			settings.validation = true;
		}
		if (args[i] == std::string("-vsync")) {
			settings.vsync = true;
		}
		if ((args[i] == std::string("-f")) || (args[i] == std::string("--fullscreen"))) {
			settings.fullscreen = true;
		}
		if ((args[i] == std::string("-w")) || (args[i] == std::string("-width"))) {
			uint32_t w = strtol(args[i + 1], &numConvPtr, 10);
			if (numConvPtr != args[i + 1]) { width = w; };
		}
		if ((args[i] == std::string("-h")) || (args[i] == std::string("-height"))) {
			uint32_t h = strtol(args[i + 1], &numConvPtr, 10);
			if (numConvPtr != args[i + 1]) { height = h; };
		}
	}

#if defined(_WIN32)
	// Enable console if validation is active
	// Debug message callback will output to it
	if (this->settings.validation)
	{
		SetupConsole("Vulkan validation output");
	}
#endif
}

#if defined(_WIN32)
// Win32 : Sets up a console window and redirects standard output to it
void VulkanApplication::SetupConsole(std::string title)
{
	AllocConsole();
	AttachConsole(GetCurrentProcessId());
	FILE* stream;
	freopen_s(&stream, "CONOUT$", "w+", stdout);
	freopen_s(&stream, "CONOUT$", "w+", stderr);
	SetConsoleTitle(TEXT(title.c_str()));
}

HWND VulkanApplication::SetupWindow(HINSTANCE hinstance, WNDPROC wndproc)
{
	this->windowInstance = hinstance;

	WNDCLASSEX wndClass;

	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.style = CS_HREDRAW | CS_VREDRAW;
	wndClass.lpfnWndProc = wndproc;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hInstance = hinstance;
	wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wndClass.lpszMenuName = NULL;
	wndClass.lpszClassName = name.c_str();
	wndClass.hIconSm = LoadIcon(NULL, IDI_WINLOGO);

	if (!RegisterClassEx(&wndClass))
	{
		std::cout << "Could not register window class!\n";
		fflush(stdout);
		exit(1);
	}

	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);

	if (settings.fullscreen)
	{
		DEVMODE dmScreenSettings;
		memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
		dmScreenSettings.dmSize = sizeof(dmScreenSettings);
		dmScreenSettings.dmPelsWidth = screenWidth;
		dmScreenSettings.dmPelsHeight = screenHeight;
		dmScreenSettings.dmBitsPerPel = 32;
		dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

		if ((width != (uint32_t)screenWidth) && (height != (uint32_t)screenHeight))
		{
			if (ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
			{
				if (MessageBox(NULL, "Fullscreen Mode not supported!\n Switch to window mode?", "Error", MB_YESNO | MB_ICONEXCLAMATION) == IDYES)
				{
					settings.fullscreen = false;
				}
				else
				{
					return nullptr;
				}
			}
		}

	}

	DWORD dwExStyle;
	DWORD dwStyle;

	if (settings.fullscreen)
	{
		dwExStyle = WS_EX_APPWINDOW;
		dwStyle = WS_POPUP | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
	}
	else
	{
		dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
		dwStyle = WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
	}

	RECT windowRect;
	windowRect.left = 0L;
	windowRect.top = 0L;
	windowRect.right = settings.fullscreen ? (long)screenWidth : (long)width;
	windowRect.bottom = settings.fullscreen ? (long)screenHeight : (long)height;

	AdjustWindowRectEx(&windowRect, dwStyle, FALSE, dwExStyle);

	std::string windowTitle = title;//getWindowTitle();
	window = CreateWindowEx(0,
		name.c_str(),
		windowTitle.c_str(),
		dwStyle | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
		0,
		0,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		NULL,
		NULL,
		hinstance,
		NULL);

	if (!settings.fullscreen)
	{
		// Center on screen
		uint32_t x = (GetSystemMetrics(SM_CXSCREEN) - windowRect.right) / 2;
		uint32_t y = (GetSystemMetrics(SM_CYSCREEN) - windowRect.bottom) / 2;
		SetWindowPos(window, 0, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
	}

	if (!window)
	{
		printf("Could not create window!\n");
		fflush(stdout);
		return nullptr;
		exit(1);
	}

	ShowWindow(window, SW_SHOW);
	SetForegroundWindow(window);
	SetFocus(window);

	return window;
}

void VulkanApplication::HandleMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CLOSE:
		prepared = false;
		DestroyWindow(hWnd);
		PostQuitMessage(0);
		break;
	case WM_PAINT:
		ValidateRect(window, NULL);
		break;
	case WM_KEYDOWN:
		switch (wParam)
		{
		case KEY_P:
			paused = !paused;
			break;
		case KEY_F1:
			if (settings.overlay) {
				UIOverlay.m_visible = !UIOverlay.m_visible;
			}
			break;
		case KEY_N:
			pressedkeys.n = true; break;
		case KEY_O:
			pressedkeys.o = true; break;
		case KEY_T:
			pressedkeys.t = true; break;
		case KEY_ESCAPE:
			PostQuitMessage(0);
			break;
		}


		switch (wParam)
		{
		case KEY_W:
			camera.keys.up = true;
			break;
		case KEY_S:
			camera.keys.down = true;
			break;
		case KEY_A:
			camera.keys.left = true;
			break;
		case KEY_D:
			camera.keys.right = true;
			break;
		}


		KeyPressed((uint32_t)wParam);
		break;
	case WM_KEYUP:

		switch (wParam)
		{
		case KEY_W:
			camera.keys.up = false;
			break;
		case KEY_S:
			camera.keys.down = false;
			break;
		case KEY_A:
			camera.keys.left = false;
			break;
		case KEY_D:
			camera.keys.right = false;
			break;
		}

		switch (wParam)
		{
		case KEY_N:
			pressedkeys.n = false; break;
		case KEY_O:
			pressedkeys.o = false; break;
		case KEY_T:
			pressedkeys.t = false; break;
		}
		break;
	case WM_LBUTTONDOWN:
		mousePos = glm::vec2((float)LOWORD(lParam), (float)HIWORD(lParam));
		mouseButtons.left = true;
		break;
	case WM_RBUTTONDOWN:
		mousePos = glm::vec2((float)LOWORD(lParam), (float)HIWORD(lParam));
		mouseButtons.right = true;
		break;
	case WM_MBUTTONDOWN:
		mousePos = glm::vec2((float)LOWORD(lParam), (float)HIWORD(lParam));
		mouseButtons.middle = true;
		break;
	case WM_LBUTTONUP:
		mouseButtons.left = false;
		break;
	case WM_RBUTTONUP:
		mouseButtons.right = false;
		break;
	case WM_MBUTTONUP:
		mouseButtons.middle = false;
		break;
	case WM_MOUSEWHEEL:
	{
		short wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
		zoom += (float)wheelDelta * 0.005f * zoomSpeed;
		if (camera.type == scene::Camera::surface)
			camera.TranslateOnSphere(glm::vec3((float)wheelDelta * 0.0001f * camera.movementSpeed, 0.0f, 0.0f));
		else
			camera.Translate(glm::vec3(0.0f, 0.0f, (float)wheelDelta * 0.005f * zoomSpeed));
		viewUpdated = true;
		break;
	}
	case WM_MOUSEMOVE:
	{
		HandleMouseMove(LOWORD(lParam), HIWORD(lParam));
		break;
	}
	case WM_SIZE:
		if ((prepared) && (wParam != SIZE_MINIMIZED))
		{
			if ((resizing) || ((wParam == SIZE_MAXIMIZED) || (wParam == SIZE_RESTORED)))
			{
				destWidth = LOWORD(lParam);
				destHeight = HIWORD(lParam);
				WindowResize();
			}
		}
		break;
	case WM_ENTERSIZEMOVE:
		resizing = true;
		break;
	case WM_EXITSIZEMOVE:
		resizing = false;
		break;
	}
}
#endif

VkResult VulkanApplication::CreateInstance(bool enableValidation)
{
	this->settings.validation = enableValidation;

	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = name.c_str();
	appInfo.pEngineName = name.c_str();
	appInfo.apiVersion = apiVersion;

	std::vector<const char*> instanceExtensions = { VK_KHR_SURFACE_EXTENSION_NAME };

	// Enable surface extensions depending on os
#if defined(_WIN32)
	instanceExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif

	if (enabledInstanceExtensions.size() > 0) {
		for (auto enabledExtension : enabledInstanceExtensions) {
			instanceExtensions.push_back(enabledExtension);
		}
	}

	VkInstanceCreateInfo instanceCreateInfo = {};
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pNext = NULL;
	instanceCreateInfo.pApplicationInfo = &appInfo;
	if (instanceExtensions.size() > 0)
	{
		if (settings.validation)
		{
			instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}
		instanceCreateInfo.enabledExtensionCount = (uint32_t)instanceExtensions.size();
		instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();
	}
	if (settings.validation)
	{
		// The VK_LAYER_KHRONOS_validation contains all current validation functionality.
		// Note that on Android this layer requires at least NDK r20 
		const char* validationLayerName = "VK_LAYER_KHRONOS_validation";
		// Check if this layer is available at instance level
		uint32_t instanceLayerCount;
		vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);
		std::vector<VkLayerProperties> instanceLayerProperties(instanceLayerCount);
		vkEnumerateInstanceLayerProperties(&instanceLayerCount, instanceLayerProperties.data());
		bool validationLayerPresent = false;
		for (VkLayerProperties layer : instanceLayerProperties) {
			if (strcmp(layer.layerName, validationLayerName) == 0) {
				validationLayerPresent = true;
				break;
			}
		}
		if (validationLayerPresent) {
			instanceCreateInfo.ppEnabledLayerNames = &validationLayerName;
			instanceCreateInfo.enabledLayerCount = 1;
		}
		else {
			std::cerr << "Validation layer VK_LAYER_KHRONOS_validation not present, validation is disabled";
		}
	}
	return vkCreateInstance(&instanceCreateInfo, nullptr, &instance);
}

void VulkanApplication::SetupSwapChain()
{
	swapChain.Create(vulkanDevice->physicalDevice, vulkanDevice->logicalDevice, &width, &height, vulkanDevice->queueFamilyIndices.graphicsFamily, vulkanDevice->queueFamilyIndices.presentFamily, settings.vsync);
}

void VulkanApplication::CreateCommandPool()
{
	cmdPool = vulkanDevice->GetCommandPool(vulkanDevice->queueFamilyIndices.graphicsFamily);//vulkanDevice->queueFamilyIndices.graphicsFamily);
}

void VulkanApplication::CreateCommandBuffers()
{
	if (vulkanDevice->queueFamilyIndices.graphicsFamily == UINT32_MAX)
		return;
	// Create one command buffer for each swap chain image and reuse for rendering
	drawCommandBuffers = vulkanDevice->CreatedrawCommandBuffers(swapChain.swapChainImageViews.size(), vulkanDevice->queueFamilyIndices.graphicsFamily);
	allDrawCommandBuffers.push_back(drawCommandBuffers);
}

void VulkanApplication::DestroyCommandBuffers()
{
	if (vulkanDevice->queueFamilyIndices.graphicsFamily != UINT32_MAX)
		vulkanDevice->FreeDrawCommandBuffers();

	allDrawCommandBuffers.clear();
}

void VulkanApplication::SetupDepthStencil()
{
	VkImageAspectFlags flags = VK_IMAGE_ASPECT_DEPTH_BIT;
	if (depthFormat >= VK_FORMAT_D16_UNORM_S8_UINT) {
		flags |= VK_IMAGE_ASPECT_STENCIL_BIT;
	}
	depthStencil = vulkanDevice->GetDepthRenderTarget(width, height, false, flags);
}

void VulkanApplication::SetupFrameBuffer()
{
	for (uint32_t i = 0; i < swapChain.swapChainImageViews.size(); i++)
	{
		render::VulkanFrameBuffer* fb = vulkanDevice->GetFrameBuffer(mainRenderPass->GetRenderPass(), width, height, { swapChain.swapChainImageViews[i], depthStencil->m_descriptor.imageView }, defaultClearColor);
		mainRenderPass->AddFrameBuffer(fb);
	}
}

void VulkanApplication::SetupRenderPass()
{
	mainRenderPass = vulkanDevice->GetRenderPass({ {swapChain.m_surfaceFormat.format, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR}, {depthFormat, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL} });
}

void VulkanApplication::CreatePipelineCache()
{
	pipelineCache = vulkanDevice->CreatePipelineCache();
}

bool VulkanApplication::InitVulkan()
{
	VkResult err;

	// Vulkan instance
	err = CreateInstance(settings.validation);
	if (err) {
		engine::tools::exitFatal("Could not create Vulkan instance : \n" + engine::tools::errorString(err), err);
		return false;
	}

	// If requested, we enable the default validation layers for debugging
	if (settings.validation)
	{
		// The report flags determine what type of messages for the layers will be displayed
		// For validating (debugging) an appplication the error and warning bits should suffice
		VkDebugReportFlagsEXT debugReportFlags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
		// Additional flags include performance info, loader and layer debug messages, etc.
		engine::debug::setupDebugging(instance, debugReportFlags, VK_NULL_HANDLE);
	}

	//initSwapchain();
	swapChain.InitSurface(instance, windowInstance, window);

	// Derived examples can override this to set actual features (based on above readings) to enable for logical device creation
	GetEnabledFeatures();

	// Vulkan device creation
	// This is handled by a separate class that gets a logical device representation
	// and encapsulates functions related to a device
	enabledDeviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	vulkanDevice = new engine::render::VulkanDevice(instance, &enabledFeatures, enabledDeviceExtensions, swapChain.surface, deviceCreatepNextChain);

	device = vulkanDevice->logicalDevice;

	// Get a graphics queue from the device
	vkGetDeviceQueue(device, vulkanDevice->queueFamilyIndices.graphicsFamily, 0, &queue);
	if (vulkanDevice->queueFamilyIndices.graphicsFamily != vulkanDevice->queueFamilyIndices.presentFamily)
		vkGetDeviceQueue(device, vulkanDevice->queueFamilyIndices.presentFamily, 0, &presentationQueue);
	else
		presentationQueue = queue;

	// Find a suitable depth format
	VkBool32 validDepthFormat = vulkanDevice->GetSupportedDepthFormat(&depthFormat);
	assert(validDepthFormat);

	//swapChain.connect(instance, vulkanDevice->physicalDevice, device);

	// Create synchronization objects
	VkSemaphoreCreateInfo semaphoreCreateInfo{};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	// Create a semaphore used to synchronize image presentation
	// Ensures that the image is displayed before we start submitting new commands to the queue
	semaphores.presentComplete = vulkanDevice->GetSemaphore();
	// Create a semaphore used to synchronize command submission
	// Ensures that the image is not presented until all commands have been sumbitted and executed
	semaphores.renderComplete = vulkanDevice->GetSemaphore();

	// Set up submit info structure
	// Semaphores will stay the same during application lifetime
	// Command buffer submission info is set by each example
	submitInfo = VkSubmitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pWaitDstStageMask = &submitPipelineStages;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &semaphores.presentComplete;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &semaphores.renderComplete;

	if (vulkanDevice->enableDebugMarkers) {
		engine::debugmarker::setup(device);
	}
	//initSwapchain();

	SetupSwapChain();
	CreateCommandPool();
	CreateCommandBuffers();
	SetupDepthStencil();
	SetupRenderPass();
	CreatePipelineCache();
	SetupFrameBuffer();

	submitFences.resize(swapChain.swapChainImageViews.size());
	for (int i = 0; i < submitFences.size(); i++)
	{
		submitFences[i] = vulkanDevice->GetSignaledFence();
	}

	return true;
}

void VulkanApplication::PrepareUI()
{
	settings.overlay = settings.overlay;
	if (settings.overlay) {
		UIOverlay._device = vulkanDevice;
		UIOverlay._queue = queue;
		UIOverlay.prepareResources();
		UIOverlay.preparePipeline(pipelineCache, mainRenderPass->GetRenderPass());
	}
}

void VulkanApplication::UpdateFrame()
{
	auto tStart = std::chrono::high_resolution_clock::now();
	if (viewUpdated)
	{
		viewUpdated = false;
		ViewChanged();
	}

	Render();
	frameCounter++;
	auto tEnd = std::chrono::high_resolution_clock::now();
	auto tDiff = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
	frameTimer = (float)tDiff / 1000.0f;
	camera.Update(frameTimer);

	if (camera.moving())
	{
		viewUpdated = true;
	}
	// Convert to clamped timer value
	if (!paused)
	{
		timer += timerSpeed * frameTimer;
		if (timer > 1.0)
		{
			timer -= 1.0f;
		}
		update(frameTimer);
	}
	float fpsTimer = (float)(std::chrono::duration<double, std::milli>(tEnd - lastTimestamp).count());
	if (fpsTimer > 1000.0f)
	{
		lastFPS = static_cast<uint32_t>((float)frameCounter * (1000.0f / fpsTimer));
		frameCounter = 0;
		lastTimestamp = tEnd;
	}
	// TODO: Cap UI overlay update rates
	UpdateOverlay();
}

void VulkanApplication::MainLoop()
{
	prepared = true;
	destWidth = width;
	destHeight = height;
	lastTimestamp = std::chrono::high_resolution_clock::now();
#if defined(_WIN32)
	MSG msg;
	bool quitMessageReceived = false;
	while (!quitMessageReceived) {
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_QUIT) {
				quitMessageReceived = true;
				break;
			}
		}
		if (!IsIconic(window)) {
			UpdateFrame();
		}
	}
#endif
	// Flush device to make sure all resources can be freed
	if (device != VK_NULL_HANDLE) {
		vkDeviceWaitIdle(device);
	}
}

void VulkanApplication::UpdateOverlay()
{
	if (!settings.overlay)
		return;

	ImGuiIO& io = ImGui::GetIO();

	io.DisplaySize = ImVec2((float)width, (float)height);
	io.DeltaTime = frameTimer;

	io.MousePos = ImVec2(mousePos.x, mousePos.y);
	io.MouseDown[0] = mouseButtons.left;
	io.MouseDown[1] = mouseButtons.right;

	ImGui::NewFrame();

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
	ImGui::SetNextWindowPos(ImVec2(10, 10));
	ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiCond_FirstUseEver);
	ImGui::Begin("Vulkan Example", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
	ImGui::TextUnformatted(title.c_str());
	ImGui::TextUnformatted(vulkanDevice->GetDeviceName());
	ImGui::Text("%.2f ms/frame (%.1d fps)", (1000.0f / lastFPS), lastFPS);

	ImGui::PushItemWidth(110.0f * UIOverlay.m_scale);
	OnUpdateUIOverlay(&UIOverlay);
	ImGui::PopItemWidth();

	ImGui::End();
	ImGui::PopStyleVar();
	ImGui::Render();

	if(UIOverlay.shouldRecreateBuffers())
		vkWaitForFences(device, submitFences.size(), submitFences.data(), VK_TRUE, UINT64_MAX);

	if (UIOverlay.update() || UIOverlay.m_updated) {		
		BuildCommandBuffers();
		UIOverlay.m_updated = false;
	}
}

void VulkanApplication::DrawUI(const VkCommandBuffer commandBuffer)
{
	if (settings.overlay) {
		const VkViewport viewport = { 0, 0, (float)width, (float)height, 0.0f, 1.0f };
		const VkRect2D scissor = { VkOffset2D{0,0}, VkExtent2D{width, height} };
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		UIOverlay.draw(commandBuffer);
	}
}

void VulkanApplication::PrepareFrame()
{
	// Acquire the next image from the swap chain
	VkResult result = swapChain.acquireNextImage(semaphores.presentComplete, &currentBuffer);
	// Recreate the swapchain if it's no longer compatible with the surface (OUT_OF_DATE) or no longer optimal for presentation (SUBOPTIMAL)
	if ((result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR)) {
		WindowResize();
	}
	else {
		VK_CHECK_RESULT(result);
	}
}

void VulkanApplication::PresentFrame()
{
	VkResult result = swapChain.queuePresent(presentationQueue, currentBuffer, semaphores.renderComplete);
	if (!((result == VK_SUCCESS) || (result == VK_SUBOPTIMAL_KHR))) {
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			// Swap chain is no longer compatible with the surface and needs to be recreated
			WindowResize();
			return;
		}
		else {
			VK_CHECK_RESULT(result);
		}
	}
	//VK_CHECK_RESULT(vkQueueWaitIdle(presentationQueue));
}

void VulkanApplication::Render()
{
	if (!prepared)
		return;

	vkWaitForFences(device, 1, &submitFences[currentBuffer], VK_TRUE, UINT64_MAX);

	VulkanApplication::PrepareFrame();

	vkResetFences(device, 1, &submitFences[currentBuffer]);

	std::vector<VkCommandBuffer> submitCommandBuffers(allDrawCommandBuffers.size());
	for (int i = 0; i < allDrawCommandBuffers.size(); i++)
	{
		submitCommandBuffers[i] = allDrawCommandBuffers[i][currentBuffer];
	}

	submitInfo.commandBufferCount = submitCommandBuffers.size();
	submitInfo.pCommandBuffers = submitCommandBuffers.data();
	VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, submitFences[currentBuffer]));

	VulkanApplication::PresentFrame();
}

void VulkanApplication::ViewChanged() {}

void VulkanApplication::KeyPressed(uint32_t) {}

void VulkanApplication::MouseMoved(double x, double y, bool & handled) {}

void VulkanApplication::BuildCommandBuffers() {}


void VulkanApplication::GetEnabledFeatures()
{
	// Can be overriden in derived class
}

void VulkanApplication::WindowResize()
{
	if (!prepared)
	{
		return;
	}
	prepared = false;

	// Ensure all operations on the device have been finished before destroying resources
	vkDeviceWaitIdle(device);

	// Recreate swap chain
	width = destWidth;
	height = destHeight;
	SetupSwapChain();

	// Recreate the frame buffers
	vulkanDevice->DestroyTexture(depthStencil);
	SetupDepthStencil();
	mainRenderPass->ResetFrameBuffers();
	SetupFrameBuffer();

	if ((width > 0.0f) && (height > 0.0f)) {
		if (settings.overlay) {
			UIOverlay.resize(width, height);
		}
	}

	// Notify derived class
	WindowResized();
	BuildCommandBuffers();
	
	ViewChanged();

	vkDeviceWaitIdle(device);

	if ((width > 0.0f) && (height > 0.0f)) {
		camera.UpdateAspectRatio((float)width / (float)height);
	}

	prepared = true;
}

void VulkanApplication::HandleMouseMove(int32_t x, int32_t y)
{
	int32_t dx = (int32_t)mousePos.x - x;
	int32_t dy = (int32_t)mousePos.y - y;

	bool handled = false;

	if (settings.overlay) {
		ImGuiIO& io = ImGui::GetIO();
		handled = io.WantCaptureMouse;
	}
	MouseMoved((float)x, (float)y, handled);

	if (handled) {
		mousePos = glm::vec2((float)x, (float)y);
		return;
	}

	if (mouseButtons.left) {
		rotation.x += dy * 1.25f * rotationSpeed;
		rotation.y -= dx * 1.25f * rotationSpeed;
		//camera.Rotate(glm::vec3(dy * camera.rotationSpeed, -dx * camera.rotationSpeed, 0.0f));
		camera.Rotate(glm::vec3(dy , -dx , 0.0f));
		viewUpdated = true;
	}
	if (mouseButtons.right) {
		zoom += dy * .005f * zoomSpeed;
		camera.Translate(glm::vec3(-0.0f, 0.0f, dy * .005f * zoomSpeed));
		viewUpdated = true;
	}
	if (mouseButtons.middle) {
		cameraPos.x -= dx * 0.01f;
		cameraPos.y -= dy * 0.01f;
		camera.Translate(glm::vec3(-dx * 0.01f, -dy * 0.01f, 0.0f));
		viewUpdated = true;
	}
	mousePos = glm::vec2((float)x, (float)y);
}

void VulkanApplication::WindowResized()
{
	// Can be overriden in derived class
}

void VulkanApplication::OnUpdateUIOverlay(engine::scene::UIOverlay *overlay) {}

VulkanApplication::~VulkanApplication()
{
	// Clean up Vulkan resources
	swapChain.CleanUp();
	DestroyCommandBuffers();
	vulkanDevice->DestroyPipelineCache();
	vulkanDevice->DestroySemaphore(semaphores.presentComplete);
	vulkanDevice->DestroySemaphore(semaphores.renderComplete);

	if (settings.overlay) {
		UIOverlay.freeResources();
	}

	delete vulkanDevice;

	if (settings.validation)
	{
		engine::debug::freeDebugCallback(instance);
	}

	vkDestroyInstance(instance, nullptr);
}