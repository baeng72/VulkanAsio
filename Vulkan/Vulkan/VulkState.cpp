#include "pch.h"

VulkState vulkState;


VulkSwapchain::VulkSwapchain() {

}

VulkSwapchain::~VulkSwapchain() {
	Destroy();
	if(swapchain!=VK_NULL_HANDLE)
		Vulkan::cleanupSwapchain(device, swapchain);
}

void VulkSwapchain::Create(VkDevice device, VkSurfaceKHR surface,VkQueue graphicsQueue,VkQueue presentQueue, VkPhysicalDeviceMemoryProperties memoryProperties, VulkSwapchainFlags flags) {

	this->device = device;
	this->surface = surface;
	this->graphicsQueue = graphicsQueue;
	this->presentQueue = presentQueue;
	this->memoryProperties = memoryProperties;
	this->flags = flags;
	this->maxFrames = flags.imageCount;
	
	if (flags.clearValueCount > 0 && flags.clearValues) {
		clearValues.resize(flags.clearValueCount);
		for (u32 i = 0; i < flags.clearValueCount; i++) {
			clearValues[i] = flags.clearValues[i];
		}
	}
	else {
		clearValues.resize(2);
		clearValues[0] = { 0.0f, 0.0f, 0.0f, 0.0f };
		clearValues[1] = { 1.f,0.f };
	}
	
	if (presentCompletes.size()) {
		for (auto presentComplete : presentCompletes) {
			Vulkan::cleanupSemaphore(device,presentComplete);
		}
	}
	if (renderCompletes.size()) {
		for (auto renderComplete : renderCompletes) {
			Vulkan::cleanupSemaphore(device, renderComplete);
		}
	}
	if (fences.size()) {
		for (auto fence : fences) {
			Vulkan::cleanupFence(device, fence);
		}
	}
	presentCompletes.resize(flags.imageCount);
	renderCompletes.resize(flags.imageCount);
	for (u32 i = 0; i < flags.imageCount; i++) {

		VkSemaphore presentComplete = Vulkan::initSemaphore(device);
		
		presentCompletes[i] = presentComplete;
		VkSemaphore renderComplete = Vulkan::initSemaphore(device);
		
		renderCompletes[i] = renderComplete;
		VkFence fence = Vulkan::initFence(device, VK_FENCE_CREATE_SIGNALED_BIT);
		
		fences.push_back(fence);
#ifdef ENABLE_DEBUG_MARKER
		char buffer[128];
		sprintf_s(buffer, "PresentComplete%d", i);
		NAME_SEMAPHORE(presentComplete, buffer);
		sprintf_s(buffer, "RenderComplete%d", i);
		NAME_SEMAPHORE(renderComplete, buffer);
		sprintf_s(buffer, "Fence%d", i);
		NAME_FENCE(fence, buffer);
#endif
	}

	Vulkan::initCommandPools(device, flags.imageCount, flags.graphicsQueueFamily, commandPools);
	Vulkan::initCommandBuffers(device, commandPools, commandBuffers);

	
	VkSwapchainKHR old = swapchain;
	VkExtent2D extent = { flags.clientWidth,flags.clientHeight };
	swapchain = Vulkan::initSwapchain(device, surface, flags.clientWidth, flags.clientHeight, flags.surfaceCaps, flags.presentMode, flags.format,extent, flags.imageCount, old);
	
	assert(swapchain);
	if (old != VK_NULL_HANDLE)
		Vulkan::cleanupSwapchain(device, old);
	Vulkan::getSwapchainImages(device, swapchain, swapchainImages);
	flags.imageCount = (u32)swapchainImages.size();
	Vulkan::initSwapchainImageViews(device, swapchainImages, flags.format.format, swapchainImageViews);

	Vulkan::ImageProperties props;
	props.width = flags.clientWidth;
	props.height = flags.clientHeight;
	props.samples = flags.samples;
#ifdef __USE__VMA__
	props.usage = VMA_MEMORY_USAGE_GPU_ONLY;
#else
	props.memoryProps = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
#endif
	
	if (flags.enableMSAA) {

		props.format = flags.format.format;
		props.imageUsage = (VkImageUsageFlagBits)(VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
		Vulkan::initImage(device, memoryProperties, props, msaaImage);
	}
	if (flags.enableDepthBuffer) {
		props.format = flags.depthFormat;
		props.samples = VK_SAMPLE_COUNT_1_BIT;
		props.imageUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | flags.depthImageUsage;
		props.aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
		Vulkan::initImage(device, memoryProperties, props, depthImage);

		//Vulkan::initDepthImage(mDevice, mDepthFormat, mFormatProperties, mMemoryProperties, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mMSAA ? mNumSamples : VK_SAMPLE_COUNT_1_BIT, mClientWidth, mClientHeight, mDepthImage);
	}
	props.aspect = VK_IMAGE_ASPECT_COLOR_BIT;
	//setup render pass based on config
	Vulkan::RenderPassProperties rpProps;
	rpProps.colorFormat = flags.format.format;
	rpProps.sampleCount = flags.enableMSAA ? flags.samples : VK_SAMPLE_COUNT_1_BIT;
	rpProps.depthFormat = flags.enableDepthBuffer ? flags.depthFormat : VK_FORMAT_UNDEFINED;
	rpProps.resolveFormat = flags.enableMSAA ? rpProps.colorFormat : VK_FORMAT_UNDEFINED;
	renderPass = Vulkan::initRenderPass(device, rpProps);
	Vulkan::FramebufferProperties fbProps;
	fbProps.colorAttachments = swapchainImageViews;
	fbProps.depthAttachment = flags.enableDepthBuffer ? depthImage.imageView : VK_NULL_HANDLE;
	fbProps.resolveAttachment = flags.enableMSAA ? msaaImage.imageView : VK_NULL_HANDLE;
	fbProps.width = flags.clientWidth;
	fbProps.height = flags.clientHeight;
	Vulkan::initFramebuffers(device, renderPass, fbProps, framebuffers);
	renderPassBeginInfo.renderPass = renderPass;
	renderPassBeginInfo.renderArea = { 0,0,(uint32_t)flags.clientWidth,(uint32_t)flags.clientHeight };
	renderPassBeginInfo.clearValueCount = (u32)clearValues.size();
	renderPassBeginInfo.pClearValues = clearValues.data();
	submitInfo.pWaitDstStageMask = &submitPipelineStages;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.commandBufferCount = 1;
	presentInfo.swapchainCount = 1;
	presentInfo.pImageIndices = &index;
	presentInfo.pSwapchains = &swapchain;
	presentInfo.waitSemaphoreCount = 1;
	currFrame = UINT32_MAX;
}

void VulkSwapchain::Destroy() {
	vkDeviceWaitIdle(device);
	Vulkan::cleanupCommandBuffers(device, commandPools, commandBuffers);
	commandBuffers.clear();
	Vulkan::cleanupCommandPools(device, commandPools);
	commandPools.clear();
	if (presentCompletes.size()) {
		for (auto presentComplete : presentCompletes) {
			Vulkan::cleanupSemaphore(device, presentComplete);
		}
	}
	presentCompletes.clear();
	if (renderCompletes.size()) {
		for (auto renderComplete : renderCompletes) {
			Vulkan::cleanupSemaphore(device, renderComplete);
		}
	}
	renderCompletes.clear();
	if (fences.size()) {
		for (auto fence : fences) {
			Vulkan::cleanupFence(device, fence);
		}
	}
	fences.clear();
	Vulkan::cleanupFramebuffers(device, framebuffers);
	framebuffers.clear();
	if (msaaImage.image != VK_NULL_HANDLE) {
		Vulkan::cleanupImage(device, msaaImage);
		msaaImage.image = VK_NULL_HANDLE;
	}
	if (depthImage.image != VK_NULL_HANDLE) {
		Vulkan::cleanupImage(device, depthImage);
		depthImage.image = VK_NULL_HANDLE;
	}
	if (renderPass != VK_NULL_HANDLE) {
		Vulkan::cleanupRenderPass(device, renderPass);
		renderPass = VK_NULL_HANDLE;
	}
	if (swapchainImageViews.size()) {
		Vulkan::cleanupSwapchainImageViews(device, swapchainImageViews);
		swapchainImageViews.clear();
	}
}

u32 VulkSwapchain::NextFrame(u64 timeout) {
	currFrame = (currFrame + 1) % maxFrames;
	currFence = fences[currFrame];
	vkWaitForFences(device, 1, &currFence, VK_TRUE, timeout);
	vkResetFences(device, 1, &currFence);
	return currFrame;
}

void VulkSwapchain::Resize(u32 width, u32 height) {
	flags.clientWidth = width;
	flags.clientHeight = height;
	flags.clearValues = clearValues.data();
	flags.clearValueCount = (u32)clearValues.size();
	Destroy();
	Create(device,surface,graphicsQueue,presentQueue,memoryProperties,flags);
}

VkCommandBuffer VulkSwapchain::BeginRender(bool startRenderPass) {
	
	

	VkResult res = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, presentCompletes[currFrame], nullptr, &index);
	assert(res == VK_SUCCESS);

	VkCommandBuffer cmd = commandBuffers[index];

	vkBeginCommandBuffer(cmd, &beginInfo);

	renderPassBeginInfo.framebuffer = framebuffers[index];
	if (startRenderPass)
		vkCmdBeginRenderPass(cmd, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	return cmd;
}

void VulkSwapchain::StartRenderPass(VkCommandBuffer cmd) {
	vkCmdBeginRenderPass(cmd, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void VulkSwapchain::EndRender(VkCommandBuffer cmd, bool present) {
	vkCmdEndRenderPass(cmd);

	VkResult res = vkEndCommandBuffer(cmd);
	assert(res == VK_SUCCESS);
	
	submitInfo.pWaitSemaphores = &presentCompletes[currFrame];
	submitInfo.pSignalSemaphores = &renderCompletes[currFrame];
	submitInfo.pCommandBuffers = &cmd;
	res = vkQueueSubmit(graphicsQueue, 1, &submitInfo, currFence);
	assert(res == VK_SUCCESS);
	if (present) {
		
		
		presentInfo.pWaitSemaphores = &renderCompletes[currFrame];
		res = vkQueuePresentKHR(presentQueue, &presentInfo);
		assert(res == VK_SUCCESS);
	}
	frameCount++;

}


VulkState::VulkState() {

}

VulkState::~VulkState() {

}

bool VulkState::Init(VulkStateInitFlags& initFlags, SDL_Window* mainWindow) {
	std::vector<const char*> requiredExtensions{ "VK_KHR_surface" };
	std::vector<const char*> requiredLayers;
	if (initFlags.enableValidation)
		requiredLayers = { "VK_LAYER_KHRONOS_validation" };
#ifdef ENABLE_DEBUG_MARKER 
	requiredExtensions.push_back("VK_EXT_debug_report");
	//requiredLayers.push_back( "VK_LAYER_RENDERDOC_Capture" );
#endif
	requiredLayers.push_back("VK_LAYER_LUNARG_monitor");
	unsigned count = 0;
	SDL_Vulkan_GetInstanceExtensions(mainWindow, &count, nullptr);
	std::vector<const char*> sdlExt(count);
	SDL_Vulkan_GetInstanceExtensions(mainWindow, &count, sdlExt.data());
	for (auto& ext : sdlExt) {
		auto first = requiredExtensions.cbegin();
		auto last = requiredExtensions.cend();
		bool found = false;
		while (first != last) {
			if (_strcmpi(*first, ext) == 0)
				found = true;
			++first;
		}
		if (!found) {
			requiredExtensions.push_back(ext);
		}
	}


	instance = Vulkan::initInstance(requiredExtensions, requiredLayers);

	assert(SDL_Vulkan_CreateSurface(mainWindow, instance, &surface));

#ifdef _WIN32
	if (surface == nullptr) {
		//create it ourselves
		SDL_SysWMinfo wmInfo;
		SDL_VERSION(&wmInfo.version);
		SDL_GetWindowWMInfo(mainWindow, &wmInfo);
		HWND hwnd = wmInfo.info.win.window;
		surface = Vulkan::initSurface(instance, GetModuleHandle(nullptr), hwnd);

	}
#endif	
	assert(surface);
	physicalDevice = choosePhysicalDevice(instance, surface, queues);
	vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &deviceMemoryProperties);
	vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCaps);
	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
	surfaceFormats.resize(formatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, surfaceFormats.data());
	uint32_t presentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
	presentModes.resize(presentModeCount);
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data());

	numSamples = Vulkan::getMaxUsableSampleCount(deviceProperties);
	std::vector<const char*> deviceExtensions;
	if (initFlags.enableSwapchain) {
		deviceExtensions.push_back( "VK_KHR_swapchain" );
	}
#ifdef ENABLE_DEBUG_MARKER
	if (Vulkan::supportsDeviceExtension(physicalDevice, VK_EXT_DEBUG_MARKER_EXTENSION_NAME))
		deviceExtensions.push_back(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
	//if (Vulkan::supportsDeviceExtension(mPhysicalDevice, "VK_EXT_debug_utils"))
		//deviceExtensions.push_back("VK_EXT_debug_utils");
#endif


	VkPhysicalDeviceFeatures enabledFeatures{};
	if (deviceFeatures.samplerAnisotropy)
		enabledFeatures.samplerAnisotropy = VK_TRUE;
	if (deviceFeatures.sampleRateShading)
		enabledFeatures.sampleRateShading = VK_TRUE;
	if (initFlags.enableLineWidth && deviceFeatures.wideLines) {
		enabledFeatures.wideLines = VK_TRUE;
	}

	if (initFlags.enableGeometryShader && deviceFeatures.geometryShader)
		enabledFeatures.geometryShader = VK_TRUE;
	if (initFlags.enableWireframe && deviceFeatures.fillModeNonSolid) {
		enabledFeatures.fillModeNonSolid = VK_TRUE;
	}

	uint32_t queueCount = 2;
	device = initDevice(physicalDevice, deviceExtensions, queues, enabledFeatures, queueCount);
#ifdef ENABLE_DEBUG_MARKER
	MARKER_SETUP(device);
#endif
	graphicsQueue = Vulkan::getDeviceQueue(device, queues.graphicsQueueFamily);
	presentQueue = Vulkan::getDeviceQueue(device, queues.presentQueueFamily);
	computeQueue = Vulkan::getDeviceQueue(device, queues.computeQueueFamily);

	backQueue = Vulkan::getDeviceQueue(device, queues.graphicsQueueFamily, 1);

	presentMode = Vulkan::chooseSwapchainPresentMode(presentModes);
	swapchainFormat = Vulkan::chooseSwapchainFormat(surfaceFormats);
	vkGetPhysicalDeviceFormatProperties(physicalDevice, swapchainFormat.format, &formatProperties);

	descriptorSetPoolCache = std::make_unique<DescriptorSetPoolCache>(device);
	descriptorSetLayoutCache = std::make_unique<DescriptorSetLayoutCache>(device);

	commandPool = Vulkan::initCommandPool(device, queues.graphicsQueueFamily);
	commandBuffer = Vulkan::initCommandBuffer(device, commandPool);

	return true;
}


void VulkState::Cleanup() {
	vkDeviceWaitIdle(device);

	descriptorSetLayoutCache.reset();
	descriptorSetPoolCache.reset();
	Vulkan::cleanupCommandBuffer(device, commandPool, commandBuffer);
	Vulkan::cleanupCommandPool(device, commandPool);
	Vulkan::cleanupDevice(device);
	Vulkan::cleanupSurface(instance, surface);
	Vulkan::cleanupInstance(instance);
}