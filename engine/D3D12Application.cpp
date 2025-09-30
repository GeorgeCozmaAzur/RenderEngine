#include "D3D12Application.h"
#include "D3D12CommandBuffer.h"
#include "D3D12RenderPass.h"
#include "render/directx/d3dx12.h"
#include "D3D12Device.h"

using Microsoft::WRL::ComPtr;
using namespace engine::render;

D3D12Application::D3D12Application(bool enableValidation) : ApplicationBase(enableValidation)
{

}

inline std::string HrToString(HRESULT hr)
{
	char s_str[64] = {};
	sprintf_s(s_str, "HRESULT of 0x%08X", static_cast<UINT>(hr));
	return std::string(s_str);
}
class HrException : public std::runtime_error
{
public:
	HrException(HRESULT hr) : std::runtime_error(HrToString(hr)), m_hr(hr) {}
	HRESULT Error() const { return m_hr; }
private:
	const HRESULT m_hr;
};
inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		throw HrException(hr);
	}
}
void GetHardwareAdapter(
	IDXGIFactory1* pFactory,
	IDXGIAdapter1** ppAdapter,
	bool requestHighPerformanceAdapter = false)
{
	*ppAdapter = nullptr;

	ComPtr<IDXGIAdapter1> adapter;

	ComPtr<IDXGIFactory6> factory6;
	if (SUCCEEDED(pFactory->QueryInterface(IID_PPV_ARGS(&factory6))))
	{
		for (
			UINT adapterIndex = 0;
			SUCCEEDED(factory6->EnumAdapterByGpuPreference(
				adapterIndex,
				requestHighPerformanceAdapter == true ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED,
				IID_PPV_ARGS(&adapter)));
				++adapterIndex)
		{
			DXGI_ADAPTER_DESC1 desc;
			adapter->GetDesc1(&desc);

			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{
				// Don't select the Basic Render Driver adapter.
				// If you want a software adapter, pass in "/warp" on the command line.
				continue;
			}

			// Check to see whether the adapter supports Direct3D 12, but don't create the
			// actual device yet.
			if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
			{
				break;
			}
		}
	}

	if (adapter.Get() == nullptr)
	{
		for (UINT adapterIndex = 0; SUCCEEDED(pFactory->EnumAdapters1(adapterIndex, &adapter)); ++adapterIndex)
		{
			DXGI_ADAPTER_DESC1 desc;
			adapter->GetDesc1(&desc);

			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{
				// Don't select the Basic Render Driver adapter.
				// If you want a software adapter, pass in "/warp" on the command line.
				continue;
			}

			// Check to see whether the adapter supports Direct3D 12, but don't create the
			// actual device yet.
			if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
			{
				break;
			}
		}
	}

	*ppAdapter = adapter.Detach();
}

bool D3D12Application::InitAPI()
{
	UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
	// Enable the debug layer (requires the Graphics Tools "optional feature").
	// NOTE: Enabling the debug layer after device creation will invalidate the active device.
	if(settings.validation)
	{
		ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();

			// Enable additional debug layers.
			dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}
	}
