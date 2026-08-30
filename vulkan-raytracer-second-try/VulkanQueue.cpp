#include "VulkanQueue.h"
#include "VulkanUtils.h"
namespace my_vulkan
{
	VulkanQueue::VulkanQueue() {
	}
	VulkanQueue::~VulkanQueue() {

	}
	void VulkanQueue::Init(VkDevice logicalDevice, VkSwapchainKHR swapChain, uint32_t QueueFamily, uint32_t queueFamilyIndex) {
		m_logicalDevice = logicalDevice;
		m_swapChain = swapChain;
		vkGetDeviceQueue(logicalDevice, queueFamilyIndex, 0, &m_queue);
	}
	void VulkanQueue::UpdateSwapChain(VkSwapchainKHR swapChain) {
		m_swapChain = swapChain;
	}
	uint32_t VulkanQueue::AcquireNextImageKHR(VkSemaphore presentCompleteSemaphore) {
		uint32_t imageIndex;
		VkResult result = vkAcquireNextImageKHR(m_logicalDevice, m_swapChain, UINT64_MAX, presentCompleteSemaphore, VK_NULL_HANDLE, &imageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			return 6969;
		}
		CHECK_VK_RESULT(result,"vkAcquireNextImageKHR");
		return imageIndex;
	}
	void VulkanQueue::SubmitAsync(VkCommandBuffer cmd,
		VkSemaphore waitSemaphore,
		VkPipelineStageFlags waitStage,
		VkSemaphore signalSemaphore,
		VkFence fence) {
	VkPipelineStageFlags waitFlags = waitStage;
	VkSubmitInfo submitInfo{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = nullptr,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &waitSemaphore,
		.pWaitDstStageMask = &waitFlags,
		.commandBufferCount = 1,
		.pCommandBuffers = &cmd,
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &signalSemaphore,
	};
	VkResult result = vkQueueSubmit(m_queue, 1, &submitInfo, fence);
	CHECK_VK_RESULT(result,"vkQueueSubmit");

	}
	void VulkanQueue::SubmitSync(VkCommandBuffer cmd) {
		VkSubmitInfo submitInfo{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = nullptr,
		.waitSemaphoreCount = 0,
		.pWaitSemaphores = nullptr,
		.pWaitDstStageMask = nullptr,
		.commandBufferCount = 1,
		.pCommandBuffers = &cmd,
		.signalSemaphoreCount = 0,
		.pSignalSemaphores = nullptr,
		};
		VkResult result = vkQueueSubmit(m_queue, 1, &submitInfo, VK_NULL_HANDLE);
		CHECK_VK_RESULT(result, "vkQueueSubmit");

		
	}
	void VulkanQueue::Present(uint32_t imageIndex, VkSemaphore waitSemaphore) {
		VkPresentInfoKHR presentInfo{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.pNext = nullptr,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &waitSemaphore,
		.swapchainCount = 1,
		.pSwapchains = &m_swapChain,
		.pImageIndices = &imageIndex,
		};
		VkResult result = vkQueuePresentKHR(m_queue, &presentInfo);
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			return;
		}
		CHECK_VK_RESULT(result, "vkQueuePresentKHR");
	}
	void VulkanQueue::WaitIdle() {
		vkQueueWaitIdle(m_queue);
	}
}
