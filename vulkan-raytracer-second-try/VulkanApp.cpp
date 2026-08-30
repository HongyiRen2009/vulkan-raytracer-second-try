#include "VulkanApp.h"
using namespace my_vulkan;
uint32_t VulkanApp::frameCounter = 0;
VulkanApp::VulkanApp() {
}
VulkanApp::~VulkanApp() {

}
void VulkanApp::Init(const char* AppName, GLFWwindow* window, Camera* camera) {
	this->camera = camera;
	vulkanCore.Init(AppName, window);
	CreateCopyCommandBuffer();
	blockTextureArray.Init(vulkanCore.GetLogicalDevice(), copyCommandBuffer, vulkanCore.GetQueue(), vulkanCore.GetPhysicalDevices());
	blockTextureArray.CreateTextureArray("atlas.png", 25, 25);
	queue = vulkanCore.GetQueue();
	numImages = vulkanCore.GetNumSwapChainImages();
	CreateDescriptorPool();
	raytracingPipeline.Init(vulkanCore, descriptorPool, "Raytrace.comp.spv");
	taaPipeline.Init(vulkanCore, descriptorPool, "TAACompute.comp.spv");
	frameData.init(vulkanCore.GetPhysicalDevices()->GetSelectedPhysicalDevice().device, vulkanCore.GetLogicalDevice(), vulkanCore.GetCommandPool(), MAX_FRAMES_IN_FLIGHT, vulkanCore.surfaceExtent);
	frameData.InitializeComputeOutputLayouts(vulkanCore.GetCommandPool(), queue->GetQueue());

	raytracingPipeline.AllocateDescriptorSets(MAX_FRAMES_IN_FLIGHT, frameData.raytracerDescriptorSet);
	for (int i = 0;i < MAX_FRAMES_IN_FLIGHT;i++) {
		int previousFrameIndex = (i - 1 + MAX_FRAMES_IN_FLIGHT) % MAX_FRAMES_IN_FLIGHT;
		raytracingPipeline.UpdateDescriptorSet(frameData.raytracerDescriptorSet[i], frameData.computeOutputImage[i].imageView, frameData.beamOutputImage[i].imageView,frameData.depthBuffer[i].imageView, frameData.raytracerUniformBuffers[i].buffer, frameData.chunkSSBO[i].buffer, frameData.voxelSSBO[i].buffer, frameData.oneNodeSSBO[i].buffer, &blockTextureArray, frameData.velocityBuffer[i].imageView, frameData.depthBuffer[previousFrameIndex].imageView);
	}

	taaPipeline.AllocateDescriptorSets(MAX_FRAMES_IN_FLIGHT, frameData.taaDescriptorSet);
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		int previousFrameIndex = (i - 1 + MAX_FRAMES_IN_FLIGHT) % MAX_FRAMES_IN_FLIGHT;
		taaPipeline.UpdateDescriptorSet(frameData.taaDescriptorSet[i], frameData.computeOutputImage[i].imageView, frameData.taaOutputImage[previousFrameIndex].imageView, frameData.taaUniformBuffers[i].buffer, frameData.velocityBuffer[i].imageView, frameData.taaOutputImage[i].imageView, frameData.depthBuffer[i].imageView);
	}

	ChunkData emptySlot = { .chunkPos = glm::vec3(0.0f), .gpuSlot = 0, .valid = 0 };
	for (int frameIdx = 0; frameIdx < MAX_FRAMES_IN_FLIGHT; frameIdx++) {
		for (uint32_t i = 0; i < 10000; i++) {
			frameData.chunkSSBO[frameIdx].AddData(vulkanCore.GetLogicalDevice(), &emptySlot, sizeof(ChunkData), i * sizeof(ChunkData));
		}
	}
}
void VulkanApp::CreateCopyCommandBuffer() {
	VkCommandBufferAllocateInfo allocInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = nullptr,
		.commandPool = vulkanCore.GetCommandPool(),
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};
	VkResult result = vkAllocateCommandBuffers(vulkanCore.GetLogicalDevice(), &allocInfo, &copyCommandBuffer);
	CHECK_VK_RESULT(result, "vkAllocateCommandBuffers");
}
void VulkanApp::CreateDescriptorPool() {
	vector<VkDescriptorPoolSize> poolSizes = { {
			.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			.descriptorCount = (uint32_t)MAX_FRAMES_IN_FLIGHT * 4,
	},
	{
		.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		.descriptorCount = (uint32_t)MAX_FRAMES_IN_FLIGHT * 3,
},
	{
		.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = (uint32_t)MAX_FRAMES_IN_FLIGHT * 2,
},
{
	.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
	.descriptorCount = (uint32_t)MAX_FRAMES_IN_FLIGHT,

},
{
	.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
	.descriptorCount = (uint32_t)MAX_FRAMES_IN_FLIGHT,
	},
	{
	.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
	.descriptorCount = (uint32_t)MAX_FRAMES_IN_FLIGHT,

},
	{
		.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = (uint32_t)MAX_FRAMES_IN_FLIGHT * 4,
}
	};
	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.maxSets = (uint32_t)MAX_FRAMES_IN_FLIGHT * 2,
		.poolSizeCount = (uint32_t)poolSizes.size(),
		.pPoolSizes = poolSizes.data(),
	};
	VkResult result = vkCreateDescriptorPool(vulkanCore.GetLogicalDevice(), &descriptorPoolCreateInfo, nullptr, &descriptorPool);
	CHECK_VK_RESULT(result, "vkCreateDescriptorPool");
	cout << "Descriptor pool created successfully!" << std::endl;
}

	void VulkanApp::RecordCommandBuffer(uint32_t frameIndex, uint32_t imageIndex) {
		VkImageSubresourceRange imageRange{
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1,
		};

		VkImageMemoryBarrier swapchainToTransferBarrier = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.pNext = nullptr,
			.srcAccessMask = 0,
			.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
			.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = vulkanCore.GetSwapChainImage(imageIndex),
			.subresourceRange = imageRange,
		};

		RaytracingUniformUpdateData uniformData{
			.view = camera->getViewMatrix(),
			.proj = camera->getProjectionMatrix(),
			.cameraPos = camera->position,
			.allChunksMin = ChunkManager::allChunksMin,
			.allChunksMax = ChunkManager::allChunksMax,
			.jitter = raytracingPipeline.getJitter(frameCounter),
			.sunPosition = vec3(10000.0f, 10000.0f, 0.0f),
			.time = static_cast<float>(glfwGetTime()),


		};
		RaytracingPushConstants pushConstants{
			.beamResolutionDivisor = FrameData::beamResolutionDivider,
			.computeToggle = (glfwGetKey(camera->window, GLFW_KEY_V) == GLFW_PRESS) ? 1u : 0u,
			.isBeam = 1u,
		};
		bool cameraMoved = glm::distance(camera->position, lastCameraPos) > 0.001f;
		bool cameraRotated = glm::distance(camera->front, lastCameraFront) > 0.001f;
		TAAPushConstants taaPushConstants{
			.TAAToggle = (glfwGetKey(camera->window, GLFW_KEY_B) == GLFW_PRESS) ? 1u : 0u,
			.cameraMoved = (cameraMoved||cameraRotated) ? 1u : 0u,
		};
		lastCameraPos = camera->position;
		lastCameraFront = camera->front;
		raytracingPipeline.updateUniformData(uniformData);

		frameData.raytracerUniformBuffers[frameIndex].AddData(vulkanCore.GetLogicalDevice(), raytracingPipeline.GetUniformData(), sizeof(RaytracingUniformData));

		BeginCommandBuffer(frameData.commandBuffers[frameIndex], VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);

		// Beam pass
		VkMemoryBarrier beamPreComputeBarrier{
			.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
			.pNext = nullptr,
			.srcAccessMask = 0,
			.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
		};
		vkCmdPipelineBarrier(frameData.commandBuffers[frameIndex],
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			0, 1, &beamPreComputeBarrier, 0, nullptr, 0, nullptr);

		uint32_t beamGroupCountX = (vulkanCore.surfaceExtent.width / FrameData::beamResolutionDivider + 15) / 16;
		uint32_t beamGroupCountY = (vulkanCore.surfaceExtent.height / FrameData::beamResolutionDivider + 15) / 16;
		raytracingPipeline.RecordCommandBuffer(frameData.commandBuffers[frameIndex], frameData.raytracerDescriptorSet[frameIndex], beamGroupCountX, beamGroupCountY, 1, &pushConstants);

		VkMemoryBarrier beamToFullBarrier{
			.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
			.pNext = nullptr,
			.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
		};
		vkCmdPipelineBarrier(frameData.commandBuffers[frameIndex],
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			0, 1, &beamToFullBarrier, 0, nullptr, 0, nullptr);

		uint32_t fullGroupCountX = (vulkanCore.surfaceExtent.width + 15) / 16;
		uint32_t fullGroupCountY = (vulkanCore.surfaceExtent.height + 15) / 16;
		pushConstants.isBeam = 0u;
		raytracingPipeline.RecordCommandBuffer(frameData.commandBuffers[frameIndex], frameData.raytracerDescriptorSet[frameIndex], fullGroupCountX, fullGroupCountY, 1, &pushConstants);

		// TAA pass
		VkMemoryBarrier raytracingToTAABarrier{
			.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
			.pNext = nullptr,
			.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
		};
		VkMemoryBarrier depthBufferBarrier{
			.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
			.pNext = nullptr,
			.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
		};
		TAAUniformData taaUniformData{
			.frameSize = glm::vec4(vulkanCore.surfaceExtent.width, vulkanCore.surfaceExtent.height, 1.0f / vulkanCore.surfaceExtent.width, 1.0f / vulkanCore.surfaceExtent.height),
			.jitter = raytracingPipeline.getJitter(frameCounter),
		};
		VkMemoryBarrier memoryBarriers[2] = { raytracingToTAABarrier, depthBufferBarrier };
		vkCmdPipelineBarrier(frameData.commandBuffers[frameIndex],
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			0, 2, memoryBarriers, 0, nullptr, 0, nullptr);

		frameData.taaUniformBuffers[frameIndex].AddData(vulkanCore.GetLogicalDevice(), &taaUniformData,sizeof(TAAUniformData));

		uint32_t taaGroupCountX = (vulkanCore.surfaceExtent.width + 15) / 16;
		uint32_t taaGroupCountY = (vulkanCore.surfaceExtent.height + 15) / 16;
		taaPipeline.RecordCommandBuffer(frameData.commandBuffers[frameIndex], frameData.taaDescriptorSet[frameIndex], taaGroupCountX, taaGroupCountY, 1, &taaPushConstants);

		// Copy to swapchain image
		VkMemoryBarrier taaToTransferBarrier{
			.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
			.pNext = nullptr,
			.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
		};
		vkCmdPipelineBarrier(frameData.commandBuffers[frameIndex],
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			0, 1, &taaToTransferBarrier, 0, nullptr, 0, nullptr);

		vkCmdPipelineBarrier(frameData.commandBuffers[frameIndex],
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			0, 0, nullptr, 0, nullptr, 1, &swapchainToTransferBarrier);

		VkImageCopy region = {
			.srcSubresource = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.mipLevel = 0,
				.baseArrayLayer = 0,
				.layerCount = 1,
			},
			.srcOffset = { 0, 0, 0 },
			.dstSubresource = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.mipLevel = 0,
				.baseArrayLayer = 0,
				.layerCount = 1,
			},
			.dstOffset = { 0, 0, 0 },
			.extent = { vulkanCore.surfaceExtent.width, vulkanCore.surfaceExtent.height, 1 },
		};

		vkCmdCopyImage(frameData.commandBuffers[frameIndex],
			frameData.taaOutputImage[frameIndex].image, VK_IMAGE_LAYOUT_GENERAL,
			vulkanCore.GetSwapChainImage(imageIndex), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &region);

		VkImageMemoryBarrier swapchainToPresentBarrier = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.pNext = nullptr,
			.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
			.dstAccessMask = 0,
			.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = vulkanCore.GetSwapChainImage(imageIndex),
			.subresourceRange = imageRange,
		};

		vkCmdPipelineBarrier(frameData.commandBuffers[frameIndex],
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			0, 0, nullptr, 0, nullptr, 1, &swapchainToPresentBarrier);

		VkResult result = vkEndCommandBuffer(frameData.commandBuffers[frameIndex]);
		CHECK_VK_RESULT(result, "vkEndCommandBuffer");
		frameCounter++;
	}
