#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include "VulkanUtils.h"
#include <iostream>
#include "BufferAndMemory.h"
#include "RaytracingComputePipeline.h"
#include "TAAComputePipeline.h"
#include "Chunk.h"
#include "ImageAndMemory.h"
using namespace std;
namespace my_vulkan
{
	class FrameData {
	public:
		vector<VkCommandBuffer> commandBuffers;
		vector<VkSemaphore> imageAvailable;
		vector<VkSemaphore> renderFinished;
		vector<VkFence> inFlight;
		vector<VkDescriptorSet> raytracerDescriptorSet;
		vector<BufferAndMemory> raytracerUniformBuffers;
		vector <BufferAndMemory> chunkSSBO;
		vector <BufferAndMemory> voxelSSBO;
		vector<BufferAndMemory> oneNodeSSBO;
		vector<ImageAndMemory> computeOutputImage;
		vector<ImageAndMemory> taaOutputImage;
		vector<ImageAndMemory> beamOutputImage;
		vector<ImageAndMemory> velocityBuffer;

		vector<VkDescriptorSet> taaDescriptorSet;
		vector<BufferAndMemory> taaUniformBuffers;
 		constexpr static int beamResolutionDivider =4;
		FrameData();
		~FrameData();
		void init(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, VkCommandPool commandPool, int numFrames,VkExtent2D extent);
		void InitializeComputeOutputLayouts(VkCommandPool commandPool, VkQueue queue);
		int AdvanceFrame() {
			frameIndex = (frameIndex + 1) % numFrames;
			return frameIndex;
		}
		int GetCurrentFrameIndex() const { return frameIndex; }
		void Cleanup(VkCommandPool commandPool);

	private:
		void CreateSemaphores();
		void CreateFences();
		void CreateCommandBuffers(VkCommandPool commandPool);
		void CreateImages(VkPhysicalDevice physicalDevice, VkExtent2D extent);
		void CreateBuffers(VkPhysicalDevice physicalDevice, VkDevice logicalDevice);
		int numFrames = 0;
		int frameIndex = -1;

		VkDevice logicalDevice = VK_NULL_HANDLE;
	};
}