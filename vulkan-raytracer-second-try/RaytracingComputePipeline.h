#pragma once
#include "ComputePipeline.h"
#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
namespace my_vulkan
{
	struct ComputeUniformData {
		glm::mat4 view;
		glm::mat4 proj;
		glm::vec3 cameraPos;
	};
	class RaytracingComputePipeline : public ComputePipeline
	{

	public:
		RaytracingComputePipeline();
		~RaytracingComputePipeline();
		void UpdateDescriptorSet(VkDescriptorSet descriptorSet, VkImageView outputImageView, VkBuffer uniformBuffer);
	protected:
		virtual VkDescriptorSetLayout CreateDescriptorSetLayout(VulkanCore& core) override;
	};
}