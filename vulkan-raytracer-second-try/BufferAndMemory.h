#pragma once
#include <vulkan/vulkan.h>
class BufferAndMemory {
public:
	BufferAndMemory();
	~BufferAndMemory();
	VkBuffer buffer = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
	VkDeviceSize size = 0;
	void CreateBuffer(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, VkDeviceSize bufferSize, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryProperties);
	void AddData(VkDevice device, const void* data, size_t size);
	void Cleanup(VkDevice logicalDevice);


};