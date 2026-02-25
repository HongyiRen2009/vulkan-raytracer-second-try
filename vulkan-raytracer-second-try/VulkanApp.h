#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include "VulkanCore.h"
#include "VulkanApp.h"
#include "VulkanUtils.h"
#include "VulkanQueue.h"
#include "RaytracingComputePipeline.h"
#include "FrameData.h"
#include "Camera.h"
#include "Chunk.h"
namespace my_vulkan
{

	const int MAX_FRAMES_IN_FLIGHT = 2;

	class VulkanApp {
		public:
			VulkanApp();
			~VulkanApp();
			void Init(const char* AppName, GLFWwindow* window, Camera* camera);
			void Render();
			void Cleanup();
			void OnWindowResized(int width=-1, int height=-1);
			void UpdateChunkSSBO(vector<Chunk>& chunks);
	private:
		VulkanCore vulkanCore;
		int numImages = 0;
		VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
		RaytracingComputePipeline rtPipeline;
		FrameData frameData;
		VulkanQueue* queue = nullptr;
		Camera* camera;
		void CreateDescriptorPool();
		void RecordCommandBuffer(uint32_t frameIndex, uint32_t imageIndex);
		void RecordComputeCommandBuffer();
		
		
	};
}