#include "VulkanApplication.h"
#include "VulkanCommandBuffer.h"

VulkanApplication::VulkanApplication(bool enableValidation) : ApplicationBase(enableValidation)
{

}
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
	primaryCmdPool = vulkanDevice->GetCommandPool(vulkanDevice->queueFamilyIndices.graphicsFamily);//vulkanDevice->queueFamilyIndices.graphicsFamily);
}

void VulkanApplication::CreateAllCommandBuffers()
{
	m_drawCommandBuffers = CreateCommandBuffers();
}

std::vector<render::CommandBuffer*> VulkanApplication::CreateCommandBuffers()
{
	std::vector<render::CommandBuffer*> returnvec;
	if (vulkanDevice->queueFamilyIndices.graphicsFamily == UINT32_MAX)
		return returnvec;
	if (allvkDrawCommandBuffers.size() == 0)
		allvkDrawCommandBuffers.resize(swapChain.swapChainImageViews.size());

	//drawCommandBuffers = vulkanDevice->CreatedrawCommandBuffers(swapChain.swapChainImageViews.size(), vulkanDevice->queueFamilyIndices.graphicsFamily);
	for (int i = 0; i < swapChain.swapChainImageViews.size(); i++)
	{
		render::VulkanCommandBuffer* vulkanBuffer = (render::VulkanCommandBuffer*)(vulkanDevice->GetCommandBuffer(primaryCmdPool));
		//drawCommandBuffers.push_back(vulkanBuffer->m_vkCommandBuffer);
		returnvec.push_back(vulkanBuffer);
		allvkDrawCommandBuffers[i].push_back(vulkanBuffer->m_vkCommandBuffer);
	}
	return returnvec;
}

void VulkanApplication::DestroyCommandBuffers()
{
	//if (vulkanDevice->queueFamilyIndices.graphicsFamily != UINT32_MAX)
	//	vulkanDevice->FreeDrawCommandBuffers();

	//allDrawCommandBuffers.clear();
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
	m_mainRenderPass = mainRenderPass;
}

void VulkanApplication::CreatePipelineCache()
{
	pipelineCache = vulkanDevice->CreatePipelineCache();
}

bool VulkanApplication::InitAPI()
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
	m_device = vulkanDevice;

	device = vulkanDevice->logicalDevice;

	// Get a graphics queue from the device
	vkGetDeviceQueue(device, vulkanDevice->queueFamilyIndices.graphicsFamily, 0, &queue);
	if (vulkanDevice->queueFamilyIndices.graphicsFamily != vulkanDevice->queueFamilyIndices.presentFamily)
		vkGetDeviceQueue(device, vulkanDevice->queueFamilyIndices.presentFamily, 0, &presentationQueue);
	else
		presentationQueue = queue;
	vulkanDevice->copyQueue = queue;

	// Find a suitable depth format
	VkBool32 validDepthFormat = vulkanDevice->GetSupportedDepthFormat(&depthFormat);
	assert(validDepthFormat);

	//swapChain.connect(instance, vulkanDevice->physicalDevice, device);

	// Create synchronization objects
	VkSemaphoreCreateInfo semaphoreCreateInfo{};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	// Create a semaphore used to synchronize image presentation
	// Ensures that the image is displayed before we start submitting new commands to the queue
	//semaphores.presentComplete = vulkanDevice->GetSemaphore();
	// Create a semaphore used to synchronize command submission
	// Ensures that the image is not presented until all commands have been sumbitted and executed
	//semaphores.renderComplete = vulkanDevice->GetSemaphore();

	

	// Set up submit info structure
	// Semaphores will stay the same during application lifetime
	// Command buffer submission info is set by each example
	submitInfo = VkSubmitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pWaitDstStageMask = &submitPipelineStages;
	submitInfo.waitSemaphoreCount = 1;
	//submitInfo.pWaitSemaphores = &semaphores.presentComplete;
	submitInfo.signalSemaphoreCount = 1;
	//submitInfo.pSignalSemaphores = &semaphores.renderComplete;

	if (vulkanDevice->enableDebugMarkers) {
		engine::debugmarker::setup(device);
	}
	//initSwapchain();

	SetupSwapChain();
	CreateCommandPool();
	CreateAllCommandBuffers();
	SetupDepthStencil();
	SetupRenderPass();
	CreatePipelineCache();
	SetupFrameBuffer();

	presentCompleteSemaphores.resize(swapChain.swapChainImageViews.size());
	renderCompleteSemaphores.resize(swapChain.swapChainImageViews.size());
	for (int i = 0; i < swapChain.swapChainImageViews.size(); i++)
	{
		presentCompleteSemaphores[i] = vulkanDevice->GetSemaphore();
		renderCompleteSemaphores[i] = vulkanDevice->GetSemaphore();
	}

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
		//UIOverlay._queue = queue;
		engine::render::VulkanVertexLayout v;
		UIOverlay.LoadGeometry(engine::tools::getAssetPath() + "Roboto-Medium.ttf", &v);
		UIOverlay.preparePipeline(pipelineCache, mainRenderPass->GetRenderPass());
	}
}

