#pragma once
#include <vulkan/vulkan.h>
#include "VulkanCore.h"
#include "VulkanUtils.h"
namespace my_vulkan
{
	class ComputePipeline
	{
	public:
		ComputePipeline();
		~ComputePipeline();
		void Init(VulkanCore& core, VkDescriptorPool DescriptorPool, const char* ComputeShaderFileName);
		void Cleanup();
		void RecordCommandBuffer(VkCommandBuffer commandBuffer,
			VkDescriptorSet DescriptorSet,
			uint32_t GroupCountX,
			uint32_t GroupCountY,
			uint32_t GroupCountZ,
			const void* pushConstants);
		void AllocateDescriptorSets(int DescriptorCount, vector<VkDescriptorSet>& DescriptorSets);

	protected:
		virtual VkDescriptorSetLayout CreateDescriptorSetLayout(VulkanCore& core) = 0;
		virtual uint32_t GetPushConstantSize() const = 0;
		VkDevice logicalDevice = VK_NULL_HANDLE;
		int numImages = 0;
	private:
		void CreatePipelineLayout();
		void CreatePipeline(VkShaderModule computeShader);
		VkShaderModule ComputeShaderModule = VK_NULL_HANDLE;
		VkPipeline pipeline = VK_NULL_HANDLE;
		VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
		VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
		VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
		VkBuffer shaderStorageBuffer = VK_NULL_HANDLE;
	};
}