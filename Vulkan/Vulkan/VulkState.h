#pragma once
struct VulkStateInitFlags {
	bool        enableGeometryShader{ false };

	bool		enableWireframe{ false };
#ifdef _DEBUG
	bool		enableValidation{ true };
#else
	bool		enableValidation{ false };
#endif
	bool		enableLineWidth{ false };
	bool		enableSwapchain{ true };
};
//#define DEFAULT_SCREEN_WIDTH 800
//#define DEFAULT_SCREEN_HEIGHT 600
struct VulkSwapchainFlags {
	u32									clientWidth{ DEFAULT_SCREEN_WIDTH };
	u32									clientHeight{ DEFAULT_SCREEN_HEIGHT };
	VkSurfaceCapabilitiesKHR			surfaceCaps = {};
	VkPresentModeKHR					presentMode{ VK_PRESENT_MODE_MAILBOX_KHR};
	VkSurfaceFormatKHR					format{ PREFERRED_FORMAT };
	u32									imageCount{ 2 };
	u32									graphicsQueueFamily{ 0 };
	VkSampleCountFlagBits				samples{ VK_SAMPLE_COUNT_1_BIT };
	bool								enableDepthBuffer{ true };
	VkFormat							depthFormat{ VK_FORMAT_D32_SFLOAT };
	VkImageUsageFlags					depthImageUsage{ 0 };
	bool								enableMSAA{ false };
	VkClearValue* clearValues;
	u32									clearValueCount{ 0 };
};
class VulkSwapchain {
	VulkSwapchainFlags					flags;
	VkDevice							device{ VK_NULL_HANDLE };
	VkSurfaceKHR						surface{ VK_NULL_HANDLE };
	VkPhysicalDeviceMemoryProperties	memoryProperties;
	VkSwapchainKHR						swapchain{ VK_NULL_HANDLE };
	VkQueue								graphicsQueue;
	VkQueue								presentQueue;
	VkPresentModeKHR					presentMode;
	VkSurfaceFormatKHR					swapchainFormat;
	VkFormatProperties					formatProperties;
	VkExtent2D							swapchainExtent;
	std::vector<VkImage>                swapchainImages;
	std::vector<VkImageView>            swapchainImageViews;
	std::vector<VkSemaphore>            presentCompletes;
	std::vector<VkSemaphore>            renderCompletes;
	std::vector<VkFence>                fences;
	VkFence								currFence{ VK_NULL_HANDLE };
	std::vector<VkCommandPool>          commandPools;
	std::vector<VkCommandBuffer>		commandBuffers;
	VkRenderPass                        renderPass{ VK_NULL_HANDLE };
	Vulkan::Image                       depthImage;
	Vulkan::Image                       msaaImage;
	std::vector<VkFramebuffer>          framebuffers;
	VkCommandBufferBeginInfo            beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	VkRenderPassBeginInfo               renderPassBeginInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	VkPipelineStageFlags                submitPipelineStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo		                submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
	VkPresentInfoKHR                    presentInfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	u32		                            index = 0;
	u32		                            frameCount = 0;
	u32		                            currFrame{ (uint32_t)(-1) };
	u32		                            maxFrames{ 0 };
	std::vector<VkClearValue>			clearValues;
public:
	VulkSwapchain();
	void operator=(VulkSwapchain const&) = delete;
	~VulkSwapchain();
	void Create(VkDevice device, VkSurfaceKHR surface, VkQueue graphicsQueue, VkQueue presentQueue, VkPhysicalDeviceMemoryProperties memoryProperties, VulkSwapchainFlags flags);
	void Destroy();
	operator VkDevice()const { return device; }
	operator VkSwapchainKHR()const { return swapchain; }
	VkDevice getDevice()const { return device; }
	VkSwapchainKHR getSwapchain()const { return swapchain; }

	u32	NextFrame(u64 timeout = UINT64_MAX);
	VkCommandBuffer BeginRender(bool startRenderPass = true);
	void EndRender(VkCommandBuffer cmd, bool present = true);
	void StartRenderPass(VkCommandBuffer);
	void Resize(u32 width, u32 height);
	VkRenderPass getRenderPass()const { return renderPass; }
	u32 getFrameCount()const { return frameCount; }
};
struct VulkContext {
	VkDevice device;
	VkQueue	queue;
	VkCommandBuffer commandBuffer;
	VkPhysicalDeviceProperties deviceProperties;
	VkPhysicalDeviceMemoryProperties memoryProperties;