void VulkanApp::UpdateChunkSSBO(ChunkManager* chunkManager) {
	const vector<unique_ptr<Chunk>>& chunks = chunkManager->GetChunks();
	VkDevice logicalDevice = vulkanCore.GetLogicalDevice();

	for (auto& chunk : chunks) {
		if (!chunk->isDirty) continue;

		const bool hasLeafArray = !chunk->oneNodePool.empty();
		if (chunk->gpuSlot == UINT32_MAX) continue;
		if (hasLeafArray && chunk->oneNodeSlot == UINT32_MAX) continue;

		uint32_t slotIndex = chunk->hashTableSlot;

		ChunkData data = {
			.chunkPos = glm::vec3(chunk->chunkPosition),
			.gpuSlot = chunk->gpuSlot,
			.valid = 1,
			.oneNodeSlot = hasLeafArray ? chunk->oneNodeSlot : 0u
		};

		for (int frameIdx = 0; frameIdx < MAX_FRAMES_IN_FLIGHT; frameIdx++) {
			frameData.chunkSSBO[frameIdx].AddData(logicalDevice, &data, sizeof(ChunkData), slotIndex * sizeof(ChunkData));

			if (!chunk->nodePool.empty()) {
				frameData.voxelSSBO[frameIdx].AddData(
					logicalDevice,
					chunk->nodePool.data(),
					chunk->nodePool.size() * sizeof(Node),
					chunk->gpuSlot * sizeof(Node)
				);
			}

			if (hasLeafArray) {
				frameData.oneNodeSSBO[frameIdx].AddData(
					logicalDevice,
					chunk->oneNodePool.data(),
					chunk->oneNodePool.size() * sizeof(unsigned int),
					chunk->oneNodeSlot * sizeof(unsigned int)
				);
			}
		}

		chunk->isDirty = false;
	}
}
void VulkanApp::InvalidateChunksInGPU(vector<uint32_t> slotIndices) {
	for (int i = 0;i < slotIndices.size();i++) {
		ChunkData data = {
			.chunkPos = glm::vec3(0.0f),
			.gpuSlot = 0,
			.valid = 0
		};
		for (int frameIdx = 0; frameIdx < MAX_FRAMES_IN_FLIGHT; frameIdx++) {
			frameData.chunkSSBO[frameIdx].AddData(vulkanCore.GetLogicalDevice(), &data, sizeof(ChunkData), slotIndices[i] * sizeof(ChunkData));
		}
	}
}
void VulkanApp::Render() {
	queue->WaitIdle();
	int frameIndex = frameData.AdvanceFrame();
	VkDevice logicalDevice = vulkanCore.GetLogicalDevice();
	vkWaitForFences(logicalDevice, 1, &frameData.inFlight[frameIndex], VK_TRUE, UINT64_MAX);
	uint32_t imageIndex = queue->AcquireNextImageKHR(frameData.imageAvailable[frameIndex]);
	if (imageIndex == 6969) {
		OnWindowResized();
		return;
	}
	vkResetFences(logicalDevice, 1, &frameData.inFlight[frameIndex]);

	RecordCommandBuffer(frameIndex, imageIndex);
	queue->SubmitAsync(frameData.commandBuffers[frameIndex], frameData.imageAvailable[frameIndex], VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, frameData.renderFinished[frameIndex], frameData.inFlight[frameIndex]);
	queue->Present(imageIndex, frameData.renderFinished[frameIndex]);
}
void VulkanApp::OnWindowResized(int width, int height) {
	queue->WaitIdle();
	frameCounter = 0;

	raytracingPipeline.Cleanup();
	taaPipeline.Cleanup();
	frameData.Cleanup(vulkanCore.GetCommandPool());

	if (descriptorPool != VK_NULL_HANDLE) {
		vkDestroyDescriptorPool(vulkanCore.GetLogicalDevice(), descriptorPool, nullptr);
		descriptorPool = VK_NULL_HANDLE;
	}

	vulkanCore.RecreateSwapChain();
	queue->UpdateSwapChain(vulkanCore.GetSwapChain());
	CreateDescriptorPool();

	frameData.init(vulkanCore.GetPhysicalDevices()->GetSelectedPhysicalDevice().device, vulkanCore.GetLogicalDevice(), vulkanCore.GetCommandPool(), MAX_FRAMES_IN_FLIGHT, vulkanCore.surfaceExtent);
	frameData.InitializeComputeOutputLayouts(vulkanCore.GetCommandPool(), queue->GetQueue());

	raytracingPipeline.Init(vulkanCore, descriptorPool, "Raytrace.comp.spv");
	raytracingPipeline.AllocateDescriptorSets(MAX_FRAMES_IN_FLIGHT, frameData.raytracerDescriptorSet);
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		int previousFrameIndex = (i - 1 + MAX_FRAMES_IN_FLIGHT) % MAX_FRAMES_IN_FLIGHT;
		raytracingPipeline.UpdateDescriptorSet(frameData.raytracerDescriptorSet[i], frameData.computeOutputImage[i].imageView, frameData.beamOutputImage[i].imageView,frameData.depthBuffer[i].imageView, frameData.raytracerUniformBuffers[i].buffer, frameData.chunkSSBO[i].buffer, frameData.voxelSSBO[i].buffer, frameData.oneNodeSSBO[i].buffer, &blockTextureArray, frameData.velocityBuffer[i].imageView, frameData.depthBuffer[previousFrameIndex].imageView);
	}

	taaPipeline.Init(vulkanCore, descriptorPool, "TAACompute.comp.spv");
	taaPipeline.AllocateDescriptorSets(MAX_FRAMES_IN_FLIGHT, frameData.taaDescriptorSet);
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		int previousFrameIndex = (i - 1 + MAX_FRAMES_IN_FLIGHT) % MAX_FRAMES_IN_FLIGHT;
		taaPipeline.UpdateDescriptorSet(frameData.taaDescriptorSet[i], frameData.computeOutputImage[i].imageView, frameData.taaOutputImage[previousFrameIndex].imageView, frameData.taaUniformBuffers[i].buffer, frameData.velocityBuffer[i].imageView, frameData.taaOutputImage[i].imageView, frameData.depthBuffer[i].imageView);
	}
}
void VulkanApp::Cleanup() {
	queue->WaitIdle();
	raytracingPipeline.Cleanup();
	taaPipeline.Cleanup();
	if (descriptorPool != VK_NULL_HANDLE) {
		vkDestroyDescriptorPool(vulkanCore.GetLogicalDevice(), descriptorPool, nullptr);
		descriptorPool = VK_NULL_HANDLE;
		cout << "Descriptor pool destroyed successfully!" << std::endl;
	}
	frameData.Cleanup(vulkanCore.GetCommandPool());
	blockTextureArray.Cleanup(vulkanCore.GetLogicalDevice());

	vulkanCore.Cleanup();
}
