#pragma once
#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include <exception>
#include <cassert>
#include <string>
#ifdef __USE__THREAD__
#include <memory>
#include <future>
#endif
#define __USE__STATE__
#ifdef __USE__STATE__
#include "VulkState.h"
#else
#include "Vulkan.h"
#endif


#include <glm/glm.hpp>
#ifdef __USE__TIMER__
#include "Timer.h"
#endif



class VulkApp {
protected:
	SDL_Window* m_mainWindow;
	std::string mTitle;
#ifdef __USE__TIMER__
	Timer	mTimer;
#endif
	uint8_t mKeys[128] = { 0 };
	uint8_t mButtons[8] = { 0 };
	float scrollY;
	float scrollX;
	glm::ivec2 mMousePos;
	bool      mAppPaused = false;  // is the application paused?
	bool      mMinimized = false;  // is the application minimized?
	bool      mMaximized = false;  // is the application maximized?
	bool      mResizing = false;   // are the resize bars being dragged?
	bool      mFullscreenState = false;// fullscreen enabled
	uint32_t mClientWidth = 800;
	uint32_t mClientHeight = 600;
	
#ifdef __USE__STATE__
	VulkStateInitFlags mInitFlags;
#else

	bool        mGeometryShader{ false };
	bool        mDepthBuffer{ true };
	bool        mMSAA{ true };
	bool    mAllowWireframe{ false };
	bool    mEnableValidation{ true };
	bool    mLineWidth{ false };
	uint32_t mSwapChainImageCount{ UINT32_MAX };
	VkSurfaceKHR                        mSurface{ VK_NULL_HANDLE };

	VkInstance                          mInstance{ VK_NULL_HANDLE };
	
	VkPhysicalDevice                    mPhysicalDevice{ VK_NULL_HANDLE };
	Vulkan::Queues                              mQueues;
	VkQueue                             mGraphicsQueue{ VK_NULL_HANDLE };
	VkQueue                             mPresentQueue{ VK_NULL_HANDLE };
	VkQueue                             mComputeQueue{ VK_NULL_HANDLE };

	VkQueue                             mBackQueue{ VK_NULL_HANDLE };

	VkPhysicalDeviceProperties          mDeviceProperties;
	VkPhysicalDeviceMemoryProperties    mMemoryProperties;
	VkPhysicalDeviceFeatures            mDeviceFeatures;
	VkSurfaceCapabilitiesKHR            mSurfaceCaps;
	std::vector<VkSurfaceFormatKHR>     mSurfaceFormats;
	std::vector<VkPresentModeKHR>       mPresentModes;
	VkSampleCountFlagBits               mNumSamples{ VK_SAMPLE_COUNT_1_BIT };
	VkDevice                            mDevice{ VK_NULL_HANDLE };
	VkPresentModeKHR                    mPresentMode;
	VkSurfaceFormatKHR                  mSwapchainFormat;
	VkFormatProperties                  mFormatProperties;
	VkExtent2D                          mSwapchainExtent{};
	VkSwapchainKHR                      mSwapchain{ VK_NULL_HANDLE };
	std::vector<VkImage>                mSwapchainImages;
	std::vector<VkImageView>            mSwapchainImageViews;
	std::vector<VkSemaphore>            mPresentCompletes;
	std::vector<VkSemaphore>            mRenderCompletes;
	std::vector<VkFence>                mFences;
	VkFence                             mCurrFence{ VK_NULL_HANDLE };
	VkCommandPool                       mCommandPool{ VK_NULL_HANDLE };
	VkCommandBuffer                     mCommandBuffer{ VK_NULL_HANDLE };
	std::vector<VkCommandPool>          mCommandPools;
	std::vector<VkCommandBuffer>        mCommandBuffers;
	VkFormat                            mDepthFormat = VK_FORMAT_D32_SFLOAT;
	VkImageUsageFlags                   mDepthImageUsage = 0;
	VkRenderPass                        mRenderPass{ VK_NULL_HANDLE };
	Vulkan::Image                               mDepthImage;
	Vulkan::Image                               mMsaaImage;
	std::vector<VkFramebuffer>          mFramebuffers;
	PFN_vkAcquireNextImageKHR pvkAcquireNextImage{ nullptr };
	PFN_vkQueuePresentKHR pvkQueuePresent{ nullptr };
	PFN_vkQueueSubmit pvkQueueSubmit{ nullptr };
	PFN_vkBeginCommandBuffer pvkBeginCommandBuffer{ nullptr };
	PFN_vkCmdBeginRenderPass pvkCmdBeginRenderPass{ nullptr };
	PFN_vkCmdBindPipeline pvkCmdBindPipeline{ nullptr };
	PFN_vkCmdBindVertexBuffers pvkCmdBindVertexBuffers{ nullptr };
	PFN_vkCmdBindIndexBuffer pvkCmdBindIndexBuffer{ nullptr };
	PFN_vkCmdDrawIndexed pvkCmdDrawIndexed{ nullptr };
	PFN_vkCmdEndRenderPass pvkCmdEndRenderPass{ nullptr };
	PFN_vkEndCommandBuffer pvkEndCommandBuffer{ nullptr };
	PFN_vkQueueWaitIdle pvkQueueWaitIdle{ nullptr };
	PFN_vkCmdBindDescriptorSets pvkCmdBindDescriptorSets{ nullptr };
	PFN_vkCmdDispatch pvkCmdDispatch{ nullptr };
	PFN_vkCmdDraw pvkCmdDraw{ nullptr };
	PFN_vkCmdSetViewport pvkCmdSetViewport{ nullptr };
	PFN_vkCmdSetScissor pvkCmdSetScissor{ nullptr };
	VkClearValue                        mClearValues[2] = { {0.0f, 0.0f, 0.0f, 0.0f},{1.0f,0.0f } };
	VkCommandBufferBeginInfo            mBeginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	VkRenderPassBeginInfo               mRenderPassBeginInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	VkDeviceSize                        mOffsets[1] = { 0 };
	VkPipelineStageFlags                mSubmitPipelineStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo		                mSubmitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
	VkPresentInfoKHR                    mPresentInfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	uint32_t                            mIndex = 0;
	uint32_t                            mFrameCount = 0;
	uint32_t                            mCurrFrame{ (uint32_t)(-1) };
	uint32_t                            mMaxFrames{ 0 };
	bool InitVulkan();
	void CreateSwapchain();
	void DestroySwapchain();
	void cleanupVulkan();
	VkCommandBuffer BeginRender(bool startRenderPass = true);
	void            EndRender(VkCommandBuffer cmd);

#endif
	bool InitMainWindow();
	
#ifdef __USE__TIMER__
	void CalculateFrameStats();
#else
	void CalculateFrameStats(i64 ticks);
#endif
	// Convenience overrides for handling mouse input.
	virtual void OnMouseDown(WPARAM btnState, int x, int y) { }
	virtual void OnMouseUp(WPARAM btnState, int x, int y) { }
	virtual void OnMouseMove(WPARAM btnState, int x, int y) { }
	virtual void OnMouseWheel(WPARAM btnState, int delta, int x, int y) {}
	virtual void OnResize();
	virtual void Update(const float deltaTime);
	virtual void Draw(const float deltaTime) = 0;
	VulkApp();
	VulkApp(const VulkApp& rhs) = delete;
	VulkApp& operator=(const VulkApp& rhs) = delete;
	virtual ~VulkApp();
public:
	float     AspectRatio()const;
	void Run();

	virtual bool Initialize(uint32_t clientWidth=UINT32_MAX,uint32_t clientHeight=UINT32_MAX);
};