#endif
	ComPtr<IDXGIFactory4> factory;
	ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

	ComPtr<IDXGIAdapter1> hardwareAdapter;
	GetHardwareAdapter(factory.Get(), &hardwareAdapter);

	ThrowIfFailed(D3D12CreateDevice(
		hardwareAdapter.Get(),
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&m_d3ddevice)
	));

	m_device = new render::D3D12Device(m_d3ddevice);

	// Describe and create the command queue.
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	ThrowIfFailed(m_d3ddevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

	// Describe and create the swap chain.
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = FrameCount;
	swapChainDesc.Width = width;
	swapChainDesc.Height = height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	ComPtr<IDXGISwapChain1> swapChain;
	ThrowIfFailed(factory->CreateSwapChainForHwnd(
		m_commandQueue.Get(),        // Swap chain needs the queue so that it can force a flush on it.
		window,
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain
	));

	// This sample does not support fullscreen transitions.
	ThrowIfFailed(factory->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER));

	ThrowIfFailed(swapChain.As(&m_swapChain));
	currentBuffer = m_swapChain->GetCurrentBackBufferIndex();

	// Create descriptor heaps.
	{
		// Describe and create a render target view (RTV) descriptor heap.
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = FrameCount;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(m_d3ddevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

		m_rtvDescriptorSize = m_d3ddevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		// Describe and create a depth stencil view (DSV) descriptor heap.
		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
		dsvHeapDesc.NumDescriptors = 1;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(m_d3ddevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)));
	}

	std::vector<CD3DX12_CPU_DESCRIPTOR_HANDLE> frameBuffersHandles;
	// Create frame resources.
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

		// Create a RTV for each frame.
		for (UINT n = 0; n < FrameCount; n++)
		{
			ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])));
			m_d3ddevice->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
			frameBuffersHandles.push_back(rtvHandle);
			rtvHandle.Offset(1, m_rtvDescriptorSize);
		}
	}
	// Create the depth stencil view.
	{
		D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
		depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
		depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

		D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
		depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
		depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
		depthOptimizedClearValue.DepthStencil.Stencil = 0;

		ThrowIfFailed(m_d3ddevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, width, height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&depthOptimizedClearValue,
			IID_PPV_ARGS(&m_depthStencil)
		));

		//NAME_D3D12_OBJECT(m_depthStencil);

		m_d3ddevice->CreateDepthStencilView(m_depthStencil.Get(), &depthStencilDesc, m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
	}

	D3D12RenderPass* pass = new D3D12RenderPass();
	std::vector<FrameBufferObject> frameBuffers;
	for (UINT n = 0; n < FrameCount; n++)
	{
		FrameBufferObject fb = { frameBuffersHandles[n], CD3DX12_CPU_DESCRIPTOR_HANDLE(m_dsvHeap->GetCPUDescriptorHandleForHeapStart()), m_renderTargets[n].Get() };
		frameBuffers.push_back(fb);
	}
			//D3D12RenderTarget* colorRT = dynamic_cast<D3D12RenderTarget*>(colorTexture);
	pass->Create(width, height, frameBuffers, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
			//m_renderPasses.push_back(pass);
	m_mainRenderPass = pass;

	//ThrowIfFailed(m_d3ddevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));
	//ThrowIfFailed(m_d3ddevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList)));
	m_commandBuffer = m_device->GetCommandBuffer();

	// Command lists are created in the recording state, but there is nothing
	// to record yet. The main loop expects it to be closed, so close it now.
	//ThrowIfFailed(m_commandList->Close());

	// Create synchronization objects.
	{
		ThrowIfFailed(m_d3ddevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
		m_fenceValue = 1;

		// Create an event handle to use for frame synchronization.
		m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (m_fenceEvent == nullptr)
		{
			ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
		}
	}

	return true;
}

//void VulkanApplication::PrepareUI()
//{
//	settings.overlay = settings.overlay;
//	if (settings.overlay) {
//		UIOverlay._device = vulkanDevice;
//		UIOverlay._queue = queue;
//		engine::render::VertexLayout v;
//		UIOverlay.LoadGeometry(engine::tools::getAssetPath() + "Roboto-Medium.ttf", &v);
//		UIOverlay.preparePipeline(pipelineCache, mainRenderPass->GetRenderPass());
//	}
//}

void D3D12Application::PrepareUI()
{

}

void D3D12Application::WaitForDevice()
{
	// Flush device to make sure all resources can be freed
	/*if (device != VK_NULL_HANDLE) {
		vkDeviceWaitIdle(device);
	}*/
	WaitForPreviousFrame();
}

void D3D12Application::UpdateOverlay()
{
	if (!settings.overlay)
		return;

	/*ImGuiIO& io = ImGui::GetIO();

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
	}*/
}

void D3D12Application::WaitForPreviousFrame()
{
	// WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
	// This is code implemented as such for simplicity. The D3D12HelloFrameBuffering
	// sample illustrates how to use fences for efficient resource usage and to
	// maximize GPU utilization.

	// Signal and increment the fence value.
	const UINT64 fence = m_fenceValue;
	ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), fence));
	m_fenceValue++;

	// Wait until the previous frame is finished.
	if (m_fence->GetCompletedValue() < fence)
	{
		ThrowIfFailed(m_fence->SetEventOnCompletion(fence, m_fenceEvent));
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}

	currentBuffer = m_swapChain->GetCurrentBackBufferIndex();
}

