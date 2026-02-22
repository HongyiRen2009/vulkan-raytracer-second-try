#include "VulkanCore.h"

namespace my_vulkan
{
	VulkanCore::VulkanCore() {
	}
	VulkanCore::~VulkanCore() {
	}

	static string GetMessageSeverityString(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity) {
		string result;
		if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
			result += "VERBOSE ";
		}
		if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
			result += "WARNING ";
		}
		if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
			result += "ERROR ";
		}
		return result;
	}
	static string GetMessageTypeString(VkDebugUtilsMessageTypeFlagsEXT messageType) {
		string result;
		if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) {
			result += "GENERAL ";
		}
		if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
			result += "VALIDATION ";
		}
		if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) {
			result += "PERFORMANCE ";
		}
		return result;
	}
	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageTypes,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData) {
		cout <<"Validation layer: "<< pCallbackData->pMessage <<endl;
		cout << "Severity: " << GetMessageSeverityString(messageSeverity)<<endl;
		cout << "Type: " << GetMessageTypeString(messageTypes)<<endl;
		for (uint32_t i = 0;i < pCallbackData->objectCount;i++) {
			cout << "Object " << i << ": Type=" << pCallbackData->pObjects[i].objectType << ", Handle=" << pCallbackData->pObjects[i].objectHandle << ", Name=" << (pCallbackData->pObjects[i].pObjectName ? pCallbackData->pObjects[i].pObjectName : "N/A") << endl;
		}
		return VK_FALSE;
	}
	static uint32_t ChooseSwapImageCount(const VkSurfaceCapabilitiesKHR& surfaceCaps) {
		uint32_t imageCount = surfaceCaps.minImageCount + 1;
		if (surfaceCaps.maxImageCount > 0 && imageCount > surfaceCaps.maxImageCount) {
			imageCount = surfaceCaps.maxImageCount;
		}
		return imageCount;
	}
	static VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
		for (const auto& availablePresentMode : availablePresentModes) {
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				return availablePresentMode;
			}
		}
		return VK_PRESENT_MODE_FIFO_KHR;
	}
	static VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
		for (const auto& availableFormat : availableFormats) {
			if (availableFormat.format == VK_FORMAT_R16G16B16_SFLOAT && availableFormat.colorSpace == VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT) {
				return availableFormat;
			}
		}
		return availableFormats[0];
	}
	void BeginCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferUsageFlags usageFlags) {
		VkCommandBufferBeginInfo beginInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = usageFlags,
		.pInheritanceInfo = nullptr,
		};
		VkResult result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
		CHECK_VK_RESULT(result,"vkBeginCommandBuffer");

	}
	void VulkanCore::Init(const char* AppName,GLFWwindow* window) {

		CreateInstance(AppName);
		CreateDebugCallback();
		CreateSurface(window);
		CreatePhysicalDevices();
		CreateLogicalDevice();
		CreateSwapChain();
		CreateCommandBufferPool();
		queue.Init(logicalDevice, swapChain, queueFamilyIndex, queueFamilyIndex);
	}
	void VulkanCore::CreateInstance(const char* AppName) {
		VkApplicationInfo appInfo{
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pNext = nullptr,
		.pApplicationName = AppName,
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName = "No Engine",
		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion = VK_API_VERSION_1_3,

		};
		VkInstanceCreateInfo createInfo{
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.pApplicationInfo = &appInfo,
		.enabledLayerCount = (uint32_t)Layers.size(),
		.ppEnabledLayerNames = Layers.data(),
		.enabledExtensionCount = (uint32_t)Extensions.size(),
		.ppEnabledExtensionNames = Extensions.data(),
		};
		VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
		CHECK_VK_RESULT(result,"vkCreateInstance");
		std::cout << "Vulkan instance created successfully!" << std::endl;


	}
	void VulkanCore::CreateDebugCallback() {
		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
		.pNext = nullptr,
		.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
		.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
		.pfnUserCallback = &DebugCallback,
		.pUserData = nullptr,
		};
		PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
		if (!vkCreateDebugUtilsMessengerEXT) {
			throw std::runtime_error("Failed to load vkCreateDebugUtilsMessengerEXT");
		}
		VkResult result = vkCreateDebugUtilsMessengerEXT(instance, &debugCreateInfo, nullptr, &debugMessenger);
		CHECK_VK_RESULT(result,"vkCreateDebugUtilsMessengerEXT");
		std::cout << "Debug callback created successfully!" << std::endl;
	}

	void VulkanCore::CreateSurface(GLFWwindow* window) {
		VkResult result = glfwCreateWindowSurface(instance, window, nullptr, &surface);
		CHECK_VK_RESULT(result,"glfwCreateWindowSurface");
		std::cout << "Window surface created successfully!" << std::endl;
	}
	void VulkanCore::CreatePhysicalDevices() {
		physicalDevices.init(instance, surface);
		queueFamilyIndex = physicalDevices.SelectPhysicalDevice(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT, true);
		surfaceExtent = physicalDevices.GetSelectedPhysicalDevice().surfaceCapabilities.currentExtent;
	}

	void VulkanCore::CreateLogicalDevice() {
		// Determines the priority of each queue in a single family from 0.0 to 1.0, with higher values indicating higher priority. The behavior of the device when multiple queues have the same priority is implementation-defined, but it is guaranteed to be fair and will not starve any queue. If there are multiple queues in a family and they have different priorities, the implementation may schedule work on the queues in any order, but it will attempt to schedule work on higher-priority queues more frequently than lower-priority queues.
		float queuePriorities[] = { 1.0f };
		VkDeviceQueueCreateInfo queueCreateInfo{
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.queueFamilyIndex = (uint32_t)queueFamilyIndex,
		.queueCount = 1,
		.pQueuePriorities = queuePriorities,
		};
		vector<const char*> deviceExtensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
			VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME
		};
		// Enabled device features that are indicated with a bitmask. Things like multidrawindirect, geometry shader, tessellation shader, etc. are all features that can be enabled here.
		const VkPhysicalDeviceFeatures deviceFeatures = {0};

		// Example of how to query for a specific feature
		//if (physicalDevices.GetSelectedPhysicalDevice().deviceFeatures.samplerAnisotropy==VK_FALSE) {
		//	cerr << "Selected physical device does not support sampler anisotropy, which is required for this application." << endl;
		//}
		VkDeviceCreateInfo deviceCreateInfo{
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &queueCreateInfo,
		.enabledExtensionCount = (uint32_t)deviceExtensions.size(),
		.ppEnabledExtensionNames = deviceExtensions.data(),
		.pEnabledFeatures = &deviceFeatures,
		};
		VkResult result = vkCreateDevice(physicalDevices.GetSelectedPhysicalDevice().device, &deviceCreateInfo, nullptr, &logicalDevice);
		CHECK_VK_RESULT(result,"vkCreateDevice");
		std::cout << "Logical device created successfully!" << std::endl;

	}
	void VulkanCore::CreateSwapChain() {
		const VkSurfaceCapabilitiesKHR& surfaceCaps = physicalDevices.GetSelectedPhysicalDevice().surfaceCapabilities;
		uint32_t imageCount = ChooseSwapImageCount(surfaceCaps);
		const std::vector<VkPresentModeKHR>& presentModes = physicalDevices.GetSelectedPhysicalDevice().presentingModes;
		VkPresentModeKHR presentMode = ChooseSwapPresentMode(presentModes);
		const std::vector<VkSurfaceFormatKHR>& surfaceFormats = physicalDevices.GetSelectedPhysicalDevice().surfaceFormats;
		VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(surfaceFormats);
		VkSwapchainCreateInfoKHR swapChainCreateInfo{
			.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			.pNext = nullptr,
			.flags = 0,
			.surface = surface,
			.minImageCount = imageCount,
			.imageFormat = surfaceFormat.format,
			.imageColorSpace = surfaceFormat.colorSpace,
			.imageExtent = surfaceCaps.currentExtent,
			.imageArrayLayers = 1,
			.imageUsage = (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT),
			.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount = 1,
			.pQueueFamilyIndices = (uint32_t*)&queueFamilyIndex,
			.preTransform = surfaceCaps.currentTransform,
			.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			.presentMode = presentMode,
			.clipped = VK_TRUE,
			};
		VkResult result = vkCreateSwapchainKHR(logicalDevice, &swapChainCreateInfo, nullptr, &swapChain);
		CHECK_VK_RESULT(result,"vkCreateSwapchainKHR");
		std::cout << "Swap chain created successfully!" << std::endl;
		uint32_t swapChainImageCount = 0;
		result = vkGetSwapchainImagesKHR(logicalDevice, swapChain, &swapChainImageCount, nullptr);
		CHECK_VK_RESULT(result,"vkGetSwapchainImagesKHR");
		assert(imageCount == swapChainImageCount);
		swapChainImages.resize(swapChainImageCount);
		swapChainImageViews.resize(swapChainImageCount);
		result = vkGetSwapchainImagesKHR(logicalDevice, swapChain, &swapChainImageCount, swapChainImages.data());
		CHECK_VK_RESULT(result,"vkGetSwapchainImagesKHR");
		int LayerCount = 1;
		uint32_t MipLevels = 1;
		for (uint32_t i = 0;i < swapChainImageCount;i++) {
			VkImageViewCreateInfo imageViewCreateInfo{
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.image = swapChainImages[i],
				.viewType = VK_IMAGE_VIEW_TYPE_2D,
				.format = surfaceFormat.format,
				.components = {
					.r = VK_COMPONENT_SWIZZLE_IDENTITY,
					.g = VK_COMPONENT_SWIZZLE_IDENTITY,
					.b = VK_COMPONENT_SWIZZLE_IDENTITY,
					.a = VK_COMPONENT_SWIZZLE_IDENTITY,
				},
				.subresourceRange = {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseMipLevel = 0,
					.levelCount = MipLevels,
					.baseArrayLayer = 0,
					.layerCount = 1,
				},
			};
			result = vkCreateImageView(logicalDevice, &imageViewCreateInfo, nullptr, &swapChainImageViews[i]);
			CHECK_VK_RESULT(result,"vkCreateImageView");
		}

	}
	void VulkanCore::CreateCommandBufferPool() {
		VkCommandPoolCreateInfo poolCreateInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = (uint32_t)queueFamilyIndex,
		};
		VkResult result = vkCreateCommandPool(logicalDevice, &poolCreateInfo, nullptr, &commandPool);
		CHECK_VK_RESULT(result,"vkCreateCommandPool");
		cout << "Command buffer pool created successfully!" << std::endl;
	}
	void VulkanCore::RecreateSwapChain() {
		vkDeviceWaitIdle(logicalDevice);
		for (uint32_t i = 0;i < swapChainImageViews.size();i++) {
			vkDestroyImageView(logicalDevice, swapChainImageViews[i], nullptr);
		}
		swapChainImageViews.clear();
		physicalDevices.UpdateSurfaceFormats(surface);
		surfaceExtent = physicalDevices.GetSelectedPhysicalDevice().surfaceCapabilities.currentExtent;
		vkDestroySwapchainKHR(logicalDevice, swapChain, nullptr);
		swapChain = VK_NULL_HANDLE;

		CreateSwapChain();
	}
	void VulkanCore::Cleanup() {
		queue.WaitIdle();
		if (commandPool != VK_NULL_HANDLE) {
			vkDestroyCommandPool(logicalDevice, commandPool, nullptr);
			cout << "Command pool destroyed successfully!" << std::endl;
		}
		if (debugMessenger != VK_NULL_HANDLE) {
			PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
			if (vkDestroyDebugUtilsMessengerEXT) {
				vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
				cout << "Debug callback destroyed successfully!" << std::endl;
			}
			else {
				std::cerr << "Failed to load vkDestroyDebugUtilsMessengerEXT" << std::endl;
			}
		}

		if (swapChain != VK_NULL_HANDLE) {
			vkDestroySwapchainKHR(logicalDevice, swapChain, nullptr);
			cout << "Swap chain destroyed successfully!" << std::endl;
		}
		cout << "Physical devices cleaned up successfully!" << std::endl;
		if (logicalDevice != VK_NULL_HANDLE) {
				for (uint32_t i = 0;i < swapChainImageViews.size();i++) {
					vkDestroyImageView(logicalDevice, swapChainImageViews[i], nullptr);
				}
				cout << "Swap chain image views destroyed successfully!" << std::endl;
		}
		if (surface != VK_NULL_HANDLE) {
			PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR = (PFN_vkDestroySurfaceKHR)vkGetInstanceProcAddr(instance, "vkDestroySurfaceKHR");
			if (vkDestroySurfaceKHR) {
				vkDestroySurfaceKHR(instance, surface, nullptr);
				cout << "Window surface destroyed successfully!" << std::endl;
			}
			else {
				std::cerr << "Failed to load vkDestroySurfaceKHR" << std::endl;
			}
		}
		vkDestroyDevice(logicalDevice, nullptr);
		cout << "Logical device destroyed successfully!" << std::endl;
		if (instance != VK_NULL_HANDLE) {
			vkDestroyInstance(instance, nullptr);
			cout << "Vulkan instance destroyed successfully!" << std::endl;
		}

		cout << "Vulkan cleanup successful!" << std::endl;
	}
	void BufferAndMemory::AddData(VkDevice device, const void* data, size_t size) {
		void* mappedData;
		VkResult result = vkMapMemory(device, memory, 0, size, 0, &mappedData);
		CHECK_VK_RESULT(result,"vkMapMemory");
		memcpy(mappedData, data, size);
		vkUnmapMemory(device,memory);
	}
	void BufferAndMemory::Cleanup(VkDevice logicalDevice) {
		if (buffer != VK_NULL_HANDLE) {
			vkDestroyBuffer(logicalDevice, buffer, nullptr);
			buffer = VK_NULL_HANDLE;
		}
		if (memory != VK_NULL_HANDLE) {
			vkFreeMemory(logicalDevice, memory, nullptr);
			memory = VK_NULL_HANDLE;
		}
		size = 0;
	}
}