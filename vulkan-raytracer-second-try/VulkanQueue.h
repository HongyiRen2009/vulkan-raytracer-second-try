#pragma once
#include <vulkan/vulkan.h>
namespace my_vulkan
{
	class VulkanQueue
	{
		public:
			VulkanQueue();
			~VulkanQueue();
			void Init(VkDevice logicalDevice, VkSwapchainKHR swapChain, uint32_t queueFamilyIndex, uint32_t queueIndex);
			uint32_t AcquireNextImageKHR();
			void SubmitSync(VkCommandBuffer commandBuffer);
			void SubmitAsync(VkCommandBuffer commandBuffer);
			void Present(uint32_t imageIndex);
			void WaitIdle();
			void Cleanup();
		private:
			void CreateSemaphores();
			VkQueue m_queue = VK_NULL_HANDLE;
			VkDevice m_logicalDevice = VK_NULL_HANDLE;
			VkSwapchainKHR m_swapChain = VK_NULL_HANDLE;
			VkSemaphore renderCompleteSemaphore = VK_NULL_HANDLE;
			VkSemaphore presentCompleteSemaphore = VK_NULL_HANDLE;
	};
}