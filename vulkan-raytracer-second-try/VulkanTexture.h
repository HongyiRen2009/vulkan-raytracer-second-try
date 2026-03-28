#pragma once
#include <vulkan/vulkan.h>
#include "VulkanPhysicalDevices.h"
#include "VulkanQueue.h"
#include "BufferAndMemory.h"
#include "VulkanUtils.h"
namespace my_vulkan
{
	class VulkanTexture
	{
	public:
		VulkanTexture();
		~VulkanTexture();

		VkImage image;
		VkImageView imageView;
		VkDeviceMemory imageMemory;
		VkSampler sampler;

		void Init(VkDevice device, VkCommandBuffer cmdBuffer, VulkanQueue* q, VulkanPhysicalDevices* physDevices);
		void CreateTexture(const char* filename);
		void CreateTextureArray(const char* filename, uint32_t tileWidth, uint32_t tileHeight);
		void UpdateTextureImage(const void* pixelData, int width, int height, VkFormat imageFormat);
		void Cleanup(VkDevice device);

	private:
		VkDevice logicalDevice;
		VkCommandBuffer copyCommandBuffer;
		VulkanQueue* queue;
		VulkanPhysicalDevices* physicalDevices;

		void CreateTextureFromData(const void* pixelData, int width, int height, VkFormat imageFormat);
		void CreateImage(const void* pixelData, int width, int height, VkFormat imageFormat, VkImageUsageFlagBits usageFlags, VkMemoryPropertyFlagBits memoryProperties);
		void CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
		void CreateArrayImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t layerCount);
		void CreateSampler(VkFilter minFilter, VkFilter magFilter, VkSamplerAddressMode addressMode);
		void CreateTextureArrayImage(const void* atlasData, uint32_t atlasWidth, uint32_t atlasHeight, uint32_t tileWidth, uint32_t tileHeight, uint32_t tilesX, uint32_t tilesY, uint32_t layerCount, VkFormat imageFormat);
		void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t layerCount = 1,uint32_t mipLevels = 1);
		void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
		void CopyBufferToImageArray(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layerCount);
		uint32_t GetMemoryTypeIndex(uint32_t typeFilter, VkMemoryPropertyFlags properties);
		void BeginCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferUsageFlags usageFlags);
		uint32_t CalculateMipLevels(uint32_t width, uint32_t height);
		void GenerateMipmaps(VkImage image, VkFormat imageFormat, uint32_t width, uint32_t height, uint32_t mipLevels);
		void GenerateMipmapsArray(VkImage image, VkFormat imageFormat, uint32_t width, uint32_t height, uint32_t layerCount, uint32_t mipLevels);
		void CopyBufferToImage(const void* pixelData, VkImage image, uint32_t width, uint32_t height, VkFormat imageFormat);
	};
}