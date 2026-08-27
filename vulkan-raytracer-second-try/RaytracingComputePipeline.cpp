#include "RaytracingComputePipeline.h"
#include <glm/glm.hpp>
#include "Chunk.h"
#include "BlockDataManager.h"
namespace my_vulkan
{
	float halton(int n, int base) {
		float f = 1.0, r = 0.0;
		while (n > 0) {
			f = f / float(base);
			r = r + f * float(n % base);
			n = int(floor(float(n) / float(base)));
		}
		return r;
	}
	RaytracingComputePipeline::RaytracingComputePipeline() {
		uint64_t directionMasks[512];
		glm::ivec3 directions[8];
		for (int i = -1;i <= 1;i += 2) {
			for(int j=-1;j<=1;j+=2){
				for(int k=-1;k<=1;k+=2){
					directions[((i+1)/2)*4 + ((j+1)/2)*2 + ((k+1)/2)] = glm::ivec3(i,j,k);
				}
			}
		}

		for (int i = 0; i < 512; i++) {
			uint64_t mask = 0;

			int directionIndex = i / 64;
			int positionIndex = i % 64;

			int posX = positionIndex / 16;
			int posY = (positionIndex / 4) % 4;
			int posZ = positionIndex % 4;

			glm::ivec3 dir = directions[directionIndex];
			for (int x = posX; x >= 0 && x < 4; x += dir.x) {
				for (int y = posY; y >= 0 && y < 4; y += dir.y) {
					for (int z = posZ; z >= 0 && z < 4; z += dir.z) {
						int bitIndex = x * 16 + y * 4 + z;
						mask |= (1ULL << bitIndex);
					}
				}
			}

			directionMasks[i] = mask;
		}
		
		for (int i = 0; i < 64; i++) {
			haltonSamples[i] = vec2(halton(i+2, 2), halton(i+2, 3));
		}	
		uniformData.CHUNK_SIZE = Chunk::CHUNK_SIZE;
		BlockDataManager::populateBlockDataArray();
		memcpy(uniformData.blockDataArray, BlockDataManager::blockDataArray, sizeof(BlockDataManager::blockDataArray));
		for (uint32_t i = 0; i < 512; i++) {
			uniformData.directionMasks[i].value = directionMasks[i];
			uniformData.directionMasks[i].pad = 0ull;
		}
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
	VkDescriptorSetLayoutBinding beamOutputImageBinding{
		.binding = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		.pImmutableSamplers = nullptr
	};
	VkDescriptorSetLayoutBinding depthImageBinding{
		.binding = 2,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		.pImmutableSamplers = nullptr
	};
	VkDescriptorSetLayoutBinding chunkSSBOBinding{
		.binding = 3,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		.pImmutableSamplers = nullptr,
	};
	VkDescriptorSetLayoutBinding voxelSSBOBinding{
		.binding = 4,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		.pImmutableSamplers = nullptr,
	};
	VkDescriptorSetLayoutBinding oneNodeSSBOBinding{
		.binding = 5,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		.pImmutableSamplers = nullptr,
	};
	VkDescriptorSetLayoutBinding blockTextureArrayBinding{
		.binding = 6,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		.pImmutableSamplers = nullptr,
	};
	VkDescriptorSetLayoutBinding velocityBufferBinding{
		.binding = 7,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		.descriptorCount =1,
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		.pImmutableSamplers = nullptr
	};
	VkDescriptorSetLayoutBinding uniformBufferBinding{
		.binding = 8,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		.pImmutableSamplers = nullptr,
	};
	VkDescriptorSetLayoutBinding previousDepthImageBinding{
		.binding = 9,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		.pImmutableSamplers = nullptr,
	};
	std::vector<VkDescriptorSetLayoutBinding> bindings = { outputImageBinding, beamOutputImageBinding, depthImageBinding, chunkSSBOBinding, voxelSSBOBinding, oneNodeSSBOBinding, blockTextureArrayBinding, velocityBufferBinding, uniformBufferBinding, previousDepthImageBinding };
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
void RaytracingComputePipeline::UpdateDescriptorSet(VkDescriptorSet descriptorSet, VkImageView outputImageView,VkImageView beamOutputImageView,VkImageView depthImageView,VkBuffer uniformBuffer,VkBuffer chunkSSBO, VkBuffer voxelSSBO,VkBuffer oneNodeSSBO, VulkanTexture* blockTextureArray,VkImageView velocityImageView, VkImageView previousDepthImageView) {
	VkDescriptorImageInfo outputImageInfo{
		.sampler = VK_NULL_HANDLE,
		.imageView = outputImageView,
		.imageLayout = VK_IMAGE_LAYOUT_GENERAL,
	};
	VkDescriptorImageInfo beamOutputImageInfo{
		.sampler = VK_NULL_HANDLE,
		.imageView = beamOutputImageView,
		.imageLayout = VK_IMAGE_LAYOUT_GENERAL
	};
	VkDescriptorImageInfo depthImageInfo{
		.sampler = VK_NULL_HANDLE,
		.imageView = depthImageView,
		.imageLayout = VK_IMAGE_LAYOUT_GENERAL
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
	VkDescriptorImageInfo velocityImageInfo{
	.sampler = VK_NULL_HANDLE,
	.imageView = velocityImageView,
		.imageLayout = VK_IMAGE_LAYOUT_GENERAL
	};
	VkDescriptorImageInfo previousDepthImageInfo{
		.sampler = VK_NULL_HANDLE,
		.imageView = previousDepthImageView,
		.imageLayout = VK_IMAGE_LAYOUT_GENERAL
	};
	VkWriteDescriptorSet computeOutputWrite{
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
	VkWriteDescriptorSet beamOutputWrite{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.pNext = nullptr,
		.dstSet = descriptorSet,
		.dstBinding = 1,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		.pImageInfo = &beamOutputImageInfo,
		.pBufferInfo = nullptr,
		.pTexelBufferView = nullptr,
	};
	VkWriteDescriptorSet depthImageWrite{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.pNext = nullptr,
		.dstSet = descriptorSet,
		.dstBinding = 2,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		.pImageInfo = &depthImageInfo,
		.pBufferInfo = nullptr,
		.pTexelBufferView = nullptr,
	};
	VkWriteDescriptorSet uniformBufferWrite{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.pNext = nullptr,
		.dstSet = descriptorSet,
		.dstBinding = 8,
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
		.dstBinding = 3,
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
	.dstBinding = 4,
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
		.dstBinding = 5,
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
		.dstBinding = 6,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.pImageInfo = &blockTextureImageInfo,
		.pBufferInfo = nullptr,
		.pTexelBufferView = nullptr,
	};
	VkWriteDescriptorSet velocityWrite{
	.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
	.pNext = nullptr,
	.dstSet = descriptorSet,
	.dstBinding = 7,
	.dstArrayElement = 0,
	.descriptorCount = 1,
	.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
	.pImageInfo = &velocityImageInfo,
	.pBufferInfo = nullptr,
	.pTexelBufferView = nullptr,
	};
	VkWriteDescriptorSet previousDepthWrite{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.pNext = nullptr,
		.dstSet = descriptorSet,
		.dstBinding = 9,
		.dstArrayElement = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		.pImageInfo = &previousDepthImageInfo,
		.pBufferInfo = nullptr,
		.pTexelBufferView = nullptr,
	};
	std::vector<VkWriteDescriptorSet> descriptorWrites = { computeOutputWrite, beamOutputWrite, depthImageWrite, chunkSSBOWrite, voxelSSBOWrite, oneNodeSSBOWrite, blockTextureWrite, velocityWrite, uniformBufferWrite, previousDepthWrite };
	vkUpdateDescriptorSets(logicalDevice, (uint32_t)descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);

}
}
