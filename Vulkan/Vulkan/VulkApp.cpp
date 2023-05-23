#include "VulkApp.h"

VulkApp::VulkApp() {
	mTitle = "VulkApp";
}

VulkApp::~VulkApp()
{
#ifdef  __USE__STATE__
	VulkState::getInstance().CleanupVulkan();
#else
	cleanupVulkan();
#endif // ! __USE__STAGE__

	
}

float VulkApp::AspectRatio()const {
	return static_cast<float>(mClientWidth) / mClientHeight;
}

void VulkApp::Run() {
	SDL_Event sdlEvent;
	bool run = true;
	bool isActive = false;
#ifdef __USE__TIMER__
	mTimer.Reset();
#else
	i64 startTime = SDL_GetTicks64();
#endif
	while (run) {
		while (SDL_PollEvent(&sdlEvent)) {
			switch (sdlEvent.type) {
			case SDL_QUIT:
				run = false;
				break;
			case SDL_KEYDOWN:
				if (sdlEvent.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
					sdlEvent.type = SDL_QUIT;
					SDL_PushEvent(&sdlEvent);
				}
				mKeys[sdlEvent.key.keysym.scancode] = true;
				break;
			case SDL_KEYUP:
				mKeys[sdlEvent.key.keysym.scancode] = false;
				break;
			case SDL_WINDOWEVENT:
				if (sdlEvent.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
					isActive = true;
#ifdef __USE__TIMER__
					mTimer.Start();
#endif
				}
				else if (sdlEvent.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
					isActive = false;
#ifdef __USE__TIMER__
					mTimer.Stop();
#endif
				}
				break;
			case SDL_MOUSEBUTTONDOWN:
				mButtons[sdlEvent.button.button] = true;
				mMousePos = glm::ivec2(sdlEvent.button.x, sdlEvent.button.y);
				break;
			case SDL_MOUSEBUTTONUP:
				mButtons[sdlEvent.button.button] = false;
				mMousePos = glm::ivec2(sdlEvent.button.x, sdlEvent.button.y);
				break;
			case SDL_MOUSEWHEEL:
				scrollY = sdlEvent.wheel.preciseY;
				scrollX = sdlEvent.wheel.preciseX;
				
				break;
			case SDL_MOUSEMOTION:
				mMousePos = glm::ivec2(sdlEvent.button.x, sdlEvent.button.y);
				break;
			}
			
				
			

		}
		if (isActive) {
#ifdef __USE__TIMER__
			mTimer.Tick();
			float deltaTime = mTimer.DeltaTime();
			CalculateFrameStats();
#else
			i64 t = SDL_GetTicks64();
			i64 delta = t - startTime;
			float deltaTime = (delta) *0.001f;
			CalculateFrameStats(t);
			startTime = t;
#endif
			
			Update(deltaTime);
			Draw(deltaTime);


		}
	}
#ifdef __USE__STATE__
	VulkState::getInstance().WaitDevice();
	
#else
	vkDeviceWaitIdle(mDevice);
#endif
}
#ifdef __USE__TIMER__
void VulkApp::CalculateFrameStates(){
#else
void VulkApp::CalculateFrameStats(i64 ticks) {
#endif
	static int frameCnt = 0;
	static float timeElapsed = 0.f;

	frameCnt++;
	//timeElapsed += deltaTime;
#ifdef __USE__TIMER__
	if ((mTimer.TotalTime() - timeElapsed) >= 1.f) {
#else 
	float tickspersecond = ticks*0.001f;
	
	if((tickspersecond-timeElapsed)>=1.f){
#endif
		float fps = (float)frameCnt;//fps = frameCnt/1
		float mspf = 1000.f / fps;
		char buffer[256];
		sprintf_s(buffer, sizeof(buffer), "%s    fps:%f,   mspf:%f", mTitle.c_str(), fps, mspf);
		SDL_SetWindowTitle(m_mainWindow, buffer);
		//Reset
		frameCnt = 0;
		timeElapsed += 1.f;
	}
}

bool VulkApp::Initialize(uint32_t clientWidth,uint32_t clientHeight) {
	if (clientWidth != UINT32_MAX && clientHeight != UINT32_MAX) {
		mClientWidth = clientWidth;
		mClientHeight = clientHeight;
	}
	if (!InitMainWindow())
		return false;
#ifdef __USE__STATE__
	
	VulkState::getInstance().Init(mInitFlags, m_mainWindow);
#else
	if (!InitVulkan())
		return false;
#endif
	OnResize();
	return true;
}
bool VulkApp::InitMainWindow() {
	assert(SDL_Init(SDL_INIT_VIDEO) >= 0);
	m_mainWindow = SDL_CreateWindow(mTitle.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, mClientWidth, mClientHeight, SDL_WINDOW_VULKAN);
	return true;
}
#ifndef __USE__STATE__
bool VulkApp::InitVulkan() {
	std::vector<const char*> requiredExtensions{ "VK_KHR_surface",VK_KHR_WIN32_SURFACE_EXTENSION_NAME };
	std::vector<const char*> requiredLayers;
	if (mEnableValidation)
		requiredLayers = { "VK_LAYER_KHRONOS_validation" };
	mInstance = Vulkan::initInstance(requiredExtensions, requiredLayers);
	//mSurface = Vulkan::initSurface(mInstance, mhAppInst, mhMainWnd);
	assert(SDL_Vulkan_CreateSurface(m_mainWindow, mInstance, &mSurface));
	mPhysicalDevice = choosePhysicalDevice(mInstance, mSurface, mQueues);
	vkGetPhysicalDeviceProperties(mPhysicalDevice, &mDeviceProperties);
	vkGetPhysicalDeviceMemoryProperties(mPhysicalDevice, &mMemoryProperties);
	vkGetPhysicalDeviceFeatures(mPhysicalDevice, &mDeviceFeatures);
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(mPhysicalDevice, mSurface, &mSurfaceCaps);
	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(mPhysicalDevice, mSurface, &formatCount, nullptr);
	mSurfaceFormats.resize(formatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(mPhysicalDevice, mSurface, &formatCount, mSurfaceFormats.data());
	uint32_t presentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(mPhysicalDevice, mSurface, &presentModeCount, nullptr);
	mPresentModes.resize(presentModeCount);
	vkGetPhysicalDeviceSurfacePresentModesKHR(mPhysicalDevice, mSurface, &presentModeCount, mPresentModes.data());

	mNumSamples = Vulkan::getMaxUsableSampleCount(mDeviceProperties);
	std::vector<const char*> deviceExtensions{ "VK_KHR_swapchain" };
	VkPhysicalDeviceFeatures enabledFeatures{};
	if (mDeviceFeatures.samplerAnisotropy)
		enabledFeatures.samplerAnisotropy = VK_TRUE;
	if (mDeviceFeatures.sampleRateShading)
		enabledFeatures.sampleRateShading = VK_TRUE;
	if (mLineWidth && mDeviceFeatures.wideLines) {
		enabledFeatures.wideLines = VK_TRUE;
	}

	if (mGeometryShader && mDeviceFeatures.geometryShader)
		enabledFeatures.geometryShader = VK_TRUE;
	if (mAllowWireframe && mDeviceFeatures.fillModeNonSolid) {
		enabledFeatures.fillModeNonSolid = VK_TRUE;
	}

	uint32_t queueCount = 2;
	mDevice = initDevice(mPhysicalDevice, deviceExtensions, mQueues, enabledFeatures, queueCount);

	mGraphicsQueue = Vulkan::getDeviceQueue(mDevice, mQueues.graphicsQueueFamily);
	mPresentQueue = Vulkan::getDeviceQueue(mDevice, mQueues.presentQueueFamily);
	mComputeQueue = Vulkan::getDeviceQueue(mDevice, mQueues.computeQueueFamily);

	mBackQueue = Vulkan::getDeviceQueue(mDevice, mQueues.graphicsQueueFamily, 1);


	mPresentMode = Vulkan::chooseSwapchainPresentMode(mPresentModes);
	mSwapchainFormat = Vulkan::chooseSwapchainFormat(mSurfaceFormats);
	vkGetPhysicalDeviceFormatProperties(mPhysicalDevice, mSwapchainFormat.format, &mFormatProperties);
	uint32_t swapChainImageCount = Vulkan::getSwapchainImageCount(mSurfaceCaps);
	if (mSwapChainImageCount != UINT32_MAX) {
		swapChainImageCount = mSwapChainImageCount;
	}
	//mPresentComplete = Vulkan::initSemaphore(mDevice);
	//mRenderComplete = Vulkan::initSemaphore(mDevice);
	mMaxFrames = swapChainImageCount;
	for (uint32_t i = 0; i < swapChainImageCount; i++) {
		VkSemaphore presentComplete = Vulkan::initSemaphore(mDevice);
		mPresentCompletes.push_back(presentComplete);
		VkSemaphore renderComplete = Vulkan::initSemaphore(mDevice);
		mRenderCompletes.push_back(renderComplete);
		VkFence fence = Vulkan::initFence(mDevice, VK_FENCE_CREATE_SIGNALED_BIT);
		mFences.push_back(fence);
	}
	mCommandPool = Vulkan::initCommandPool(mDevice, mQueues.graphicsQueueFamily);
	mCommandBuffer = Vulkan::initCommandBuffer(mDevice, mCommandPool);

	Vulkan::initCommandPools(mDevice, swapChainImageCount, mQueues.graphicsQueueFamily, mCommandPools);
	Vulkan::initCommandBuffers(mDevice, mCommandPools, mCommandBuffers);
	VkDevice device = mDevice;
	pvkAcquireNextImage = (PFN_vkAcquireNextImageKHR)vkGetDeviceProcAddr(device, "vkAcquireNextImageKHR");
	assert(pvkAcquireNextImage);
	pvkQueuePresent = (PFN_vkQueuePresentKHR)vkGetDeviceProcAddr(device, "vkQueuePresentKHR");
	assert(pvkQueuePresent);
	pvkQueueSubmit = (PFN_vkQueueSubmit)vkGetDeviceProcAddr(device, "vkQueueSubmit");
	assert(pvkQueueSubmit);
	pvkBeginCommandBuffer = (PFN_vkBeginCommandBuffer)vkGetDeviceProcAddr(device, "vkBeginCommandBuffer");
	assert(pvkBeginCommandBuffer);
	pvkCmdBeginRenderPass = (PFN_vkCmdBeginRenderPass)vkGetDeviceProcAddr(device, "vkCmdBeginRenderPass");
	assert(pvkCmdBeginRenderPass);
	pvkCmdBindPipeline = (PFN_vkCmdBindPipeline)vkGetDeviceProcAddr(device, "vkCmdBindPipeline");
	assert(pvkCmdBindPipeline);
	pvkCmdBindVertexBuffers = (PFN_vkCmdBindVertexBuffers)vkGetDeviceProcAddr(device, "vkCmdBindVertexBuffers");
	assert(pvkCmdBindVertexBuffers);
	pvkCmdBindIndexBuffer = (PFN_vkCmdBindIndexBuffer)vkGetDeviceProcAddr(device, "vkCmdBindIndexBuffer");
	assert(pvkCmdBindIndexBuffer);
	pvkCmdDrawIndexed = (PFN_vkCmdDrawIndexed)vkGetDeviceProcAddr(device, "vkCmdDrawIndexed");
	assert(pvkCmdDrawIndexed);
	pvkCmdEndRenderPass = (PFN_vkCmdEndRenderPass)vkGetDeviceProcAddr(device, "vkCmdEndRenderPass");
	assert(pvkCmdEndRenderPass);
	pvkEndCommandBuffer = (PFN_vkEndCommandBuffer)vkGetDeviceProcAddr(device, "vkEndCommandBuffer");
	assert(pvkEndCommandBuffer);
	pvkQueueWaitIdle = (PFN_vkQueueWaitIdle)vkGetDeviceProcAddr(device, "vkQueueWaitIdle");
	assert(pvkQueueWaitIdle);
	pvkCmdBindDescriptorSets = (PFN_vkCmdBindDescriptorSets)vkGetDeviceProcAddr(device, "vkCmdBindDescriptorSets");
	assert(pvkCmdBindDescriptorSets);
	pvkCmdDispatch = (PFN_vkCmdDispatch)vkGetDeviceProcAddr(device, "vkCmdDispatch");
	assert(pvkCmdDispatch);
	pvkCmdSetViewport = (PFN_vkCmdSetViewport)vkGetDeviceProcAddr(device, "vkCmdSetViewport");
	assert(pvkCmdSetViewport);
	pvkCmdSetScissor = (PFN_vkCmdSetScissor)vkGetDeviceProcAddr(device, "vkCmdSetScissor");
	mSubmitInfo.pWaitDstStageMask = &mSubmitPipelineStages;
	mSubmitInfo.waitSemaphoreCount = 1;
	//mSubmitInfo.pWaitSemaphores = &mPresentComplete;
	mSubmitInfo.signalSemaphoreCount = 1;
	//mSubmitInfo.pSignalSemaphores = &mRenderComplete;
	mSubmitInfo.commandBufferCount = 1;
	mRenderPassBeginInfo.clearValueCount = sizeof(mClearValues) / sizeof(mClearValues[0]);
	mRenderPassBeginInfo.pClearValues = mClearValues;
	mPresentInfo.swapchainCount = 1;
	mPresentInfo.pImageIndices = &mIndex;
	//mPresentInfo.pWaitSemaphores = &mRenderComplete;
	mPresentInfo.waitSemaphoreCount = 1;
	return true;
}

void VulkApp::CreateSwapchain() {
	VkSwapchainKHR oldSwapchain = mSwapchain;

	mSwapchain = Vulkan::initSwapchain(mDevice, mSurface, mClientWidth, mClientHeight, mSurfaceCaps, mPresentMode, mSwapchainFormat, mSwapchainExtent, mSwapChainImageCount, oldSwapchain);
	if (oldSwapchain != VK_NULL_HANDLE) {
		Vulkan::cleanupSwapchain(mDevice, oldSwapchain);
	}
	Vulkan::getSwapchainImages(mDevice, mSwapchain, mSwapchainImages);

	mSwapChainImageCount = (uint32_t)mSwapchainImages.size();
	Vulkan::initSwapchainImageViews(mDevice, mSwapchainImages, mSwapchainFormat.format, mSwapchainImageViews);
	VkSampleCountFlagBits numSamples = VK_SAMPLE_COUNT_1_BIT;
	Vulkan::ImageProperties props;
	props.width = mClientWidth;
	props.height = mClientHeight;
	numSamples = mNumSamples;
#ifdef __USE__VMA__
	props.usage = VMA_MEMORY_USAGE_GPU_ONLY;
#else
	props.memoryProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
#endif
	props.samples = numSamples;
	if (mMSAA) {

		props.format = mSwapchainFormat.format;
		props.imageUsage = (VkImageUsageFlagBits)(VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
		Vulkan::initImage(mDevice, mMemoryProperties, props, mMsaaImage);
	}
	if (mDepthBuffer) {
		props.format = mDepthFormat;
		props.samples = VK_SAMPLE_COUNT_1_BIT;
		props.imageUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | mDepthImageUsage;
		props.aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
		Vulkan::initImage(mDevice, mMemoryProperties, props, mDepthImage);

		//Vulkan::initDepthImage(mDevice, mDepthFormat, mFormatProperties, mMemoryProperties, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mMSAA ? mNumSamples : VK_SAMPLE_COUNT_1_BIT, mClientWidth, mClientHeight, mDepthImage);
	}
	props.aspect = VK_IMAGE_ASPECT_COLOR_BIT;
	//setup render pass based on config
	Vulkan::RenderPassProperties rpProps;
	rpProps.colorFormat = mSwapchainFormat.format;
	rpProps.sampleCount = mMSAA ? numSamples : VK_SAMPLE_COUNT_1_BIT;
	rpProps.depthFormat = mDepthBuffer ? mDepthFormat : VK_FORMAT_UNDEFINED;
	rpProps.resolveFormat = mMSAA ? rpProps.colorFormat : VK_FORMAT_UNDEFINED;
	mRenderPass = Vulkan::initRenderPass(mDevice, rpProps);
	Vulkan::FramebufferProperties fbProps;
	fbProps.colorAttachments = mSwapchainImageViews;
	fbProps.depthAttachment = mDepthBuffer ? mDepthImage.imageView : VK_NULL_HANDLE;
	fbProps.resolveAttachment = mMSAA ? mMsaaImage.imageView : VK_NULL_HANDLE;
	fbProps.width = mClientWidth;
	fbProps.height = mClientHeight;
	Vulkan::initFramebuffers(mDevice, mRenderPass, fbProps, mFramebuffers);
	mRenderPassBeginInfo.renderPass = mRenderPass;
	mRenderPassBeginInfo.renderArea = { 0,0,(uint32_t)mClientWidth,(uint32_t)mClientHeight };

	mPresentInfo.pSwapchains = &mSwapchain;
}

void VulkApp::cleanupVulkan() {
	vkDeviceWaitIdle(mDevice);
	DestroySwapchain();
	Vulkan::cleanupSwapchain(mDevice, mSwapchain);
	Vulkan::cleanupCommandBuffers(mDevice, mCommandPools, mCommandBuffers);
	Vulkan::cleanupCommandPools(mDevice, mCommandPools);
	Vulkan::cleanupCommandBuffer(mDevice, mCommandPool, mCommandBuffer);
	Vulkan::cleanupCommandPool(mDevice, mCommandPool);

	//Vulkan::cleanupSemaphore(mDevice, mRenderComplete);
	//Vulkan::cleanupSemaphore(mDevice, mPresentComplete);
	for (uint32_t i = 0; i < mMaxFrames; i++) {
		Vulkan::cleanupSemaphore(mDevice, mPresentCompletes[i]);
		Vulkan::cleanupSemaphore(mDevice, mRenderCompletes[i]);
		Vulkan::cleanupFence(mDevice, mFences[i]);
	}
	Vulkan::cleanupDevice(mDevice);
	Vulkan::cleanupSurface(mInstance, mSurface);
	Vulkan::cleanupInstance(mInstance);
}


void VulkApp::DestroySwapchain() {
	Vulkan::cleanupFramebuffers(mDevice, mFramebuffers);
	if (mMsaaImage.image != VK_NULL_HANDLE)
		Vulkan::cleanupImage(mDevice, mMsaaImage);
	if (mDepthImage.image != VK_NULL_HANDLE)
		Vulkan::cleanupImage(mDevice, mDepthImage);
	Vulkan::cleanupRenderPass(mDevice, mRenderPass);
	Vulkan::cleanupSwapchainImageViews(mDevice, mSwapchainImageViews);
	//cleanupSwapchain(mDevice, mSwapchain);
}

VkCommandBuffer VulkApp::BeginRender(bool startRenderPass) {


	mSubmitInfo.pWaitSemaphores = &mPresentCompletes[mCurrFrame];
	mSubmitInfo.pSignalSemaphores = &mRenderCompletes[mCurrFrame];

	mRenderPassBeginInfo.clearValueCount = sizeof(mClearValues) / sizeof(mClearValues[0]);
	mRenderPassBeginInfo.pClearValues = mClearValues;
	mPresentInfo.swapchainCount = 1;
	mPresentInfo.pImageIndices = &mIndex;
	mPresentInfo.pWaitSemaphores = &mRenderCompletes[mCurrFrame];
	VkResult res = pvkAcquireNextImage(mDevice, mSwapchain, UINT64_MAX, mPresentCompletes[mCurrFrame], nullptr, &mIndex);
	assert(res == VK_SUCCESS);


	VkCommandBuffer cmd = mCommandBuffers[mIndex];


	pvkBeginCommandBuffer(cmd, &mBeginInfo);


	mRenderPassBeginInfo.framebuffer = mFramebuffers[mIndex];
	if (startRenderPass)
		pvkCmdBeginRenderPass(cmd, &mRenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	return cmd;
}

void VulkApp::EndRender(VkCommandBuffer cmd) {
	pvkCmdEndRenderPass(cmd);


	VkResult res = pvkEndCommandBuffer(cmd);
	assert(res == VK_SUCCESS);
	VkFence fence = mFences[mCurrFrame];
	mSubmitInfo.pCommandBuffers = &cmd;
	res = pvkQueueSubmit(mGraphicsQueue, 1, &mSubmitInfo, mCurrFence);
	assert(res == VK_SUCCESS);

	res = pvkQueuePresent(mPresentQueue, &mPresentInfo);
	assert(res == VK_SUCCESS);
	//pvkQueueWaitIdle(mPresentQueue);
	mFrameCount++;
}

#endif


void VulkApp::OnResize()
{
#ifdef __USE__STATE__
	VulkState::getInstance().Resize(mClientWidth, mClientHeight);
#else
	vkDeviceWaitIdle(mDevice);
	DestroySwapchain();
	CreateSwapchain();
#endif
}

void VulkApp::Update(const float deltaTime) {
#ifdef __USE__STATE__
	VulkState::getInstance().NextFrame();
#else
	mCurrFrame = (mCurrFrame + 1) % mMaxFrames;
	mCurrFence = mFences[mCurrFrame];
	vkWaitForFences(mDevice, 1, &mCurrFence, VK_TRUE, UINT64_MAX);
	vkResetFences(mDevice, 1, &mCurrFence);
#endif
}

