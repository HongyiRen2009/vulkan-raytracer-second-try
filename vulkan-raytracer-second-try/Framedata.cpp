#include "Framedata.h"
namespace my_vulkan
{
	FrameData::FrameData() {
	}
	FrameData::~FrameData() {
	}
	void FrameData::init(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, VkCommandPool commandPool, int numFrames) {
		this->numFrames = numFrames;
		this->logicalDevice = logicalDevice;
		CreateSemaphores();
		CreateCommandBuffers(commandPool);
		CreateComputeOutputImageViews(physicalDevice);
		CreateFences();
	}
	void FrameData::InitializeComputeOutputLayouts(VkCommandPool commandPool, VkQueue queue) {
		VkCommandBuffer commandBuffer;
		VkCommandBufferAllocateInfo allocInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.pNext = nullptr,
			.commandPool = commandPool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1,
		};
		vkAllocateCommandBuffers(logicalDevice, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.pNext = nullptr,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
			.pInheritanceInfo = nullptr,
		};
		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		for (int i = 0; i < numFrames; i++) {
			VkImageMemoryBarrier barrier{
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				.pNext = nullptr,
				.srcAccessMask = 0,
				.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
				.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.newLayout = VK_IMAGE_LAYOUT_GENERAL,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.image = computeOutputImage[i],
				.subresourceRange = {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseMipLevel = 0,
					.levelCount = 1,
					.baseArrayLayer = 0,
					.layerCount = 1,
				},
			};
			vkCmdPipelineBarrier(commandBuffer,
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				0, 0, nullptr, 0, nullptr, 1, &barrier);
		}

		vkEndCommandBuffer(commandBuffer);

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
		vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(queue);

		vkFreeCommandBuffers(logicalDevice, commandPool, 1, &commandBuffer);
	}
	void FrameData::CreateSemaphores() {
		VkSemaphoreCreateInfo semaphoreCreateInfo{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
		};
		imageAvailable.resize(numFrames);
		renderFinished.resize(numFrames);
		for (int i = 0;i < numFrames;i++) {
			VkResult result = vkCreateSemaphore(logicalDevice, &semaphoreCreateInfo, nullptr, &imageAvailable[i]);
			CHECK_VK_RESULT(result, "vkCreateSemaphore");
			result = vkCreateSemaphore(logicalDevice, &semaphoreCreateInfo, nullptr, &renderFinished[i]);
			CHECK_VK_RESULT(result, "vkCreateSemaphore");
		}
	}
	void FrameData::CreateCommandBuffers(VkCommandPool commandPool) {
		VkCommandBufferAllocateInfo allocInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = nullptr,
		.commandPool = commandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = (uint32_t)numFrames,
		};
		commandBuffers.resize(numFrames);
		VkResult result = vkAllocateCommandBuffers(logicalDevice, &allocInfo, commandBuffers.data());
		CHECK_VK_RESULT(result, "vkAllocateCommandBuffers");
		cout << "Command buffers allocated successfully!" << std::endl;
	}
	uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}
		throw std::runtime_error("failed to find suitable memory type!");
	}
	void FrameData::CreateComputeOutputImageViews(VkPhysicalDevice physicalDevice) {
		computeOutput.resize(numFrames);
		computeOutputImage.resize(numFrames);
		computeOutputImageMemory.resize(numFrames);
		for (int i = 0;i < numFrames;i++) {


			VkImageCreateInfo imageCreateInfo{
				.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.imageType = VK_IMAGE_TYPE_2D,
				.format = VK_FORMAT_R8G8B8A8_UNORM,
				.extent = { 16, 16, 1 },
				.mipLevels = 1,
				.arrayLayers = 1,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.tiling = VK_IMAGE_TILING_OPTIMAL,
				.usage =  VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
				.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
				.queueFamilyIndexCount = 0,
				.pQueueFamilyIndices = nullptr,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			};
			VkResult result = vkCreateImage(logicalDevice, &imageCreateInfo, nullptr, &computeOutputImage[i]);
			CHECK_VK_RESULT(result, "vkCreateImage");
			VkMemoryRequirements memRequirements;
			vkGetImageMemoryRequirements(logicalDevice, computeOutputImage[i], &memRequirements);

			VkMemoryAllocateInfo allocInfo{
				.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
				.allocationSize = memRequirements.size,
				.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
			};

			result = vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &computeOutputImageMemory[i]);
			CHECK_VK_RESULT(result, "vkAllocateMemory");

			vkBindImageMemory(logicalDevice, computeOutputImage[i], computeOutputImageMemory[i], 0);

			VkImageViewCreateInfo imageViewCreateInfo{
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.image = computeOutputImage[i],
				.viewType = VK_IMAGE_VIEW_TYPE_2D,
				.format = VK_FORMAT_R8G8B8A8_UNORM,
				.components = {
					.r = VK_COMPONENT_SWIZZLE_IDENTITY,
					.g = VK_COMPONENT_SWIZZLE_IDENTITY,
					.b = VK_COMPONENT_SWIZZLE_IDENTITY,
					.a = VK_COMPONENT_SWIZZLE_IDENTITY,
				},
				.subresourceRange = {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseMipLevel = 0,
					.levelCount = 1,
					.baseArrayLayer = 0,
					.layerCount = 1,
				},
			};
			result = vkCreateImageView(logicalDevice, &imageViewCreateInfo, nullptr, &computeOutput[i]);
			CHECK_VK_RESULT(result, "vkCreateImageView");
		}
	}
	void FrameData::CreateFences() {
		inFlight.resize(numFrames);
		VkFenceCreateInfo fenceCreateInfo{
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.pNext = nullptr,
			.flags = VK_FENCE_CREATE_SIGNALED_BIT,
		};
		for (int i = 0;i < numFrames;i++) {
			VkResult result = vkCreateFence(logicalDevice, &fenceCreateInfo, nullptr, &inFlight[i]);
			CHECK_VK_RESULT(result, "vkCreateFence");
		}
	}
	void FrameData::Cleanup(VkCommandPool commandPool) {
		for (int i = 0;i < numFrames;i++) {
			vkDestroySemaphore(logicalDevice, imageAvailable[i], nullptr);
			vkDestroySemaphore(logicalDevice, renderFinished[i], nullptr);
			vkDestroyFence(logicalDevice, inFlight[i], nullptr);
			vkDestroyImageView(logicalDevice, computeOutput[i], nullptr);
			vkDestroyImage(logicalDevice, computeOutputImage[i], nullptr);
			vkFreeMemory(logicalDevice, computeOutputImageMemory[i], nullptr);
		}
	}
}