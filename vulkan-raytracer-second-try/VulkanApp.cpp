#include "VulkanApp.h"
using namespace my_vulkan;
VulkanApp::VulkanApp() {
}
VulkanApp::~VulkanApp() {

}
void VulkanApp::Init(const char* AppName, GLFWwindow* window, Camera* camera) {
	this->camera = camera;
	vulkanCore.Init(AppName, window);
	queue = vulkanCore.GetQueue();
	numImages = vulkanCore.GetNumSwapChainImages();
	CreateDescriptorPool();
	rtPipeline.Init(vulkanCore, descriptorPool, "Raytrace.comp.spv");
	frameData.init(vulkanCore.GetPhysicalDevice(), vulkanCore.GetLogicalDevice(), vulkanCore.GetCommandPool(), MAX_FRAMES_IN_FLIGHT, vulkanCore.surfaceExtent);
	frameData.InitializeComputeOutputLayouts(vulkanCore.GetCommandPool(), queue->GetQueue());

	rtPipeline.AllocateDescriptorSets(MAX_FRAMES_IN_FLIGHT, frameData.descriptor);
	for (int i = 0;i < MAX_FRAMES_IN_FLIGHT;i++) {
		rtPipeline.UpdateDescriptorSet(frameData.descriptor[i], frameData.computeOutput[i], frameData.uniformBuffers[i].buffer, frameData.chunkSSBO[i].buffer, frameData.voxelSSBO[i].buffer);
	}
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
		.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT,
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
		.CHUNK_HEIGHT = Chunk::CHUNK_HEIGHT,
		
	};
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
		.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
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
uint32_t hashChunkPos(ivec2 pos) {
	uint32_t hash = uint32_t(pos.x * 73856093) ^ uint32_t(pos.y * 19349663);
	return hash % 1000u;
}

void VulkanApp::UpdateChunkSSBO(vector<Chunk>& chunks) {
	unsigned int CHUNK_SSBO_DATA_SIZE = Chunk::CHUNK_SIZE * Chunk::CHUNK_SIZE * Chunk::CHUNK_HEIGHT;
	unsigned int CHUNK_SSBO_DATA_SIZE_BYTES = CHUNK_SSBO_DATA_SIZE * sizeof(unsigned int);
	for (auto& chunk : chunks) {

		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			if (!chunk.isDirty) continue;
			ChunkData data = {
				.chunkPos = chunk.chunkPosition,
				.gpuSlot = chunk.gpuSlot,
			};
			frameData.chunkSSBO[i].AddData(vulkanCore.GetLogicalDevice(), &data, sizeof(ChunkData), hashChunkPos(data.chunkPos) * sizeof(ChunkData));
			frameData.voxelSSBO[i].AddData(vulkanCore.GetLogicalDevice(), chunk.blocks, CHUNK_SSBO_DATA_SIZE_BYTES, data.gpuSlot * CHUNK_SSBO_DATA_SIZE_BYTES);

		}
		chunk.isDirty = false;
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

	frameData.init(vulkanCore.GetPhysicalDevice(), vulkanCore.GetLogicalDevice(), vulkanCore.GetCommandPool(), MAX_FRAMES_IN_FLIGHT, vulkanCore.surfaceExtent);
	frameData.InitializeComputeOutputLayouts(vulkanCore.GetCommandPool(), queue->GetQueue());

	rtPipeline.Init(vulkanCore, descriptorPool, "Raytrace.comp.spv");
	rtPipeline.AllocateDescriptorSets(MAX_FRAMES_IN_FLIGHT, frameData.descriptor);
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		rtPipeline.UpdateDescriptorSet(frameData.descriptor[i], frameData.computeOutput[i], frameData.uniformBuffers[i].buffer, frameData.chunkSSBO[i].buffer, frameData.voxelSSBO[i].buffer);
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
	vulkanCore.Cleanup();
}