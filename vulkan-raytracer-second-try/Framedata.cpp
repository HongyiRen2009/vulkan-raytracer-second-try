#include "Framedata.h"
namespace my_vulkan
{
	FrameData::FrameData() {
	}
	FrameData::~FrameData() {
	}
	void FrameData::init(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, VkCommandPool commandPool, int numFrames, VkExtent2D extent) {
		frameIndex = -1;
		this->numFrames = numFrames;
		this->logicalDevice = logicalDevice;
		CreateSemaphores();
		CreateCommandBuffers(commandPool);
		CreateImages(physicalDevice, extent);
		CreateFences();
		CreateBuffers(physicalDevice, logicalDevice);
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
			VkImageMemoryBarrier computeBarrier{
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				.pNext = nullptr,
				.srcAccessMask = 0,
				.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
				.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.newLayout = VK_IMAGE_LAYOUT_GENERAL,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.image = computeOutputImage[i].image,
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
				0, 0, nullptr, 0, nullptr, 1, &computeBarrier);

			VkImageMemoryBarrier taaBarrier{
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				.pNext = nullptr,
				.srcAccessMask = 0,
				.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
				.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.newLayout = VK_IMAGE_LAYOUT_GENERAL,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.image = taaOutputImage[i].image,
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
				0, 0, nullptr, 0, nullptr, 1, &taaBarrier);

			VkImageMemoryBarrier beamBarrier{
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				.pNext = nullptr,
				.srcAccessMask = 0,
				.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
				.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.newLayout = VK_IMAGE_LAYOUT_GENERAL,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.image = beamOutputImage[i].image,
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
				0, 0, nullptr, 0, nullptr, 1, &beamBarrier);

			VkImageMemoryBarrier velocityBarrier{
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				.pNext = nullptr,
				.srcAccessMask = 0,
				.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
				.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.newLayout = VK_IMAGE_LAYOUT_GENERAL,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.image = velocityBuffer[i].image,
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
				0, 0, nullptr, 0, nullptr, 1, &velocityBarrier);

			VkImageMemoryBarrier depthBarrier{
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				.pNext = nullptr,
				.srcAccessMask = 0,
				.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT,
				.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.newLayout = VK_IMAGE_LAYOUT_GENERAL,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.image = depthBuffer[i].image,
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
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
				0, 0, nullptr, 0, nullptr, 1, &depthBarrier);

			// Nonzero means ordinary geometry. This prevents uninitialized
			// previous-frame depth from being mistaken for the water marker.
			VkClearColorValue initialDepth{ .float32 = { 999.0f, 0.0f, 0.0f, 0.0f } };
			VkImageSubresourceRange depthRange{
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1,
			};
			vkCmdClearColorImage(commandBuffer, depthBuffer[i].image, VK_IMAGE_LAYOUT_GENERAL, &initialDepth, 1, &depthRange);
			VkImageMemoryBarrier depthClearBarrier{
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				.pNext = nullptr,
				.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
				.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
				.oldLayout = VK_IMAGE_LAYOUT_GENERAL,
				.newLayout = VK_IMAGE_LAYOUT_GENERAL,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.image = depthBuffer[i].image,
				.subresourceRange = depthRange,
			};
			vkCmdPipelineBarrier(commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				0, 0, nullptr, 0, nullptr, 1, &depthClearBarrier);
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
	void FrameData::CreateImages(VkPhysicalDevice physicalDevice, VkExtent2D extent) {
		computeOutputImage.resize(numFrames);
		taaOutputImage.resize(numFrames);
		beamOutputImage.resize(numFrames);
		velocityBuffer.resize(numFrames);
		depthBuffer.resize(numFrames);
		for (int i = 0;i < numFrames;i++) {
			computeOutputImage[i].CreateImage(physicalDevice, logicalDevice, extent.width, extent.height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			taaOutputImage[i].CreateImage(physicalDevice, logicalDevice, extent.width, extent.height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			beamOutputImage[i].CreateImage(physicalDevice, logicalDevice, extent.width / beamResolutionDivider, extent.height / beamResolutionDivider, VK_FORMAT_R32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			velocityBuffer[i].CreateImage(physicalDevice, logicalDevice, extent.width, extent.height, VK_FORMAT_R16G16_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			depthBuffer[i].CreateImage(physicalDevice, logicalDevice, extent.width, extent.height, VK_FORMAT_R16_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		}
	}
	void FrameData::CreateBuffers(VkPhysicalDevice physicalDevice, VkDevice logicalDevice) {
		raytracerUniformBuffers.resize(numFrames);
		taaUniformBuffers.resize(numFrames);
		chunkSSBO.resize(numFrames);
		voxelSSBO.resize(numFrames);
		oneNodeSSBO.resize(numFrames);
		for (int i = 0;i < numFrames;i++) {
			raytracerUniformBuffers[i].CreateBuffer(physicalDevice, logicalDevice, sizeof(RaytracingUniformData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			taaUniformBuffers[i].CreateBuffer(physicalDevice, logicalDevice, sizeof(TAAUniformData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			chunkSSBO[i].CreateBuffer(physicalDevice, logicalDevice, sizeof(ChunkData) * 10000, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			voxelSSBO[i].CreateBuffer(physicalDevice, logicalDevice, 512 * 1024 * 1024, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT); // 512 MB for voxel data
			oneNodeSSBO[i].CreateBuffer(physicalDevice, logicalDevice, 512 * 1024 * 1024, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		}
	}
	void FrameData::Cleanup(VkCommandPool commandPool) {
		for (int i = 0;i < numFrames;i++) {
			vkDestroySemaphore(logicalDevice, imageAvailable[i], nullptr);
			vkDestroySemaphore(logicalDevice, renderFinished[i], nullptr);
			vkDestroyFence(logicalDevice, inFlight[i], nullptr);
			vkFreeCommandBuffers(logicalDevice, commandPool, 1, &commandBuffers[i]);
			raytracerUniformBuffers[i].Cleanup(logicalDevice);
			taaUniformBuffers[i].Cleanup(logicalDevice);
			chunkSSBO[i].Cleanup(logicalDevice);
			voxelSSBO[i].Cleanup(logicalDevice);
			oneNodeSSBO[i].Cleanup(logicalDevice);
			computeOutputImage[i].Cleanup(logicalDevice);
			taaOutputImage[i].Cleanup(logicalDevice);
			beamOutputImage[i].Cleanup(logicalDevice);
			velocityBuffer[i].Cleanup(logicalDevice);
			depthBuffer[i].Cleanup(logicalDevice);
		}
	}
}
