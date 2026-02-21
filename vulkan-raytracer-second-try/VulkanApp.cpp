#include "VulkanApp.h"
using namespace my_vulkan;
VulkanApp::VulkanApp() {
}
VulkanApp::~VulkanApp() {
		
}
void VulkanApp::Init(const char* AppName, GLFWwindow* window) {
	vulkanCore.Init(AppName, window);
	numImages = vulkanCore.GetNumSwapChainImages();
	queue = vulkanCore.GetQueue();
	CreateCommandBuffers();
	RecordCommandBuffer();
}
void VulkanApp::CreateCommandBuffers() {
	commandBuffers.resize(numImages);
	vulkanCore.CreateCommandBuffers(numImages, commandBuffers.data());
}
void VulkanApp::RecordCommandBuffer() {
	VkClearColorValue clearColor = { 1.0f, 0.0f, 0.0f, 1.0f };
	VkImageSubresourceRange imageRange{
		.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.baseMipLevel = 0,
		.levelCount = 1,
		.baseArrayLayer = 0,
		.layerCount = 1,
	};
	for (uint32_t i = 0;i < numImages;i++) {
		VkImageMemoryBarrier PresentToClearBarrier = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.pNext = nullptr,
			.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT,
			.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
			.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = vulkanCore.GetSwapChainImage(i),
			.subresourceRange = imageRange,
		};
		VkImageMemoryBarrier ClearToPresentBarrier = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.pNext = nullptr,
			.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
			.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = vulkanCore.GetSwapChainImage(i),
			.subresourceRange = imageRange,
		};
		BeginCommandBuffer(commandBuffers[i], VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);
		vkCmdPipelineBarrier(commandBuffers[i], VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &PresentToClearBarrier);
		vkCmdClearColorImage(commandBuffers[i], vulkanCore.GetSwapChainImage(i), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor, 1, &imageRange);
		vkCmdPipelineBarrier(commandBuffers[i], VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &ClearToPresentBarrier);
		VkResult result = vkEndCommandBuffer(commandBuffers[i]);
		CHECK_VK_RESULT(result,"vkEndCommandBuffer");
	}

}
void VulkanApp::Render() {
	queue->WaitIdle();
	uint32_t imageIndex = queue->AcquireNextImageKHR();
	queue->SubmitAsync(commandBuffers[imageIndex]);
	queue->Present(imageIndex);
}
//void VulkanApp::RecordComputeCommandBuffer() {
//	for (int i = 0;i < numImages;i++) {
//		BeginCommandBuffer(commandBuffers[i], VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);
//		computePipeline.RecordCommandBuffer(commandBuffers[i], computePipeline.GetDescriptorSet(i), 16, 16, 1);
//		VkResult result = vkEndCommandBuffer(commandBuffers[i]);
//		CHECK_VK_RESULT(result,"vkEndCommandBuffer");
//	}
//}