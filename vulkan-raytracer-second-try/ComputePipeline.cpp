#include "ComputePipeline.h"

namespace my_vulkan
{
	ComputePipeline::ComputePipeline() {
	}
	ComputePipeline::~ComputePipeline() {
	}
	void ComputePipeline::Init(VulkanCore& core, VkDescriptorPool DescriptorPool, const char* ComputeShaderFileName) {
		logicalDevice = core.GetLogicalDevice();
		descriptorPool = DescriptorPool;
		descriptorSetLayout = CreateDescriptorSetLayout(core);
		CreatePipelineLayout();

		ComputeShaderModule = VulkanUtils::CreateShaderModule(logicalDevice, ComputeShaderFileName);
		CreatePipeline(ComputeShaderModule);
	}
	void ComputePipeline::CreatePipeline(VkShaderModule computeShader) {
		VkPipelineShaderStageCreateInfo shaderStageInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.stage = VK_SHADER_STAGE_COMPUTE_BIT,
			.module = computeShader,
			.pName = "main",
			.pSpecializationInfo = nullptr,
		};
		VkComputePipelineCreateInfo pipelineCreateInfo{
			.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.stage = shaderStageInfo,
			.layout = pipelineLayout,
			.basePipelineHandle = VK_NULL_HANDLE,
			.basePipelineIndex = -1,
		};
		VkResult result = vkCreateComputePipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline);
		CHECK_VK_RESULT(result,"vkCreateComputePipelines");
	}
	void ComputePipeline::CreatePipelineLayout() {
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.setLayoutCount = 1,
			.pSetLayouts = &descriptorSetLayout,
			.pushConstantRangeCount = 0,
			.pPushConstantRanges = nullptr,
		};
		VkResult result = vkCreatePipelineLayout(logicalDevice, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout);
		CHECK_VK_RESULT(result,"vkCreatePipelineLayout");
	}
	void ComputePipeline::RecordCommandBuffer(VkCommandBuffer commandBuffer, VkDescriptorSet DescriptorSet, uint32_t GroupCountX, uint32_t GroupCountY, uint32_t GroupCountZ) {
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &DescriptorSet, 0, nullptr);
		vkCmdDispatch(commandBuffer, GroupCountX, GroupCountY, GroupCountZ);
	}
	void ComputePipeline::AllocateDescriptorSets(int DescriptorCount, vector<VkDescriptorSet>& DescriptorSets) {
		assert(DescriptorCount > 0);
		DescriptorSets.resize(DescriptorCount);
		vector<VkDescriptorSetLayout> layouts(DescriptorCount, descriptorSetLayout);
		VkDescriptorSetAllocateInfo allocInfo{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.pNext = nullptr,
			.descriptorPool = descriptorPool,
			.descriptorSetCount = (uint32_t)DescriptorCount,
			.pSetLayouts = layouts.data(),
		};
		VkResult result = vkAllocateDescriptorSets(logicalDevice, &allocInfo, DescriptorSets.data());
		CHECK_VK_RESULT(result,"vkAllocateDescriptorSets");
	}
	void ComputePipeline::Cleanup() {
		if (pipeline != VK_NULL_HANDLE) {
			vkDestroyPipeline(logicalDevice, pipeline, nullptr);
			pipeline = VK_NULL_HANDLE;
		}
		if (pipelineLayout != VK_NULL_HANDLE) {
			vkDestroyPipelineLayout(logicalDevice, pipelineLayout, nullptr);
			pipelineLayout = VK_NULL_HANDLE;
		}
		if (ComputeShaderModule != VK_NULL_HANDLE) {
			vkDestroyShaderModule(logicalDevice, ComputeShaderModule, nullptr);
			ComputeShaderModule = VK_NULL_HANDLE;
		}
		if (descriptorSetLayout != VK_NULL_HANDLE) {
			vkDestroyDescriptorSetLayout(logicalDevice, descriptorSetLayout, nullptr);
			descriptorSetLayout = VK_NULL_HANDLE;
		}
	}
}