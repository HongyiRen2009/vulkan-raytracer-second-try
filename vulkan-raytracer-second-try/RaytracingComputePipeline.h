#pragma once
#include "ComputePipeline.h"
#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <cstddef>
#include "VulkanTexture.h"
#include "BlockDataManager.h"
namespace my_vulkan
{
	struct alignas(16) Std140Uint64 {
		uint64_t value;
		uint64_t pad;
	};

	struct ComputeUniformData {
		glm::mat4 view;
		glm::mat4 proj;
		glm::vec3 cameraPos;
		uint32_t CHUNK_SIZE;
		blockData blockDataArray[256];
		Std140Uint64 twoWideRegionMasks[27];
		Std140Uint64 directionMasks[512];
		alignas(16) glm::ivec3 allChunksMin;
		alignas(16) glm::ivec3 allChunksMax;
	};
	struct ComputeUniformUpdateData {
		glm::mat4 view;
		glm::mat4 proj;
		glm::vec3 cameraPos;
		glm::ivec3 allChunksMin;
		glm::ivec3 allChunksMax;
	};


	struct alignas(16) ChunkData {
		glm::vec3 chunkPos;
		uint32_t gpuSlot;
		uint32_t valid;
		uint32_t oneNodeSlot;
	};

	class RaytracingComputePipeline : public ComputePipeline
	{
	public:
		RaytracingComputePipeline();
		~RaytracingComputePipeline();
		void UpdateDescriptorSet(VkDescriptorSet descriptorSet, VkImageView outputImageView, VkImageView beamOutputImageView, VkBuffer uniformBuffer, VkBuffer chunkSSBO, VkBuffer voxelSSBO, VkBuffer oneNodeSSBO, VulkanTexture* blockTextureArray);
		uint64_t twoWideRegionMasks[27];
		uint64_t directionMasks[512];
		ComputeUniformData* GetUniformData()  { return &uniformData; }
		void updateUniformData(ComputeUniformUpdateData uniformUpdateData) {
			uniformData.view = uniformUpdateData.view;
			uniformData.proj = uniformUpdateData.proj;
			uniformData.cameraPos = uniformUpdateData.cameraPos;
			uniformData.allChunksMin = uniformUpdateData.allChunksMin;
			uniformData.allChunksMax = uniformUpdateData.allChunksMax;
		}
	protected:
		virtual VkDescriptorSetLayout CreateDescriptorSetLayout(VulkanCore& core) override;
		ComputeUniformData uniformData;

	};
}