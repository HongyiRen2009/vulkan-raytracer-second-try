#include "RaytracingComputePipeline.h"
namespace my_vulkan
{
	RaytracingComputePipeline::RaytracingComputePipeline() {
	}
	RaytracingComputePipeline::~RaytracingComputePipeline() {
	}
VkDescriptorSetLayout RaytracingComputePipeline::CreateDescriptorSetLayout(VulkanCore& core) {
	VkDescriptorSetLayoutBinding outputImageBinding{
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		.pImmutableSamplers = nullptr,
	};
	VkDescriptorSetLayoutCreateInfo layoutCreateInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.bindingCount = 1,
		.pBindings = &outputImageBinding,
	};
	VkDescriptorSetLayout descriptorSetLayout;
	VkResult result = vkCreateDescriptorSetLayout(core.GetLogicalDevice(), &layoutCreateInfo, nullptr, &descriptorSetLayout);
	CHECK_VK_RESULT(result,"vkCreateDescriptorSetLayout");
	return descriptorSetLayout;
}
void RaytracingComputePipeline::UpdateDescriptorSet(VkDescriptorSet descriptorSet, VkImageView outputImageView) {
	VkDescriptorImageInfo outputImageInfo{
		.sampler = VK_NULL_HANDLE,
		.imageView = outputImageView,
		.imageLayout = VK_IMAGE_LAYOUT_GENERAL,
	};
	VkWriteDescriptorSet writeDescriptorSet{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.pNext = nullptr,
		.dstSet = descriptorSet,
		.dstBinding = 0,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		.pImageInfo = &outputImageInfo,
		.pBufferInfo = nullptr,
		.pTexelBufferView = nullptr,
	};
	vkUpdateDescriptorSets(logicalDevice, 1, &writeDescriptorSet, 0, nullptr);
}
}