void VulkanApplication::WaitForDevice()
{
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

void VulkanApplication::DrawUI(render::CommandBuffer* commandBuffer)
{
	if (settings.overlay) {
		UIOverlay.draw(commandBuffer);
	}
}

void VulkanApplication::Render()
{
	if (!prepared)
		return;

	vkWaitForFences(device, 1, &submitFences[currentBuffer], VK_TRUE, UINT64_MAX);

	//VulkanApplication::PrepareFrame();
	// Acquire the next image from the swap chain
	VkResult result = swapChain.acquireNextImage(presentCompleteSemaphores[currentBuffer], &currentBuffer);
	// Recreate the swapchain if it's no longer compatible with the surface (OUT_OF_DATE) or no longer optimal for presentation (SUBOPTIMAL)
	if ((result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR)) {
		WindowResize();
	}
	else {
		VK_CHECK_RESULT(result);
	}

	vkResetFences(device, 1, &submitFences[currentBuffer]);

	//std::vector<VkCommandBuffer> submitCommandBuffers(allDrawCommandBuffers.size());//TODO make them [cb][i]
	//for (int i = 0; i < allDrawCommandBuffers.size(); i++)
	//{
	//	submitCommandBuffers[i] = allDrawCommandBuffers[i][currentBuffer];
	//}

	submitInfo.pWaitSemaphores = &presentCompleteSemaphores[currentBuffer];
	submitInfo.pSignalSemaphores = &renderCompleteSemaphores[currentBuffer];
	submitInfo.commandBufferCount = allvkDrawCommandBuffers[currentBuffer].size();
	submitInfo.pCommandBuffers = allvkDrawCommandBuffers[currentBuffer].data();
	VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, submitFences[currentBuffer]));

	//VulkanApplication::PresentFrame();
	result = swapChain.queuePresent(presentationQueue, currentBuffer, renderCompleteSemaphores[currentBuffer]);
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
	currentBuffer = (currentBuffer + 1) % swapChain.swapChainImageViews.size();
}

void VulkanApplication::ViewChanged() {}

void VulkanApplication::KeyPressed(uint32_t) {}

void VulkanApplication::MouseMoved(double x, double y, bool & handled) {}

void VulkanApplication::BuildCommandBuffers() {}

void VulkanApplication::SubmitOnQueue(class render::CommandBuffer* commandBuffer)
{
	//TODO make me
}


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
	//vkDeviceWaitIdle(device);
	WaitForDevice();

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

	//vkDeviceWaitIdle(device);
	WaitForDevice();

	if ((width > 0.0f) && (height > 0.0f)) {
		camera.UpdateAspectRatio((float)width / (float)height);
	}

	prepared = true;
}

void VulkanApplication::WindowResized()
{
	// Can be overriden in derived class
}

void VulkanApplication::OnUpdateUIOverlay(engine::scene::UIOverlay *overlay) {}

void VulkanApplication::DrawFullScreenQuad(render::CommandBuffer* commandBuffer)
{
	VkCommandBuffer vkbuffer = ((render::VulkanCommandBuffer*)commandBuffer)->m_vkCommandBuffer;
	vkCmdDraw(vkbuffer, 3, 1, 0, 0);
}

const std::string VulkanApplication::GetShadersPath()
{
	return "shaders/";
}

const std::string VulkanApplication::GetVertexShadersExt()
{
	return ".vert.spv";
}

const std::string VulkanApplication::GetFragShadersExt()
{
	return ".frag.spv";
}

VulkanApplication::~VulkanApplication()
{
	// Clean up Vulkan resources
	swapChain.CleanUp();
	DestroyCommandBuffers();
	vulkanDevice->DestroyPipelineCache();
	for(int i=0;i<presentCompleteSemaphores.size();i++)
	vulkanDevice->DestroySemaphore(presentCompleteSemaphores[i]);
	for (int i = 0; i < renderCompleteSemaphores.size(); i++)
	vulkanDevice->DestroySemaphore(renderCompleteSemaphores[i]);

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