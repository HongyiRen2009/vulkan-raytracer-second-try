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
			uint32_t AcquireNextImageKHR(VkSemaphore presentCompleteSemaphore);
			void SubmitSync(VkCommandBuffer cmd);
			void SubmitAsync(VkCommandBuffer cmd,
				VkSemaphore waitSemaphore,
				VkPipelineStageFlags waitStage,
				VkSemaphore signalSemaphore,
				VkFence fence);
			void Present(uint32_t imageIndex, VkSemaphore waitSemaphore);
			void WaitIdle();
			VkQueue GetQueue() const { return m_queue; }
		private:
			VkQueue m_queue = VK_NULL_HANDLE;
			VkDevice m_logicalDevice = VK_NULL_HANDLE;
			VkSwapchainKHR m_swapChain = VK_NULL_HANDLE;
	};
}