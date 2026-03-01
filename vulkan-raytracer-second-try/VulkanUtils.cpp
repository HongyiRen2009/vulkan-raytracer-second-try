#include "VulkanUtils.h"
namespace my_vulkan
{
	 VkShaderModule VulkanUtils::CreateShaderModule(VkDevice device, const std::string& filename) {
		std::vector<char> code = readFile(filename);
		VkShaderModuleCreateInfo createInfo{
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.codeSize = code.size(),
			.pCode = reinterpret_cast<const uint32_t*>(code.data()),
		};
		VkShaderModule shaderModule;
		VkResult result = vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule);
		CHECK_VK_RESULT(result, "vkCreateShaderModule");
		return shaderModule;
	}
	 std::vector<char> VulkanUtils:: readFile(const std::string& filename) {
		std::ifstream file(filename, std::ios::ate | std::ios::binary);

		if (!file.is_open()) {
			throw std::runtime_error("failed to open file!");
		}
		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);
		file.seekg(0);
		file.read(buffer.data(), fileSize);
		file.close();

		return buffer;
	}
	 void VulkanUtils::BeginCommandBuffer(VkCommandBuffer CommandBuffer, VkCommandBufferUsageFlags usageFlags) {
		 VkCommandBufferBeginInfo beginInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.pNext = nullptr,
			.flags = usageFlags,
			.pInheritanceInfo = nullptr
		 };
		 VkResult result = vkBeginCommandBuffer(CommandBuffer,&beginInfo);
		 CHECK_VK_RESULT(result,"vkBeginCommandBuffer");
	}
}