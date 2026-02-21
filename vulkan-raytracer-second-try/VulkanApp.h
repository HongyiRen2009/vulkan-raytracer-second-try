#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include "VulkanCore.h"
#include "VulkanApp.h"
#include "VulkanUtils.h"
#include "VulkanQueue.h"
#include "ComputePipeline.h"
namespace my_vulkan
{


	class VulkanApp {
		public:
			VulkanApp();
			~VulkanApp();
			void Init(const char* AppName, GLFWwindow* window);
			void Render();
	private:
		VulkanCore vulkanCore;
		int numImages = 0;
		VulkanQueue *queue = nullptr;
		//ComputePipeline computePipeline;
		vector<VkCommandBuffer> commandBuffers;
		vector<VkDescriptorSet> DescriptorSets;
		void CreateCommandBuffers();
		void RecordCommandBuffer();
		void RecordComputeCommandBuffer();
		
		
	};
}