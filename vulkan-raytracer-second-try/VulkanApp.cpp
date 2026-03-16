#include "VulkanApp.h"
using namespace my_vulkan;
VulkanApp::VulkanApp() {
}
VulkanApp::~VulkanApp() {

}
void VulkanApp::Init(const char* AppName, GLFWwindow* window, Camera* camera) {
	this->camera = camera;
	vulkanCore.Init(AppName, window);
	CreateCopyCommandBuffer();
	blockTextureArray.Init(vulkanCore.GetLogicalDevice(), copyCommandBuffer, vulkanCore.GetQueue(), vulkanCore.GetPhysicalDevices());
	blockTextureArray.CreateTextureArray("atlas.png",25,25);
	queue = vulkanCore.GetQueue();
	numImages = vulkanCore.GetNumSwapChainImages();
	CreateDescriptorPool();
	rtPipeline.Init(vulkanCore, descriptorPool, "Raytrace.comp.spv");
	frameData.init(vulkanCore.GetPhysicalDevices()->GetSelectedPhysicalDevice().device, vulkanCore.GetLogicalDevice(), vulkanCore.GetCommandPool(), MAX_FRAMES_IN_FLIGHT, vulkanCore.surfaceExtent);
	frameData.InitializeComputeOutputLayouts(vulkanCore.GetCommandPool(), queue->GetQueue());

	rtPipeline.AllocateDescriptorSets(MAX_FRAMES_IN_FLIGHT, frameData.descriptor);
	for (int i = 0;i < MAX_FRAMES_IN_FLIGHT;i++) {
		rtPipeline.UpdateDescriptorSet(frameData.descriptor[i], frameData.computeOutput[i], frameData.uniformBuffers[i].buffer, frameData.chunkSSBO[i].buffer, frameData.voxelSSBO[i].buffer,&blockTextureArray);
	}
	ChunkData emptySlot = { .chunkPos = glm::vec2(0.0f), .gpuSlot = 0, .valid = 0 };
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
			.descriptorCount = (uint32_t)MAX_FRAMES_IN_FLIGHT,
	},
	{
		.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
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
		.descriptorCount = (uint32_t)MAX_FRAMES_IN_FLIGHT,
}
	};
	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.maxSets = (uint32_t)MAX_FRAMES_IN_FLIGHT,
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

	ComputeUniformData uniformData{
		.view = camera->getViewMatrix(),
		.proj = camera->getProjectionMatrix(),
		.cameraPos = camera->position,
		.CHUNK_SIZE = Chunk::CHUNK_SIZE,
		.blockDataArray = {}
	};
	memcpy(uniformData.blockDataArray, Chunk::blockDataArray, sizeof(Chunk::blockDataArray));
	frameData.uniformBuffers[frameIndex].AddData(vulkanCore.GetLogicalDevice(), &uniformData, sizeof(ComputeUniformData));

	BeginCommandBuffer(frameData.commandBuffers[frameIndex], VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);

	vkCmdPipelineBarrier(frameData.commandBuffers[frameIndex],
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		0, 0, nullptr, 0, nullptr, 1, &swapchainToTransferBarrier);

	uint32_t groupCountX = (vulkanCore.surfaceExtent.width + 15) / 16;
	uint32_t groupCountY = (vulkanCore.surfaceExtent.height + 15) / 16;
	rtPipeline.RecordCommandBuffer(frameData.commandBuffers[frameIndex], frameData.descriptor[frameIndex], groupCountX, groupCountY, 1);

	VkMemoryBarrier memoryBarrier{
		.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
		.pNext = nullptr,
		.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
		.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
	};
	vkCmdPipelineBarrier(frameData.commandBuffers[frameIndex],
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);

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
		frameData.computeOutputImage[frameIndex], VK_IMAGE_LAYOUT_GENERAL,
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
}
void VulkanApp::UpdateChunkSSBO(ChunkManager* chunkManager) {
	const vector<unique_ptr<Chunk>>& chunks = chunkManager->GetChunks();
	size_t maxNodesPerChunk = Chunk::CHUNK_SIZE * Chunk::CHUNK_SIZE * Chunk::CHUNK_SIZE;
	size_t bytesPerChunkSlot = maxNodesPerChunk * sizeof(Node);

	VkDevice logicalDevice = vulkanCore.GetLogicalDevice();

	for (auto& chunk : chunks) {
		if (!chunk->isDirty) continue;

		uint32_t slotIndex = chunk->hashTableSlot;

		ChunkData data = {
			.chunkPos = glm::vec2(chunk->chunkPosition),
			.gpuSlot = chunk->gpuSlot,
			.valid = 1
		};

		for (int frameIdx = 0; frameIdx < MAX_FRAMES_IN_FLIGHT; frameIdx++) {
			frameData.chunkSSBO[frameIdx].AddData(logicalDevice, &data, sizeof(ChunkData), slotIndex * sizeof(ChunkData));
			if (!chunk->nodePool.empty()) {
				size_t nodePoolSizeBytes = chunk->nodePool.size() * sizeof(Node);
				// CRITICAL FIX: Calculate offset in bytes from gpuSlot reservation
				VkDeviceSize offsetBytes = static_cast<VkDeviceSize>(chunk->gpuSlot) * bytesPerChunkSlot;
				frameData.voxelSSBO[frameIdx].AddData(
					logicalDevice,
					chunk->nodePool.data(),
					nodePoolSizeBytes,
					offsetBytes
				);

				// Debug: Verify the offset calculation
				cout << "Chunk at " << chunk->chunkPosition.x << "," << chunk->chunkPosition.y
					<< " - gpuSlot: " << chunk->gpuSlot
					<< ", offset bytes: " << offsetBytes
					<< ", nodes: " << chunk->nodePool.size() << endl;
			}
		}
		chunk->isDirty = false;
	}
}
void VulkanApp::InvalidateChunksInGPU(vector<uint32_t> slotIndices) {
	for (int i = 0;i < slotIndices.size();i++) {
		ChunkData data = {
			.chunkPos = glm::vec2(0.0f),
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

	rtPipeline.Cleanup();
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

	rtPipeline.Init(vulkanCore, descriptorPool, "Raytrace.comp.spv");
	rtPipeline.AllocateDescriptorSets(MAX_FRAMES_IN_FLIGHT, frameData.descriptor);
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		rtPipeline.UpdateDescriptorSet(frameData.descriptor[i], frameData.computeOutput[i], frameData.uniformBuffers[i].buffer, frameData.chunkSSBO[i].buffer, frameData.voxelSSBO[i].buffer,&blockTextureArray);
	}

}
void VulkanApp::Cleanup() {
	queue->WaitIdle();
	rtPipeline.Cleanup();
	if (descriptorPool != VK_NULL_HANDLE) {
		vkDestroyDescriptorPool(vulkanCore.GetLogicalDevice(), descriptorPool, nullptr);
		descriptorPool = VK_NULL_HANDLE;
		cout << "Descriptor pool destroyed successfully!" << std::endl;
	}
	frameData.Cleanup(vulkanCore.GetCommandPool());
	blockTextureArray.Cleanup(vulkanCore.GetLogicalDevice());
	
	vulkanCore.Cleanup();
}