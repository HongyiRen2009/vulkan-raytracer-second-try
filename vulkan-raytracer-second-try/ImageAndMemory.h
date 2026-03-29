#include <vulkan/vulkan.h>
class ImageAndMemory
{
	public:
		ImageAndMemory();
		~ImageAndMemory();
		VkImage image = VK_NULL_HANDLE;
		VkImageView imageView = VK_NULL_HANDLE;
		VkDeviceMemory memory = VK_NULL_HANDLE;
		void CreateImage(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usageFlags, VkMemoryPropertyFlags memoryProperties);
		void Cleanup(VkDevice logicalDevice);
		
};