#pragma once
#include "ComputePipeline.h"
namespace my_vulkan
{
	class RaytracingComputePipeline : public ComputePipeline
	{
	public:
		RaytracingComputePipeline();
		~RaytracingComputePipeline();
		void UpdateDescriptorSet(VkDescriptorSet descriptorSet, VkImageView outputImageView);
	protected:
		virtual VkDescriptorSetLayout CreateDescriptorSetLayout(VulkanCore& core) override;
	};
}