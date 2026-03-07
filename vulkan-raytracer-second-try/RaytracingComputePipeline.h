#pragma once
#include "ComputePipeline.h"
#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include "VulkanTexture.h"
#include "ChunkTypes.h"
namespace my_vulkan
{

	struct ComputeUniformData {
		glm::mat4 view;
		glm::mat4 proj;
		glm::vec3 cameraPos;
		uint32_t CHUNK_SIZE;
		uint32_t CHUNK_HEIGHT;
		ChunkTypes::blockData blockDataArray[256];
	};
	struct alignas(16) ChunkData {
		glm::vec2 chunkPos;
		uint32_t gpuSlot;
		uint32_t valid;
	};
	class RaytracingComputePipeline : public ComputePipeline
	{

	public:
		RaytracingComputePipeline();
		~RaytracingComputePipeline();
		void UpdateDescriptorSet(VkDescriptorSet descriptorSet, VkImageView outputImageView, VkBuffer uniformBuffer, VkBuffer chunkSSBO, VkBuffer voxelSSBO, VulkanTexture *blockTextureArray);
	protected:
		virtual VkDescriptorSetLayout CreateDescriptorSetLayout(VulkanCore& core) override;
	};
}