#pragma once
#include "ComputePipeline.h"
#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <cstddef>
#include "VulkanTexture.h"
#include "BlockDataManager.h"
using namespace glm;
namespace my_vulkan
{
	struct alignas(16) Std140Uint64 {
		uint64_t value;
		uint64_t pad;
	};

	struct RaytracingUniformData {
		mat4 view;
		mat4 proj;
		mat4 previousView;
		mat4 previousProj;
		vec3 cameraPos;
		uint32_t CHUNK_SIZE;
		blockData blockDataArray[256];
		Std140Uint64 directionMasks[512];
		alignas(16) ivec3 allChunksMin;
		alignas(16) ivec3 allChunksMax;
		alignas(16) vec2 jitter;

	};
	struct RaytracingUniformUpdateData {
		mat4 view;
		mat4 proj;
		vec3 cameraPos;
		ivec3 allChunksMin;
		ivec3 allChunksMax;
		vec2 jitter;

	};

	struct RaytracingPushConstants {
		uint32_t beamResolutionDivisor;
		uint32_t computeToggle;
		uint32_t isBeam;
	};

	struct alignas(16) ChunkData {
		vec3 chunkPos;
		uint32_t gpuSlot;
		uint32_t valid;
		uint32_t oneNodeSlot;
	};

	class RaytracingComputePipeline : public ComputePipeline
	{
	public:
		RaytracingComputePipeline();
		~RaytracingComputePipeline();
		void UpdateDescriptorSet(VkDescriptorSet descriptorSet, VkImageView outputImageView, VkImageView beamOutputImageView, VkBuffer uniformBuffer, VkBuffer chunkSSBO, VkBuffer voxelSSBO, VkBuffer oneNodeSSBO, VulkanTexture* blockTextureArray, VkImageView velocityImageView);
		RaytracingUniformData* GetUniformData() { return &uniformData; }
		void updateUniformData(RaytracingUniformUpdateData uniformUpdateData) {
			uniformData.previousView = uniformData.view;
			uniformData.previousProj = uniformData.proj;
			uniformData.view = uniformUpdateData.view;
			uniformData.proj = uniformUpdateData.proj;
			uniformData.cameraPos = uniformUpdateData.cameraPos;
			uniformData.allChunksMin = uniformUpdateData.allChunksMin;
			uniformData.allChunksMax = uniformUpdateData.allChunksMax;
			uniformData.jitter = uniformUpdateData.jitter;
		}
		vec2 haltonSamples[64];
		vec2 getJitter(uint32_t frameCounter) {
			return haltonSamples[frameCounter % 64]-vec2(0.5,0.5);
		}
	protected:
		virtual VkDescriptorSetLayout CreateDescriptorSetLayout(VulkanCore& core) override;
		virtual uint32_t GetPushConstantSize() const override { return sizeof(RaytracingPushConstants); }
		RaytracingUniformData uniformData;

	};
}