	DescriptorSetPoolCache* pPoolCache;
	DescriptorSetLayoutCache* pLayoutCache;
};
class VulkState {
	VulkStateInitFlags					initFlags;
	VkInstance							instance{ VK_NULL_HANDLE };
	VkSurfaceKHR						surface{ VK_NULL_HANDLE };
	VkPhysicalDevice					physicalDevice{ VK_NULL_HANDLE };
	Vulkan::Queues						queues;
	VkQueue								graphicsQueue{ VK_NULL_HANDLE };
	VkQueue								presentQueue{ VK_NULL_HANDLE };
	VkQueue								computeQueue{ VK_NULL_HANDLE };
	VkQueue								backQueue{ VK_NULL_HANDLE };
	VkPhysicalDeviceProperties			deviceProperties;
	VkPhysicalDeviceMemoryProperties	deviceMemoryProperties;
	VkPhysicalDeviceFeatures			deviceFeatures;
	VkSurfaceCapabilitiesKHR			surfaceCaps{};
	std::vector<VkSurfaceFormatKHR>		surfaceFormats;
	std::vector<VkPresentModeKHR>		presentModes;
	VkSampleCountFlagBits				numSamples{ VK_SAMPLE_COUNT_1_BIT };
	VkDevice							device{ VK_NULL_HANDLE };
	VkPresentModeKHR					presentMode{  };
	VkSurfaceFormatKHR					swapchainFormat{};
	VkFormatProperties					formatProperties{};
	VkCommandPool						commandPool{ VK_NULL_HANDLE };
	VkCommandBuffer						commandBuffer{ VK_NULL_HANDLE };
	/*VkExtent2D							swapchainExtent{};
	VkSwapchainKHR						swapchain{ VK_NULL_HANDLE };
	std::vector<VkImage>				swapchainImages;
	std::vector<VkImageView>			swapchainImageViews;
	std::vector<VkSemaphore>			presentCompletes;
	std::vector<VkSemaphore>			renderCompletes;
	std::vector<VkFence>				fences;
	VkFence								currFence{ VK_NULL_HANDLE };
	VkCommandPool						commandPool{ VK_NULL_HANDLE };
	VkCommandBuffer						commandBuffer{ VK_NULL_HANDLE };
	VkCommandBuffer						currCommandBuffer{ VK_NULL_HANDLE };
	std::vector<VkCommandPool>			commandPools;
	std::vector<VkCommandBuffer>		commandBuffers;
	VkFormat							depthFormat{ VK_FORMAT_D32_SFLOAT };
	VkImageUsageFlags					depthImageUsage{ VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT };
	VkRenderPass						renderPass{ VK_NULL_HANDLE };
	Vulkan::Image						depthImage;
	Vulkan::Image						msaaImage;
	std::vector<VkFramebuffer>			framebuffers;*/
	std::unique_ptr<DescriptorSetLayoutCache> descriptorSetLayoutCache;
	std::unique_ptr<DescriptorSetPoolCache> descriptorSetPoolCache;
public:
	VulkState();
	void operator=(VulkState const&) = delete;
	~VulkState();
	bool Init(VulkStateInitFlags& initFlags, SDL_Window* mainWindow);
	void Cleanup();
	void Resize(uint32_t width, uint32_t height);



	VkInstance							getInstance()const { return instance; }
	VkSurfaceKHR						getSurface()const { return surface; }
	VkPhysicalDevice					getPhysicalDevice()const { return physicalDevice; }
	u32									getGraphicsQueueFamily()const { return queues.graphicsQueueFamily; }
	u32									getPresentQueueFamily()const { return queues.presentQueueFamily; }
	u32									getComputeQueueFamily()const { return queues.computeQueueFamily; }
	VkQueue								getGraphicsQueue()const { return graphicsQueue; }
	VkQueue								getPresentQueue()const { return presentQueue; }
	VkQueue								getComputeQueue()const { return computeQueue; }
	VkPhysicalDeviceProperties			getPhysicalDeviceProperties()const { return deviceProperties; }
	VkPhysicalDeviceMemoryProperties	getPhysicalDeviceMemoryProperties()const { return deviceMemoryProperties; }
	VkPhysicalDeviceFeatures			getPhysicalDeviceFeatures()const { return deviceFeatures; }
	VkSurfaceCapabilitiesKHR			getSurfaceCapabilities()const { return surfaceCaps; }
	VkSampleCountFlagBits				getSampleCount()const { return numSamples; }
	VkDevice							getDevice()const { return device; }
	VulkContext							getContext()const { return { device,backQueue,commandBuffer,deviceProperties,deviceMemoryProperties,descriptorSetPoolCache.get(),descriptorSetLayoutCache.get() }; }
	/*VkPresentModeKHR					getPresentMode()const { return presentMode; }
	VkSurfaceFormatKHR					getSwapchainFormat()const { return swapchainFormat; }
	VkFormatProperties					getFormatProperties()const { return formatProperties; }
	VkExtent2D							getSwapchainExtent()const { return swapchainExtent; }*/

};

extern VulkState vulkState;