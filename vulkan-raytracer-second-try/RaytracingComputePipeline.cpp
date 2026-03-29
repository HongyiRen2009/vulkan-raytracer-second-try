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
	VkDescriptorSetLayoutBinding uniformBufferBinding{
		.binding = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		.pImmutableSamplers = nullptr,
	};
	VkDescriptorSetLayoutBinding chunkSSBOBinding{
		.binding = 2,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		.pImmutableSamplers = nullptr,
	};
	VkDescriptorSetLayoutBinding voxelSSBOBinding{
		.binding = 3,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		.pImmutableSamplers = nullptr,
	};
	VkDescriptorSetLayoutBinding oneNodeSSBOBinding{
		.binding = 4,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		.pImmutableSamplers = nullptr,
	};
	VkDescriptorSetLayoutBinding blockTextureArrayBinding{
		.binding = 5,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		.pImmutableSamplers = nullptr,
	};
	std::vector<VkDescriptorSetLayoutBinding> bindings = { outputImageBinding, uniformBufferBinding,chunkSSBOBinding,voxelSSBOBinding,oneNodeSSBOBinding,blockTextureArrayBinding };
	VkDescriptorSetLayoutCreateInfo layoutCreateInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.bindingCount = (uint32_t)bindings.size(),
		.pBindings =	bindings.data(),
	};
	VkDescriptorSetLayout descriptorSetLayout;
	VkResult result = vkCreateDescriptorSetLayout(core.GetLogicalDevice(), &layoutCreateInfo, nullptr, &descriptorSetLayout);
	CHECK_VK_RESULT(result,"vkCreateDescriptorSetLayout");
	return descriptorSetLayout;
}
void RaytracingComputePipeline::UpdateDescriptorSet(VkDescriptorSet descriptorSet, VkImageView outputImageView,VkBuffer uniformBuffer,VkBuffer chunkSSBO, VkBuffer voxelSSBO,VkBuffer oneNodeSSBO, VulkanTexture* blockTextureArray ) {
	VkDescriptorImageInfo outputImageInfo{
		.sampler = VK_NULL_HANDLE,
		.imageView = outputImageView,
		.imageLayout = VK_IMAGE_LAYOUT_GENERAL,
	};
	VkDescriptorBufferInfo uniformBufferInfo{
		.buffer = uniformBuffer,
		.offset = 0,
		.range = VK_WHOLE_SIZE,
	};
	VkDescriptorBufferInfo chunkSSBOInfo{
		.buffer = chunkSSBO,
		.offset = 0,
		.range = VK_WHOLE_SIZE,
	};
	VkDescriptorBufferInfo voxelSSBOInfo{
		.buffer = voxelSSBO,
		.offset = 0,
		.range = VK_WHOLE_SIZE,
	};
	VkDescriptorBufferInfo oneNodeSSBOInfo{
		.buffer = oneNodeSSBO,
		.offset = 0,
		.range = VK_WHOLE_SIZE,
	};
	VkDescriptorImageInfo blockTextureImageInfo{
		.sampler = blockTextureArray->sampler,
		.imageView = blockTextureArray->imageView,
		.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
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
	VkWriteDescriptorSet uniformBufferWrite{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.pNext = nullptr,
		.dstSet = descriptorSet,
		.dstBinding = 1,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.pImageInfo = nullptr,
		.pBufferInfo = &uniformBufferInfo,
		.pTexelBufferView = nullptr,
	};
	VkWriteDescriptorSet chunkSSBOWrite{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.pNext = nullptr,
		.dstSet = descriptorSet,
		.dstBinding = 2,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.pImageInfo = nullptr,
		.pBufferInfo = &chunkSSBOInfo,
		.pTexelBufferView = nullptr,
	};
	VkWriteDescriptorSet voxelSSBOWrite{
	.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
	.pNext = nullptr,
	.dstSet = descriptorSet,
	.dstBinding = 3,
	.dstArrayElement = 0,
	.descriptorCount = 1,
	.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
	.pImageInfo = nullptr,
	.pBufferInfo = &voxelSSBOInfo,
	.pTexelBufferView = nullptr,
	};
	VkWriteDescriptorSet oneNodeSSBOWrite{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.pNext = nullptr,
		.dstSet = descriptorSet,
		.dstBinding = 4,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.pImageInfo = nullptr,
		.pBufferInfo = &oneNodeSSBOInfo,
		.pTexelBufferView = nullptr,
	};
	VkWriteDescriptorSet blockTextureWrite{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.pNext = nullptr,
		.dstSet = descriptorSet,
		.dstBinding = 5,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.pImageInfo = &blockTextureImageInfo,
		.pBufferInfo = nullptr,
		.pTexelBufferView = nullptr,
	};
	std::vector<VkWriteDescriptorSet> descriptorWrites = { writeDescriptorSet, uniformBufferWrite,chunkSSBOWrite,voxelSSBOWrite,oneNodeSSBOWrite,blockTextureWrite };
	vkUpdateDescriptorSets(logicalDevice, (uint32_t)descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);

}
}