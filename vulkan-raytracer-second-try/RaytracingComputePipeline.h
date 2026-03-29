#pragma once
#include "ComputePipeline.h"
#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <cstddef>
#include "VulkanTexture.h"
#include "ChunkTypes.h"

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
		ChunkTypes::blockData blockDataArray[256];
		Std140Uint64 twoWideRegionMasks[27];
		alignas(16) glm::ivec3 allChunksMin;
		alignas(16) glm::ivec3 allChunksMax;
	};

	static_assert(offsetof(ComputeUniformData, allChunksMin) == 8768, "UBO layout mismatch");
	static_assert(sizeof(ComputeUniformData) == 8800, "UBO size mismatch");

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

	protected:
		virtual VkDescriptorSetLayout CreateDescriptorSetLayout(VulkanCore& core) override;
	};
}