void D3D12Application::Render()
{
	if (!prepared)
		return;

	// Record all the commands we need to render the scene into the command list.
	BuildCommandBuffers();

	// Execute the command list.
	ID3D12CommandList* ppCommandLists[] = { ((render::D3D12CommandBuffer*)m_commandBuffer)->m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// Present the frame.
	ThrowIfFailed(m_swapChain->Present(1, 0));

	WaitForPreviousFrame();

	//vkWaitForFences(device, 1, &submitFences[currentBuffer], VK_TRUE, UINT64_MAX);

	////VulkanApplication::PrepareFrame();
	//// Acquire the next image from the swap chain
	//VkResult result = swapChain.acquireNextImage(presentCompleteSemaphores[currentBuffer], &currentBuffer);
	//// Recreate the swapchain if it's no longer compatible with the surface (OUT_OF_DATE) or no longer optimal for presentation (SUBOPTIMAL)
	//if ((result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR)) {
	//	WindowResize();
	//}
	//else {
	//	VK_CHECK_RESULT(result);
	//}

	//vkResetFences(device, 1, &submitFences[currentBuffer]);

	//std::vector<VkCommandBuffer> submitCommandBuffers(allDrawCommandBuffers.size());
	//for (int i = 0; i < allDrawCommandBuffers.size(); i++)
	//{
	//	submitCommandBuffers[i] = allDrawCommandBuffers[i][currentBuffer];
	//}

	//submitInfo.pWaitSemaphores = &presentCompleteSemaphores[currentBuffer];
	//submitInfo.pSignalSemaphores = &renderCompleteSemaphores[currentBuffer];
	//submitInfo.commandBufferCount = submitCommandBuffers.size();
	//submitInfo.pCommandBuffers = submitCommandBuffers.data();
	//VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, submitFences[currentBuffer]));

	////VulkanApplication::PresentFrame();
	//result = swapChain.queuePresent(presentationQueue, currentBuffer, renderCompleteSemaphores[currentBuffer]);
	//if (!((result == VK_SUCCESS) || (result == VK_SUBOPTIMAL_KHR))) {
	//	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
	//		// Swap chain is no longer compatible with the surface and needs to be recreated
	//		WindowResize();
	//		return;
	//	}
	//	else {
	//		VK_CHECK_RESULT(result);
	//	}
	//}
	//currentBuffer = (currentBuffer + 1) % swapChain.swapChainImageViews.size();
}

void D3D12Application::ViewChanged() {}

void D3D12Application::KeyPressed(uint32_t) {}

void D3D12Application::MouseMoved(double x, double y, bool & handled) {}

void D3D12Application::BuildCommandBuffers() 
{
	// Command list allocators can only be reset when the associated 
	// command lists have finished execution on the GPU; apps should use 
	// fences to determine GPU execution progress.
	/*ThrowIfFailed(m_commandBuffer.m_commandAllocator->Reset());

	// However, when ExecuteCommandList() is called on a particular command 
	// list, that command list can then be reset at any time and must be before 
	// re-recording.
	ThrowIfFailed(m_commandBuffer.m_commandList->Reset(m_commandBuffer.m_commandAllocator.Get(), m_pipelineState.Get()));

	// Indicate that the back buffer will be used as a render target.
	m_commandBuffer.m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[currentBuffer].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), currentBuffer, m_rtvDescriptorSize);

	// Record commands.
	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	m_commandBuffer.m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

	// Indicate that the back buffer will now be used to present.
	m_commandBuffer.m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[currentBuffer].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	ThrowIfFailed(m_commandBuffer.m_commandList->Close());*/
}

void D3D12Application::SubmitOnQueue(render::CommandBuffer* commandBuffer)
{
	render::D3D12CommandBuffer* d3dcommandBuffer = dynamic_cast<render::D3D12CommandBuffer*>(commandBuffer);
	ID3D12CommandList* ppCommandLists[] = { d3dcommandBuffer->m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
}


void D3D12Application::GetEnabledFeatures()
{
	// Can be overriden in derived class
}

void D3D12Application::WindowResize()
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
	//SetupSwapChain();

	//// Recreate the frame buffers
	//vulkanDevice->DestroyTexture(depthStencil);
	//SetupDepthStencil();
	//mainRenderPass->ResetFrameBuffers();
	//SetupFrameBuffer();

	//if ((width > 0.0f) && (height > 0.0f)) {
	//	if (settings.overlay) {
	//		UIOverlay.resize(width, height);
	//	}
	//}

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

void D3D12Application::WindowResized()
{
	// Can be overriden in derived class
}

//void VulkanApplication::OnUpdateUIOverlay(engine::scene::UIOverlay *overlay) {}

D3D12Application::~D3D12Application()
{
	delete m_mainRenderPass;//TODO the device should do that
	delete m_device;
	// Clean up Vulkan resources
	/*swapChain.CleanUp();
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

	vkDestroyInstance(instance, nullptr);*/
	WaitForPreviousFrame();

	CloseHandle(m_fenceEvent);
}