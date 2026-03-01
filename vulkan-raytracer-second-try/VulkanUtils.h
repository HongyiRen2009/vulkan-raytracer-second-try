#pragma once
#include <stdexcept>
#include <string>
#include <vulkan/vulkan.h>
#include <vector>
#include <fstream>
namespace my_vulkan
{
	#define CHECK_VK_RESULT(result,operationName) if (result != VK_SUCCESS) { throw std::runtime_error("Vulkan error: " + std::to_string(result) +" in operation "+operationName); }
	static class VulkanUtils
	{
	public:
			
				static VkShaderModule CreateShaderModule(VkDevice device, const std::string& filename);
				static void BeginCommandBuffer(VkCommandBuffer CommandBuffer,VkCommandBufferUsageFlags usageFlags);
	private:
				static std::vector<char> readFile(const std::string& filename);
	};
}
