#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "VulkanPhysicalDevices.h"
#include <iostream>
#include "VulkanUtils.h"
#include <cassert>
#include "VulkanQueue.h"
using namespace std;
namespace my_vulkan
{

	void BeginCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferUsageFlags usageFlags);
	class BufferAndMemory {
		public:
		VkBuffer buffer = VK_NULL_HANDLE;
		VkDeviceMemory memory = VK_NULL_HANDLE;
		VkDeviceSize size = 0;
		void AddData(VkDevice device, const void* data, size_t size);
		
		void Cleanup(VkDevice logicalDevice);

		
	};
	class VulkanCore
	{
		public:
			VulkanCore();
			~VulkanCore();
			void Init(const char* AppName,GLFWwindow* window);
			void Cleanup();
			int GetNumSwapChainImages() const { return (int)swapChainImages.size(); }
			VkImage GetSwapChainImage(uint32_t index) const { return swapChainImages[index]; }
			VkImageView GetSwapChainImageView(uint32_t index) const { return swapChainImageViews[index]; }
			VulkanQueue* GetQueue() { return &queue; }
			uint32_t GetQueueFamilyIndex() const { return (uint32_t)queueFamilyIndex; }
			VkDevice GetLogicalDevice() const { return logicalDevice; }
			VkCommandPool GetCommandPool() const { return commandPool; }
			VkPhysicalDevice GetPhysicalDevice() { return physicalDevices.GetSelectedPhysicalDevice().device; }
			VkExtent2D surfaceExtent;
		private:
			VkInstance instance = VK_NULL_HANDLE;
			VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
			VkSurfaceKHR surface = VK_NULL_HANDLE;
			VulkanPhysicalDevices physicalDevices;
			int queueFamilyIndex = -1;
			VkDevice logicalDevice = VK_NULL_HANDLE;
			VkSwapchainKHR swapChain = VK_NULL_HANDLE;
			vector<VkImage> swapChainImages;
			vector<VkImageView> swapChainImageViews;
			VkCommandPool commandPool = VK_NULL_HANDLE;
			VulkanQueue queue;
			vector<const char*> Layers = {
				"VK_LAYER_KHRONOS_validation"
			};
			vector <const char*> Extensions = {
				VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
				VK_KHR_SURFACE_EXTENSION_NAME,
				"VK_KHR_win32_surface",
			};
			void CreateInstance(const char* AppName);
			void CreateDebugCallback();
			void CreateSurface(GLFWwindow* window);
			void CreatePhysicalDevices();
			void CreateLogicalDevice();
			void CreateSwapChain();
			void CreateCommandBufferPool();
			

	};
}