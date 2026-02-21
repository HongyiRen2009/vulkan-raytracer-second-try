#include "VulkanQueue.h"
#include "VulkanUtils.h"
namespace my_vulkan
{
	VulkanQueue::VulkanQueue() {
	}
	VulkanQueue::~VulkanQueue() {

	}
	void VulkanQueue::Cleanup() {
		if (renderCompleteSemaphore != VK_NULL_HANDLE) {
			vkDestroySemaphore(m_logicalDevice, renderCompleteSemaphore, nullptr);
			renderCompleteSemaphore = VK_NULL_HANDLE;
		}
		if (presentCompleteSemaphore != VK_NULL_HANDLE){
			vkDestroySemaphore(m_logicalDevice, presentCompleteSemaphore, nullptr);
			presentCompleteSemaphore = VK_NULL_HANDLE;
		}
	}
	void VulkanQueue::Init(VkDevice logicalDevice, VkSwapchainKHR swapChain, uint32_t QueueFamily, uint32_t queueFamilyIndex) {
		m_logicalDevice = logicalDevice;
		m_swapChain = swapChain;
		vkGetDeviceQueue(logicalDevice, queueFamilyIndex, 0, &m_queue);
		CreateSemaphores();
	}
	void VulkanQueue::CreateSemaphores() {
		VkSemaphoreCreateInfo semaphoreCreateInfo{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
		};
		VkResult result = vkCreateSemaphore(m_logicalDevice, &semaphoreCreateInfo, nullptr, &renderCompleteSemaphore);
		CHECK_VK_RESULT(result,"vkCreateSemaphore");
		result = vkCreateSemaphore(m_logicalDevice, &semaphoreCreateInfo, nullptr, &presentCompleteSemaphore);
		CHECK_VK_RESULT(result,"vkCreateSemaphore");

	}
	uint32_t VulkanQueue::AcquireNextImageKHR() {
		uint32_t imageIndex;
		VkResult result = vkAcquireNextImageKHR(m_logicalDevice, m_swapChain, UINT64_MAX, presentCompleteSemaphore, VK_NULL_HANDLE, &imageIndex);
		CHECK_VK_RESULT(result,"vkAcquireNextImageKHR");
		return imageIndex;
	}
	void VulkanQueue::SubmitAsync(VkCommandBuffer commandBuffer) {
		VkPipelineStageFlags waitFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submitInfo{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = nullptr,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &presentCompleteSemaphore,
		.pWaitDstStageMask = &waitFlags,
		.commandBufferCount = 1,
		.pCommandBuffers = &commandBuffer,
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &renderCompleteSemaphore,
	};
	VkResult result = vkQueueSubmit(m_queue, 1, &submitInfo, VK_NULL_HANDLE);
	CHECK_VK_RESULT(result,"vkQueueSubmit");

	}
	void VulkanQueue::SubmitSync(VkCommandBuffer commandBuffer) {
		VkSubmitInfo submitInfo{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = nullptr,
		.waitSemaphoreCount = 0,
		.pWaitSemaphores = nullptr,
		.pWaitDstStageMask = nullptr,
		.commandBufferCount = 1,
		.pCommandBuffers = &commandBuffer,
		.signalSemaphoreCount = 0,
		.pSignalSemaphores = nullptr,
		};
		VkResult result = vkQueueSubmit(m_queue, 1, &submitInfo, VK_NULL_HANDLE);
		CHECK_VK_RESULT(result, "vkQueueSubmit");

		
	}
	void VulkanQueue::Present(uint32_t imageIndex) {
		VkPresentInfoKHR presentInfo{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.pNext = nullptr,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &renderCompleteSemaphore,
		.swapchainCount = 1,
		.pSwapchains = &m_swapChain,
		.pImageIndices = &imageIndex,
		};
		VkResult result = vkQueuePresentKHR(m_queue, &presentInfo);
		CHECK_VK_RESULT(result, "vkQueuePresentKHR");
	}
	void VulkanQueue::WaitIdle() {
		vkQueueWaitIdle(m_queue);
	}
}