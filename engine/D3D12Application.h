#pragma once

#include "ApplicationBase.h"
#include <d3d12.h>
#include <dxgi1_6.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include "render/directx/d3dx12.h"
#include "render/directx/D3D12CommandBuffer.h"

#include <string>
#include <wrl.h>
#include <shellapi.h>

using namespace engine;

class D3D12Application : public ApplicationBase
{
private:	
	// Called if the window is resized and some resources have to be recreatesd
	
	//void HandleMouseMove(int32_t x, int32_t y);
protected:
	static const UINT FrameCount = 2;
	Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapChain;
	Microsoft::WRL::ComPtr<ID3D12Device> m_d3ddevice;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
	Microsoft::WRL::ComPtr<ID3D12Resource> m_depthStencil;
	std::vector <std::vector<ID3D12CommandList*>> alld3dDrawCommandLists;
	//render::D3D12CommandBuffer m_commandBuffer;
	//Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandAllocator;
	//Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;
	UINT m_rtvDescriptorSize;
	UINT m_cbvSrvDescriptorSize;

	CD3DX12_VIEWPORT m_viewport;
	CD3DX12_RECT m_scissorRect;

	// Synchronization objects.
	//UINT m_frameIndex;
	HANDLE m_fenceEvent;
	Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
	UINT64 m_fenceValue;
	
public: 
	D3D12Application::D3D12Application(bool enableValidation);

	// Setup the vulkan instance, enable required extensions and connect to the physical device (GPU)
	virtual bool InitAPI();

	//Prepare ui elements
	void PrepareUI();

	void WaitForPreviousFrame();
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

	virtual void CreateAllCommandBuffers();
	virtual std::vector<render::CommandBuffer*> CreateCommandBuffers();
	// Pure virtual function to be overriden by the dervice class
	// Called in case of an event where e.g. the framebuffer has to be rebuild and thus
	// all command buffers that may reference this
	virtual void BuildCommandBuffers();

	virtual void SubmitOnQueue(class render::CommandBuffer *commandBuffer);

	/** @brief (Virtual) Called after the physical device features have been read, can be used to set features to enable on the device */
	virtual void GetEnabledFeatures();

	// Render one frame of a render loop on platforms that sync rendering
	//void UpdateFrame();

	virtual void UpdateOverlay();
	//void DrawUI(const VkCommandBuffer commandBuffer);

	virtual void WaitForDevice();

	virtual void DrawFullScreenQuad(render::CommandBuffer* commandBuffer);
	/** @brief (Virtual) Called when the UI overlay is updating, can be used to add custom elements to the overlay */
	//virtual void OnUpdateUIOverlay(engine::scene::UIOverlay *overlay);

	virtual const std::string GetShadersPath();
	virtual const std::string GetVertexShadersExt();
	virtual const std::string GetFragShadersExt();

	virtual ~D3D12Application();
};
