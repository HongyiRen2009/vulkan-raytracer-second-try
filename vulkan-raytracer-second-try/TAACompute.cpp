#include "TAAComputePipeline.h"	
using namespace my_vulkan;
TAAComputePipeline::TAAComputePipeline() {
}
TAAComputePipeline::~TAAComputePipeline() {
}
void TAAComputePipeline::UpdateDescriptorSet(VkDescriptorSet descriptorSet, VkImageView currentFrameImageView, VkImageView previousFrameImageView, VkBuffer uniformBuffer, VkImageView velocityImage, VkImageView outputFrameImageView) {
	VkDescriptorImageInfo currentFrameImageInfo{
		.sampler = VK_NULL_HANDLE,
		.imageView = currentFrameImageView,
		.imageLayout = VK_IMAGE_LAYOUT_GENERAL,
	};
	VkDescriptorImageInfo previousFrameImageInfo{
		.sampler = VK_NULL_HANDLE,
		.imageView = previousFrameImageView,
		.imageLayout = VK_IMAGE_LAYOUT_GENERAL,
	};

	VkDescriptorBufferInfo uniformBufferInfo{
		.buffer = uniformBuffer,
		.offset = 0,
		.range = sizeof(TAAUniformData),
	};
	VkDescriptorImageInfo velocityImageInfo{
	.sampler = VK_NULL_HANDLE,
	.imageView = velocityImage,
	.imageLayout = VK_IMAGE_LAYOUT_GENERAL,
	};
	VkDescriptorImageInfo outputFrameImageInfo{
		.sampler = VK_NULL_HANDLE,
		.imageView = outputFrameImageView,
		.imageLayout = VK_IMAGE_LAYOUT_GENERAL,
	};
	vector<VkWriteDescriptorSet> descriptorWrites = {
	{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.pNext = nullptr,
		.dstSet = descriptorSet,
		.dstBinding = 0,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		.pImageInfo = &currentFrameImageInfo,
},
{
	.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
	.pNext = nullptr,
	.dstSet = descriptorSet,
	.dstBinding = 1,
	.dstArrayElement = 0,
	.descriptorCount = 1,
	.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
	.pImageInfo = &previousFrameImageInfo,
	},
	{
	.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.pNext = nullptr,
		.dstSet = descriptorSet,
		.dstBinding = 2,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.pImageInfo = nullptr,
		.pBufferInfo = &uniformBufferInfo,
		.pTexelBufferView = nullptr,
	},
		{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.pNext = nullptr,
		.dstSet = descriptorSet,
		.dstBinding = 3,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		.pImageInfo = &velocityImageInfo,
	},
	{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.pNext = nullptr,
		.dstSet = descriptorSet,
		.dstBinding = 4,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		.pImageInfo = &outputFrameImageInfo,
	}
	};
	vkUpdateDescriptorSets(logicalDevice, (uint32_t)descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}
VkDescriptorSetLayout TAAComputePipeline::CreateDescriptorSetLayout(VulkanCore& core) {
	VkDescriptorSetLayoutBinding currentFrameImageBinding{
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		.pImmutableSamplers = nullptr,
	};
	VkDescriptorSetLayoutBinding previousFrameImageBinding{
		.binding = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		.pImmutableSamplers = nullptr,
	};
	VkDescriptorSetLayoutBinding uniformBufferBinding{
		.binding = 2,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		.pImmutableSamplers = nullptr,
	};
	VkDescriptorSetLayoutBinding velocityImageBinding{
	.binding = 3,
	.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
	.descriptorCount = 1,
	.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
	.pImmutableSamplers = nullptr,
	};
	VkDescriptorSetLayoutBinding outputFrameImageBinding{
		.binding = 4,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		.pImmutableSamplers = nullptr,
	};
	std::vector<VkDescriptorSetLayoutBinding> bindings = { currentFrameImageBinding, previousFrameImageBinding, uniformBufferBinding, velocityImageBinding, outputFrameImageBinding };
	VkDescriptorSetLayoutCreateInfo layoutCreateInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.bindingCount = (uint32_t)bindings.size(),
		.pBindings = bindings.data(),
	};
	VkDescriptorSetLayout descriptorSetLayout;
	VkResult result = vkCreateDescriptorSetLayout(core.GetLogicalDevice(), &layoutCreateInfo, nullptr, &descriptorSetLayout);
	CHECK_VK_RESULT(result, "vkCreateDescriptorSetLayout");
	return descriptorSetLayout;
}