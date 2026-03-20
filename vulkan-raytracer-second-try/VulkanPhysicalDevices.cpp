#include <vector>
#include "VulkanPhysicalDevices.h"
#include <vulkan/vulkan.h>
#include <cassert>
#include <iostream>
#include "VulkanUtils.h"
using namespace my_vulkan;
using namespace std;
bool VulkanPhysicalDevices::useIntegratedGPU = false;
VulkanPhysicalDevices::VulkanPhysicalDevices(){}
VulkanPhysicalDevices::~VulkanPhysicalDevices() {}
void VulkanPhysicalDevices::init(VkInstance instance, VkSurfaceKHR surface) {
    uint32_t numDevices = 0;
    VkResult result = vkEnumeratePhysicalDevices(instance, &numDevices, nullptr);
    CHECK_VK_RESULT(result,"vkEnumeratePhysicalDevices");
    cout << "Number of physical devices: " << numDevices << endl;
    physicalDevices.resize(numDevices);
    vector<VkPhysicalDevice> devices(numDevices);
    devices.resize(numDevices);
    result = vkEnumeratePhysicalDevices(instance, &numDevices, devices.data());
    CHECK_VK_RESULT(result,"vkEnumeratePhysicalDevices");
    for (uint32_t i = 0; i < numDevices; i++) {
        VkPhysicalDevice device = devices[i];
        physicalDevices[i].device = device;
        vkGetPhysicalDeviceProperties(device, &physicalDevices[i].deviceProperties);

        const auto& props = physicalDevices[i].deviceProperties;
        cout << "Device " << i << ": " << props.deviceName << endl;
        cout << "  Type: " << props.deviceType << endl;
        cout << "  API Version: "
            << VK_VERSION_MAJOR(props.apiVersion) << "."
            << VK_VERSION_MINOR(props.apiVersion) << "."
            << VK_VERSION_PATCH(props.apiVersion) << endl;
        cout << "  Driver Version: " << props.driverVersion << endl;
        cout << "  Vendor ID: " << props.vendorID << ", Device ID: " << props.deviceID << endl;

        uint32_t numQueueFamilies = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &numQueueFamilies, nullptr);
        physicalDevices[i].queueFamilyProperties.resize(numQueueFamilies);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &numQueueFamilies, physicalDevices[i].queueFamilyProperties.data());
        physicalDevices[i].queueFamilySupportsPresenting.resize(numQueueFamilies);
        for (uint32_t j = 0; j < numQueueFamilies; j++) {
            const VkQueueFamilyProperties& queueFamily = physicalDevices[i].queueFamilyProperties[j];
            VkQueueFlags queueFlags = queueFamily.queueFlags;
            result = vkGetPhysicalDeviceSurfaceSupportKHR(device, j, surface, &physicalDevices[i].queueFamilySupportsPresenting[j]);
            CHECK_VK_RESULT(result,"vkGetPhysicalDeviceSurfaceSupportKHR");

            cout << "    Queue Family " << j << ": " << endl;
            cout << "      Queue Count: " << queueFamily.queueCount << endl;
            cout << "      Queue Flags: " << queueFlags << endl;
			cout << "      Supports Presenting: " << (physicalDevices[i].queueFamilySupportsPresenting[j] ? "Yes" : "No") << endl;
			VkQueueFlags flags = queueFamily.queueFlags;
			cout << "      Supports Graphics: " << ((flags & VK_QUEUE_GRAPHICS_BIT) ? "Yes" : "No") 
            << "      Supports Compute: " << ((flags & VK_QUEUE_COMPUTE_BIT) ? "Yes" : "No") 
            << "      Supports Transfer: " << ((flags & VK_QUEUE_TRANSFER_BIT) ? "Yes" : "No") 
            << "      Supports Sparse Binding: " << ((flags & VK_QUEUE_SPARSE_BINDING_BIT) ? "Yes" : "No") <<
            endl;
        }
        uint32_t numSurfaceFormats = 0;
        result = vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &numSurfaceFormats, nullptr);
        CHECK_VK_RESULT(result,"vkGetPhysicalDeviceSurfaceFormatsKHR");
        physicalDevices[i].surfaceFormats.resize(numSurfaceFormats);
        result = vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &numSurfaceFormats, physicalDevices[i].surfaceFormats.data());
        CHECK_VK_RESULT(result,"vkGetPhysicalDeviceSurfaceFormatsKHR");
        for (uint32_t j = 0; j < numSurfaceFormats; j++) {
            const VkSurfaceFormatKHR& surfaceFormat = physicalDevices[i].surfaceFormats[j];
            cout << "    Surface format " << j << ": Format=" << surfaceFormat.format << ", ColorSpace=" << surfaceFormat.colorSpace << endl;
        }
        result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &physicalDevices[i].surfaceCapabilities);
        CHECK_VK_RESULT(result,"vkGetPhysicalDeviceSurfaceCapabilitiesKHR");

        const auto& caps = physicalDevices[i].surfaceCapabilities;
        cout << "  Surface Capabilities:" << endl;
        cout << "    minImageCount: " << caps.minImageCount << endl;
        cout << "    maxImageCount: " << caps.maxImageCount << endl;
        cout << "    currentExtent: " << caps.currentExtent.width << "x" << caps.currentExtent.height << endl;
        cout << "    minImageExtent: " << caps.minImageExtent.width << "x" << caps.minImageExtent.height << endl;
        cout << "    maxImageExtent: " << caps.maxImageExtent.width << "x" << caps.maxImageExtent.height << endl;
        cout << "    supportedTransforms: " << caps.supportedTransforms << endl;
        cout << "    supportedCompositeAlpha: " << caps.supportedCompositeAlpha << endl;
        cout << "    supportedUsageFlags: " << caps.supportedUsageFlags << endl;

        uint32_t numPresentingModes = 0;
        result = vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &numPresentingModes, nullptr);
        CHECK_VK_RESULT(result,"vkGetPhysicalDeviceSurfacePresentModesKHR");
        physicalDevices[i].presentingModes.resize(numPresentingModes);
        result = vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &numPresentingModes, physicalDevices[i].presentingModes.data());
        CHECK_VK_RESULT(result,"vkGetPhysicalDeviceSurfacePresentModesKHR");
        assert(numPresentingModes > 0);

        cout << "  Present Modes: ";
        for (uint32_t j = 0; j < numPresentingModes; j++) {
            cout << physicalDevices[i].presentingModes[j];
            if (j < numPresentingModes - 1) cout << ", ";
        }
        cout << endl;

        vkGetPhysicalDeviceMemoryProperties(device, &physicalDevices[i].memoryProperties);

        const auto& memProps = physicalDevices[i].memoryProperties;
        cout << "  Memory Heaps: " << memProps.memoryHeapCount << endl;
        for (uint32_t j = 0; j < memProps.memoryHeapCount; j++) {
            cout << "    Heap " << j << ": Size=" << memProps.memoryHeaps[j].size / (1024 * 1024) << " MB, Flags=" << memProps.memoryHeaps[j].flags << endl;
        }
        cout << "  Memory Types: " << memProps.memoryTypeCount << endl;
        for (uint32_t j = 0; j < memProps.memoryTypeCount; j++) {
            cout << "    Type " << j << ": HeapIndex=" << memProps.memoryTypes[j].heapIndex << ", PropertyFlags=" << memProps.memoryTypes[j].propertyFlags << endl;
        }
		vkGetPhysicalDeviceFeatures(physicalDevices[i].device, &physicalDevices[i].deviceFeatures);

    }


}
uint32_t VulkanPhysicalDevices::SelectPhysicalDevice(VkQueueFlags RequiredQueueType, bool SupportsPresenting) {
    for (uint32_t i = 0; i < physicalDevices.size(); i++) {
        const auto& device = physicalDevices[i];
        for (uint32_t j = 0; j < device.queueFamilyProperties.size(); j++) {
            const auto& queueFamily = device.queueFamilyProperties[j];
			if (physicalDevices[i].deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                continue;
            }
            if ((queueFamily.queueFlags & RequiredQueueType) == RequiredQueueType) {
                if (!SupportsPresenting || device.queueFamilySupportsPresenting[j]) {
                    selectedPhysicalDeviceIndex = i;
					cout << "Selected physical device " << i << " and queue family " << j << endl;
                    return j;
                }
            }
        }
    }
    throw std::runtime_error("No suitable physical device found");
	return 0;
}
void VulkanPhysicalDevices::UpdateSurfaceFormats(VkSurfaceKHR surface) {
    for (uint32_t i = 0; i < physicalDevices.size(); i++) {
		auto& device = physicalDevices[i];
        VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.device, surface, &device.surfaceCapabilities);
        CHECK_VK_RESULT(result, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");

        uint32_t numSurfaceFormats = 0;
        result = vkGetPhysicalDeviceSurfaceFormatsKHR(device.device, surface, &numSurfaceFormats, nullptr);
        CHECK_VK_RESULT(result, "vkGetPhysicalDeviceSurfaceFormatsKHR");
        device.surfaceFormats.resize(numSurfaceFormats);
        result = vkGetPhysicalDeviceSurfaceFormatsKHR(device.device, surface, &numSurfaceFormats, device.surfaceFormats.data());
        CHECK_VK_RESULT(result, "vkGetPhysicalDeviceSurfaceFormatsKHR");

        uint32_t numPresentingModes = 0;
        result = vkGetPhysicalDeviceSurfacePresentModesKHR(device.device, surface, &numPresentingModes, nullptr);
        CHECK_VK_RESULT(result, "vkGetPhysicalDeviceSurfacePresentModesKHR");
        device.presentingModes.resize(numPresentingModes);
        result = vkGetPhysicalDeviceSurfacePresentModesKHR(device.device, surface, &numPresentingModes, device.presentingModes.data());
        CHECK_VK_RESULT(result, "vkGetPhysicalDeviceSurfacePresentModesKHR");
    }
}
PhysicalDevice& VulkanPhysicalDevices::GetSelectedPhysicalDevice() {
    if (selectedPhysicalDeviceIndex < 0) {
        throw std::runtime_error("No physical device selected");
    }
    return physicalDevices[selectedPhysicalDeviceIndex];
}