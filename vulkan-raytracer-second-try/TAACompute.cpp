#include "TAAComputePipeline.h"	
using namespace my_vulkan;
TAAComputePipeline::TAAComputePipeline() {
}
TAAComputePipeline::~TAAComputePipeline() {
}
void TAAComputePipeline::Cleanup() {
	if (linearSampler != VK_NULL_HANDLE && logicalDevice != VK_NULL_HANDLE) {
		vkDestroySampler(logicalDevice, linearSampler, nullptr);
		linearSampler = VK_NULL_HANDLE;
	}
	ComputePipeline::Cleanup();
}
void TAAComputePipeline::UpdateDescriptorSet(VkDescriptorSet descriptorSet, VkImageView currentFrameImageView, VkImageView previousFrameImageView, VkBuffer uniformBuffer, VkImageView velocityImage, VkImageView outputFrameImageView,VkImageView depthBufferImageView) {
	VkDescriptorImageInfo currentFrameImageInfo{
		.sampler = linearSampler,
		.imageView = currentFrameImageView,
		.imageLayout = VK_IMAGE_LAYOUT_GENERAL,
	};
	VkDescriptorImageInfo previousFrameImageInfo{
		.sampler = linearSampler,
		.imageView = previousFrameImageView,
		.imageLayout = VK_IMAGE_LAYOUT_GENERAL,
	};

	VkDescriptorBufferInfo uniformBufferInfo{
		.buffer = uniformBuffer,
		.offset = 0,
		.range = sizeof(TAAUniformData),
	};
	VkDescriptorImageInfo velocityImageInfo{
	.sampler = linearSampler,
	.imageView = velocityImage,
	.imageLayout = VK_IMAGE_LAYOUT_GENERAL,
	};
	VkDescriptorImageInfo outputFrameImageInfo{
		.sampler = VK_NULL_HANDLE,
		.imageView = outputFrameImageView,
		.imageLayout = VK_IMAGE_LAYOUT_GENERAL,
	};
	VkDescriptorImageInfo depthBufferInfo{
		.sampler = linearSampler,
		.imageView = depthBufferImageView,
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
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.pImageInfo = &currentFrameImageInfo,
},
{
	.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
	.pNext = nullptr,
	.dstSet = descriptorSet,
	.dstBinding = 1,
	.dstArrayElement = 0,
	.descriptorCount = 1,
	.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
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
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
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
	},
	{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.pNext = nullptr,
		.dstSet = descriptorSet,
		.dstBinding = 5,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.pImageInfo = &depthBufferInfo,
	}
	};
	vkUpdateDescriptorSets(logicalDevice, (uint32_t)descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}
VkDescriptorSetLayout TAAComputePipeline::CreateDescriptorSetLayout(VulkanCore& core) {
	if (linearSampler == VK_NULL_HANDLE) {
		VkSamplerCreateInfo samplerCreateInfo{
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.magFilter = VK_FILTER_LINEAR,
			.minFilter = VK_FILTER_LINEAR,
			.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
			.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			.mipLodBias = 0.0f,
			.anisotropyEnable = VK_FALSE,
			.maxAnisotropy = 1.0f,
			.compareEnable = VK_FALSE,
			.compareOp = VK_COMPARE_OP_ALWAYS,
			.minLod = 0.0f,
			.maxLod = 0.0f,
			.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
			.unnormalizedCoordinates = VK_FALSE,
		};
		VkResult samplerResult = vkCreateSampler(core.GetLogicalDevice(), &samplerCreateInfo, nullptr, &linearSampler);
		CHECK_VK_RESULT(samplerResult, "vkCreateSampler (TAA linearSampler)");
	}

	VkDescriptorSetLayoutBinding currentFrameImageBinding{
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		.pImmutableSamplers = nullptr,
	};
	VkDescriptorSetLayoutBinding previousFrameImageBinding{
		.binding = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
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
	.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
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
	VkDescriptorSetLayoutBinding depthBufferBinding{
		.binding = 5,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		.pImmutableSamplers = nullptr,
	};
	std::vector<VkDescriptorSetLayoutBinding> bindings = { currentFrameImageBinding, previousFrameImageBinding, uniformBufferBinding, velocityImageBinding, outputFrameImageBinding,depthBufferBinding };
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