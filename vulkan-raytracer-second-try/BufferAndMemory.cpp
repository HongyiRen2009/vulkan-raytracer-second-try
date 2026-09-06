#include "BufferAndMemory.h"
#include "VulkanUtils.h"
BufferAndMemory::BufferAndMemory() {
}
BufferAndMemory::~BufferAndMemory() {
}
uint32_t FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}
	throw std::runtime_error("Failed to find suitable memory type!");
}
void BufferAndMemory::AddData(VkDevice device, const void* data, size_t size,VkDeviceSize offset) {
	memcpy(static_cast<char*>(mappedData) + offset, data, size);
}
void BufferAndMemory::CreateBuffer(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, VkDeviceSize bufferSize, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryProperties) {
	size = bufferSize;
	VkBufferCreateInfo bufferCreateInfo{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.size = bufferSize,
		.usage = usageFlags,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	};
	VkResult result = vkCreateBuffer(logicalDevice, &bufferCreateInfo, nullptr, &buffer);
	CHECK_VK_RESULT(result,"vkCreateBuffer");
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(logicalDevice, buffer, &memRequirements);
	uint32_t memoryTypeIndex = FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, memoryProperties);
	VkMemoryAllocateInfo allocInfo{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = nullptr,
		.allocationSize = memRequirements.size,
		.memoryTypeIndex = memoryTypeIndex,
	};
	result = vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &memory);
	CHECK_VK_RESULT(result,"vkAllocateMemory");
	result = vkBindBufferMemory(logicalDevice, buffer, memory, 0);
	CHECK_VK_RESULT(result,"vkBindBufferMemory");

	// Device-local buffers are populated through a staging copy and cannot be
	// mapped on discrete GPUs. Keep host-visible buffers persistently mapped.
	if ((memoryProperties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0) {
		result = vkMapMemory(logicalDevice, memory, 0, bufferSize, 0, &mappedData);
		CHECK_VK_RESULT(result, "vkMapMemory");
		memset(mappedData, -1, bufferSize);
	}

	
}
void BufferAndMemory::Cleanup(VkDevice logicalDevice) {
	if (buffer != VK_NULL_HANDLE) {
		vkDestroyBuffer(logicalDevice, buffer, nullptr);
		buffer = VK_NULL_HANDLE;
	}
	if (memory != VK_NULL_HANDLE) {
		if (mappedData != nullptr) {
			vkUnmapMemory(logicalDevice, memory);
			mappedData = nullptr;
		}
		vkFreeMemory(logicalDevice, memory, nullptr);
		memory = VK_NULL_HANDLE;
	}
	size = 0;
}
