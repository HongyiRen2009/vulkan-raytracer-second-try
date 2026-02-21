#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <iostream>
namespace my_vulkan
{
	struct PhysicalDevice {
		VkPhysicalDevice device;
		VkPhysicalDeviceProperties deviceProperties;
		VkPhysicalDeviceFeatures deviceFeatures;
		std::vector<VkQueueFamilyProperties> queueFamilyProperties;
		std::vector<VkBool32> queueFamilySupportsPresenting;
		std::vector<VkSurfaceFormatKHR> surfaceFormats;
		VkSurfaceCapabilitiesKHR surfaceCapabilities;
		VkPhysicalDeviceMemoryProperties memoryProperties;
		std::vector<VkPresentModeKHR> presentingModes;
	};
	class VulkanPhysicalDevices
	{
		public:
			VulkanPhysicalDevices();
			~VulkanPhysicalDevices();
			void init(VkInstance instance, VkSurfaceKHR surface);
			/*
			Gets the currently selected physical device. Must be called after SelectPhysicalDevice() has been called successfully and throws an error otherwise.
			*/
			PhysicalDevice& GetSelectedPhysicalDevice();
			/*
			Selects a physical device that supports the required queue type and optionally supports presenting to the surface.
			@param RequiredQueueType: The required queue type (e.g., VK_QUEUE_GRAPHICS_BIT).
			@param SupportsPresenting: If true, the selected device must support presenting to the surface.
			@return The index of the selected physical device's queue family that meets the criteria.
			*/
			uint32_t SelectPhysicalDevice(VkQueueFlags RequiredQueueType, bool SupportsPresenting);
		private:
			std::vector<PhysicalDevice> physicalDevices;
			int selectedPhysicalDeviceIndex = -1;
	};
}