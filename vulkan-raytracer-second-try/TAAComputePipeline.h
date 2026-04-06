#pragma once
#include <vulkan/vulkan.h>
#include "ComputePipeline.h"
#include <glm/glm.hpp>
using namespace glm;
namespace my_vulkan
{
	struct alignas(16) TAAPushConstants {
		uint32_t TAAToggle;
	};
	struct alignas(16) TAAUniformData {
		vec4 frameSize;
		alignas(16) vec2 jitter;
	};
	class TAAComputePipeline : public ComputePipeline
	{

	public:
		TAAComputePipeline();
		~TAAComputePipeline();
		void UpdateDescriptorSet(VkDescriptorSet descriptorSet, VkImageView currentFrameImageView, VkImageView previousFrameImageView, VkBuffer uniformBuffer, VkImageView velocityImage, VkImageView outputFrameImageView);
	
	protected:
		virtual VkDescriptorSetLayout CreateDescriptorSetLayout(VulkanCore& core) override;
		
		virtual uint32_t GetPushConstantSize() const override {
			return sizeof(TAAPushConstants);
		}
	};
}