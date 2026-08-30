#include "VulkanTexture.h"
#define STB_IMAGE_IMPLEMENTATION
#include "include/stb_image.h"

namespace my_vulkan
{
	VulkanTexture::VulkanTexture() : logicalDevice(VK_NULL_HANDLE), copyCommandBuffer(VK_NULL_HANDLE), queue(nullptr), physicalDevices(nullptr) {
	}

	VulkanTexture::~VulkanTexture() {
	}

	void VulkanTexture::Init(VkDevice device, VkCommandBuffer copyCommandBuffer, VulkanQueue* q, VulkanPhysicalDevices* physDevices) {
		logicalDevice = device;
		this->copyCommandBuffer = copyCommandBuffer;
		queue = q;
		physicalDevices = physDevices;
	}

	void VulkanTexture::BeginCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferUsageFlags usageFlags) {
		VkResult result = vkResetCommandBuffer(commandBuffer, 0);
		CHECK_VK_RESULT(result, "vkResetCommandBuffer");
		VkCommandBufferBeginInfo beginInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.pNext = nullptr,
			.flags = usageFlags,
			.pInheritanceInfo = nullptr,
		};
		result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
		CHECK_VK_RESULT(result, "vkBeginCommandBuffer");
	}

	uint32_t VulkanTexture::CalculateMipLevels(uint32_t width, uint32_t height) {
		uint32_t maxDim = (width > height) ? width : height;
		uint32_t mipLevels = 1;
		while (maxDim > 1) {
			maxDim >>= 1;
			mipLevels++;
		}
		return mipLevels;
	}

	void VulkanTexture::CreateTexture(const char* filename) {
		int imageWidth, imageHeight, imageChannels;
		stbi_uc* imageData = stbi_load(filename, &imageWidth, &imageHeight, &imageChannels, STBI_rgb_alpha);
		if (!imageData) {
			throw std::runtime_error("Failed to load texture image: " + std::string(filename));
		}
		VkFormat imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
		CreateTextureFromData(imageData, imageWidth, imageHeight, imageFormat);
		stbi_image_free(imageData);
		VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
		CreateImageView(image, imageFormat, aspectFlags);
		VkFilter MinFilter = VK_FILTER_NEAREST;
		VkFilter MagFilter = VK_FILTER_NEAREST;
		VkSamplerAddressMode AddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		CreateSampler(MinFilter, MagFilter, AddressMode);
	}

	void VulkanTexture::CreateTextureFromData(const void* pixelData, int width, int height, VkFormat imageFormat) {
		VkImageUsageFlagBits usageFlags = (VkImageUsageFlagBits)(VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
		VkMemoryPropertyFlagBits memoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		CreateImage(pixelData, width, height, imageFormat, usageFlags, memoryProperties);
	}

	void VulkanTexture::CreateImage(const void* pixelData, int width, int height, VkFormat imageFormat, VkImageUsageFlagBits usageFlags, VkMemoryPropertyFlagBits memoryProperties) {
		uint32_t mipLevels = CalculateMipLevels(width, height);

		VkImageCreateInfo imageCreateInfo{
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.imageType = VK_IMAGE_TYPE_2D,
			.format = imageFormat,
			.extent = { (uint32_t)width, (uint32_t)height, 1 },
			.mipLevels = mipLevels,
			.arrayLayers = 1,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.tiling = VK_IMAGE_TILING_OPTIMAL,
			.usage = (VkImageUsageFlags)usageFlags,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount = 0,
			.pQueueFamilyIndices = nullptr,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		};
		VkResult result = vkCreateImage(logicalDevice, &imageCreateInfo, nullptr, &image);
		CHECK_VK_RESULT(result, "vkCreateImage");
		VkMemoryRequirements memRequirements = { 0 };
		vkGetImageMemoryRequirements(logicalDevice, image, &memRequirements);
		uint32_t memoryTypeIndex = GetMemoryTypeIndex(memRequirements.memoryTypeBits, memoryProperties);
		VkMemoryAllocateInfo allocInfo{
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.pNext = nullptr,
			.allocationSize = memRequirements.size,
			.memoryTypeIndex = memoryTypeIndex,
		};
		result = vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &imageMemory);
		CHECK_VK_RESULT(result, "vkAllocateMemory");
		result = vkBindImageMemory(logicalDevice, image, imageMemory, 0);
		CHECK_VK_RESULT(result, "vkBindImageMemory");

		// Upload base level and generate mipmaps
		TransitionImageLayout(image, imageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, mipLevels);
		CopyBufferToImage(pixelData, image, width, height, imageFormat);
		GenerateMipmaps(image, imageFormat, width, height, mipLevels);
	}

	void VulkanTexture::CreateImageView(VkImage img, VkFormat format, VkImageAspectFlags aspectFlags) {
		VkImageViewCreateInfo imageViewCreateInfo{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.image = img,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = format,
			.components = {
				.r = VK_COMPONENT_SWIZZLE_IDENTITY,
				.g = VK_COMPONENT_SWIZZLE_IDENTITY,
				.b = VK_COMPONENT_SWIZZLE_IDENTITY,
				.a = VK_COMPONENT_SWIZZLE_IDENTITY,
			},
			.subresourceRange = {
				.aspectMask = aspectFlags,
				.baseMipLevel = 0,
				.levelCount = VK_REMAINING_MIP_LEVELS,
				.baseArrayLayer = 0,
				.layerCount = 1,
			},
		};
		VkResult result = vkCreateImageView(logicalDevice, &imageViewCreateInfo, nullptr, &imageView);
		CHECK_VK_RESULT(result, "vkCreateImageView");
	}

	void VulkanTexture::CreateArrayImageView(VkImage img, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t layerCount) {
		VkImageViewCreateInfo imageViewCreateInfo{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.image = img,
			.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY,
			.format = format,
			.components = {
				.r = VK_COMPONENT_SWIZZLE_IDENTITY,
				.g = VK_COMPONENT_SWIZZLE_IDENTITY,
				.b = VK_COMPONENT_SWIZZLE_IDENTITY,
				.a = VK_COMPONENT_SWIZZLE_IDENTITY,
			},
			.subresourceRange = {
				.aspectMask = aspectFlags,
				.baseMipLevel = 0,
				.levelCount = VK_REMAINING_MIP_LEVELS,
				.baseArrayLayer = 0,
				.layerCount = layerCount,
			},
		};
		VkResult result = vkCreateImageView(logicalDevice, &imageViewCreateInfo, nullptr, &imageView);
		CHECK_VK_RESULT(result, "vkCreateImageView");
	}

	void VulkanTexture::CreateSampler(VkFilter minFilter, VkFilter magFilter, VkSamplerAddressMode addressMode) {
		VkSamplerCreateInfo samplerCreateInfo{
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.magFilter = magFilter,
			.minFilter = minFilter,
			.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
			.addressModeU = addressMode,
			.addressModeV = addressMode,
			.addressModeW = addressMode,
			.mipLodBias = 0.0f,
			.anisotropyEnable = VK_TRUE,
			.maxAnisotropy = 8.0f,
			.compareEnable = VK_FALSE,
			.compareOp = VK_COMPARE_OP_ALWAYS,
			.minLod = 0.0f,
			.maxLod = 16.0f,
			.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
			.unnormalizedCoordinates = VK_FALSE,
		};
		VkResult result = vkCreateSampler(logicalDevice, &samplerCreateInfo, nullptr, &sampler);
		CHECK_VK_RESULT(result, "vkCreateSampler");
	}

	int GetBytesPerPixel(VkFormat format) {
		switch (format) {
		case VK_FORMAT_R8G8B8A8_UNORM:
			return 4;
		case VK_FORMAT_R16G16B16_SFLOAT:
			return 6;
		case VK_FORMAT_R32G32B32A32_SFLOAT:
			return 16;
		case VK_FORMAT_R8G8B8_UNORM:
			return 3;
		case VK_FORMAT_R16G16_SFLOAT:
			return 4;
		case VK_FORMAT_R32G32_SFLOAT:
			return 8;
		case VK_FORMAT_R8_UNORM:
			return 1;
		case VK_FORMAT_R16_SFLOAT:
			return 2;
		case VK_FORMAT_R32_SFLOAT:
			return 4;
		case VK_FORMAT_B8G8R8A8_UNORM:
			return 4;
		default:
			throw std::runtime_error("Unsupported image format for bytes per pixel calculation");
		}
	}

	void VulkanTexture::UpdateTextureImage(const void* pixelData, int width, int height, VkFormat imageFormat) {
		int BytesPerPixel = GetBytesPerPixel(imageFormat);
		VkDeviceSize layerSize = width * height * BytesPerPixel;
		int layerCount = 1;
		VkDeviceSize imageSize = layerSize * layerCount;
		VkImageUsageFlags usageFlags = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		VkMemoryPropertyFlags memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		BufferAndMemory stagingBuffer;
		stagingBuffer.CreateBuffer(physicalDevices->GetSelectedPhysicalDevice().device, logicalDevice, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, memoryProperties);
		stagingBuffer.AddData(logicalDevice, pixelData, (size_t)imageSize);
		TransitionImageLayout(image, imageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		CopyBufferToImage(pixelData, image, width, height, imageFormat);
		TransitionImageLayout(image, imageFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		stagingBuffer.Cleanup(logicalDevice);
	}

	void VulkanTexture::TransitionImageLayout(VkImage img, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t layerCount, uint32_t mipLevels) {
		VkCommandBuffer commandBuffer = copyCommandBuffer;
		BeginCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

		VkImageMemoryBarrier barrier{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.pNext = nullptr,
			.srcAccessMask = 0,
			.dstAccessMask = 0,
			.oldLayout = oldLayout,
			.newLayout = newLayout,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = img,
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = mipLevels,
				.baseArrayLayer = 0,
				.layerCount = layerCount,
			},
		};

		VkPipelineStageFlags srcStage = 0;
		VkPipelineStageFlags dstStage = 0;

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			dstStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else {
			throw std::invalid_argument("Unsupported layout transition!");
		}

		vkCmdPipelineBarrier(
			commandBuffer,
			srcStage, dstStage,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);

		VkResult result = vkEndCommandBuffer(commandBuffer);
		CHECK_VK_RESULT(result, "vkEndCommandBuffer");

		VkSubmitInfo submitInfo{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = nullptr,
			.waitSemaphoreCount = 0,
			.pWaitSemaphores = nullptr,
			.pWaitDstStageMask = nullptr,
			.commandBufferCount = 1,
			.pCommandBuffers = &commandBuffer
		};
		result = vkQueueSubmit(queue->GetQueue(), 1, &submitInfo, VK_NULL_HANDLE);
		CHECK_VK_RESULT(result, "vkQueueSubmit");
		vkQueueWaitIdle(queue->GetQueue());
	}

	void VulkanTexture::GenerateMipmaps(VkImage image, VkFormat imageFormat, uint32_t width, uint32_t height, uint32_t mipLevels) {
		VkCommandBuffer commandBuffer = copyCommandBuffer;
		BeginCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

		VkImageMemoryBarrier barrier{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.pNext = nullptr,
			.srcAccessMask = 0,
			.dstAccessMask = 0,
			.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.newLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = image,
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1,
			},
		};

		for (uint32_t i = 1; i < mipLevels; i++) {
			barrier.subresourceRange.baseMipLevel = i - 1;
			barrier.oldLayout = (i == 1) ? VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL : VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			vkCmdPipelineBarrier(
				commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);

			barrier.subresourceRange.baseMipLevel = i;
			barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			vkCmdPipelineBarrier(
				commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);

			VkImageBlit blit{
				.srcSubresource = {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.mipLevel = i - 1,
					.baseArrayLayer = 0,
					.layerCount = 1,
				},
				.srcOffsets = {
					{0, 0, 0},
					{(int32_t)(width >> (i - 1)), (int32_t)(height >> (i - 1)), 1}
				},
				.dstSubresource = {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.mipLevel = i,
					.baseArrayLayer = 0,
					.layerCount = 1,
				},
				.dstOffsets = {
					{0, 0, 0},
					{(int32_t)(width >> i), (int32_t)(height >> i), 1}
				},
			};

			vkCmdBlitImage(
				commandBuffer,
				image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &blit,
				VK_FILTER_NEAREST
			);
		}

		if (mipLevels > 1) {
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = mipLevels - 1;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(
				commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);

			barrier.subresourceRange.baseMipLevel = mipLevels - 1;
			barrier.subresourceRange.levelCount = 1;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(
				commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);
		}
		else {
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = 1;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(
				commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);
		}

		VkResult result = vkEndCommandBuffer(commandBuffer);
		CHECK_VK_RESULT(result, "vkEndCommandBuffer");

		VkSubmitInfo submitInfo{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = nullptr,
			.waitSemaphoreCount = 0,
			.pWaitSemaphores = nullptr,
			.pWaitDstStageMask = nullptr,
			.commandBufferCount = 1,
			.pCommandBuffers = &commandBuffer
		};
		result = vkQueueSubmit(queue->GetQueue(), 1, &submitInfo, VK_NULL_HANDLE);
		CHECK_VK_RESULT(result, "vkQueueSubmit");
		vkQueueWaitIdle(queue->GetQueue());
	}

	void VulkanTexture::GenerateMipmapsArray(VkImage image, VkFormat imageFormat, uint32_t width, uint32_t height, uint32_t layerCount, uint32_t mipLevels) {
		VkCommandBuffer commandBuffer = copyCommandBuffer;
		BeginCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

		VkImageMemoryBarrier barrier{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.pNext = nullptr,
			.srcAccessMask = 0,
			.dstAccessMask = 0,
			.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.newLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = image,
			.subresourceRange = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = layerCount,
			},
		};

		for (uint32_t i = 1; i < mipLevels; i++) {
			barrier.subresourceRange.baseMipLevel = i - 1;
			barrier.oldLayout = (i == 1) ? VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL : VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			vkCmdPipelineBarrier(
				commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);

			barrier.subresourceRange.baseMipLevel = i;
			barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			vkCmdPipelineBarrier(
				commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);

			VkImageBlit blit{
				.srcSubresource = {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.mipLevel = i - 1,
					.baseArrayLayer = 0,
					.layerCount = layerCount,
				},
				.srcOffsets = {
					{0, 0, 0},
					{(int32_t)(width >> (i - 1)), (int32_t)(height >> (i - 1)), 1}
				},
				.dstSubresource = {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.mipLevel = i,
					.baseArrayLayer = 0,
					.layerCount = layerCount,
				},
				.dstOffsets = {
					{0, 0, 0},
					{(int32_t)(width >> i), (int32_t)(height >> i), 1}
				},
			};

			vkCmdBlitImage(
				commandBuffer,
				image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &blit,
				VK_FILTER_NEAREST
			);
		}

		if (mipLevels > 1) {
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = mipLevels - 1;
			barrier.subresourceRange.layerCount = layerCount;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(
				commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);

			barrier.subresourceRange.baseMipLevel = mipLevels - 1;
			barrier.subresourceRange.levelCount = 1;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(
				commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);
		}
		else {
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = 1;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(
				commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);
		}

		VkResult result = vkEndCommandBuffer(commandBuffer);
		CHECK_VK_RESULT(result, "vkEndCommandBuffer");

		VkSubmitInfo submitInfo{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = nullptr,
			.waitSemaphoreCount = 0,
			.pWaitSemaphores = nullptr,
			.pWaitDstStageMask = nullptr,
			.commandBufferCount = 1,
			.pCommandBuffers = &commandBuffer
		};
		result = vkQueueSubmit(queue->GetQueue(), 1, &submitInfo, VK_NULL_HANDLE);
		CHECK_VK_RESULT(result, "vkQueueSubmit");
		vkQueueWaitIdle(queue->GetQueue());
	}


	void VulkanTexture::CopyBufferToImage(const void* pixelData, VkImage img, uint32_t width, uint32_t height, VkFormat imageFormat) {
		int bytesPerPixel = GetBytesPerPixel(imageFormat);
		VkDeviceSize imageSize = width * height * bytesPerPixel;
		VkMemoryPropertyFlags memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		BufferAndMemory stagingBuffer;
		stagingBuffer.CreateBuffer(physicalDevices->GetSelectedPhysicalDevice().device, logicalDevice, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, memoryProperties);
		stagingBuffer.AddData(logicalDevice, pixelData, (size_t)imageSize);

		BeginCommandBuffer(copyCommandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		VkBufferImageCopy region{
			.bufferOffset = 0,
			.bufferRowLength = 0,
			.bufferImageHeight = 0,
			.imageSubresource = {
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.mipLevel = 0,
				.baseArrayLayer = 0,
				.layerCount = 1,
			},
			.imageOffset = { 0, 0, 0 },
			.imageExtent = { width, height, 1 },
		};
		vkCmdCopyBufferToImage(
			copyCommandBuffer,
			stagingBuffer.buffer,
			img,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&region
		);
		VkResult result = vkEndCommandBuffer(copyCommandBuffer);
		CHECK_VK_RESULT(result, "vkEndCommandBuffer");
		VkSubmitInfo submitInfo{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = nullptr,
			.waitSemaphoreCount = 0,
			.pWaitSemaphores = nullptr,
			.pWaitDstStageMask = nullptr,
			.commandBufferCount = 1,
			.pCommandBuffers = &copyCommandBuffer
		};
		result = vkQueueSubmit(queue->GetQueue(), 1, &submitInfo, VK_NULL_HANDLE);
		CHECK_VK_RESULT(result, "vkQueueSubmit");
		vkQueueWaitIdle(queue->GetQueue());
		stagingBuffer.Cleanup(logicalDevice);
	}

	void VulkanTexture::CopyBufferToImageArray(VkBuffer buffer, VkImage img, uint32_t width, uint32_t height, uint32_t layerCount) {
		BeginCommandBuffer(copyCommandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

		for (uint32_t layer = 0; layer < layerCount; ++layer) {
			VkBufferImageCopy region{
				.bufferOffset = layer * width * height * 4,
				.bufferRowLength = 0,
				.bufferImageHeight = 0,
				.imageSubresource = {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.mipLevel = 0,
					.baseArrayLayer = layer,
					.layerCount = 1,
				},
				.imageOffset = { 0, 0, 0 },
				.imageExtent = { width, height, 1 },
			};

			vkCmdCopyBufferToImage(
				copyCommandBuffer,
				buffer,
				img,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1,
				&region
			);
		}

		VkResult result = vkEndCommandBuffer(copyCommandBuffer);
		CHECK_VK_RESULT(result, "vkEndCommandBuffer");

		VkSubmitInfo submitInfo{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = nullptr,
			.waitSemaphoreCount = 0,
			.pWaitSemaphores = nullptr,
			.pWaitDstStageMask = nullptr,
			.commandBufferCount = 1,
			.pCommandBuffers = &copyCommandBuffer
		};

		result = vkQueueSubmit(queue->GetQueue(), 1, &submitInfo, VK_NULL_HANDLE);
		CHECK_VK_RESULT(result, "vkQueueSubmit");
		vkQueueWaitIdle(queue->GetQueue());
	}

	void VulkanTexture::CreateTextureArray(const char* filename, uint32_t tileWidth, uint32_t tileHeight) {
		int atlasWidth, atlasHeight, imageChannels;
		const char* basePath = "assets/";
		stbi_uc* atlasData = stbi_load((std::string(basePath) + filename).c_str(), &atlasWidth, &atlasHeight, &imageChannels, STBI_rgb_alpha);
		if (!atlasData) {
			throw std::runtime_error("Failed to load texture atlas: " + std::string(filename));
		}

		uint32_t tilesX = atlasWidth / tileWidth;
		uint32_t tilesY = atlasHeight / tileHeight;
		uint32_t layerCount = tilesX * tilesY;

		if (atlasWidth % tileWidth != 0 || atlasHeight % tileHeight != 0) {
			stbi_image_free(atlasData);
			throw std::runtime_error("Atlas dimensions must be divisible by tile dimensions");
		}

		VkFormat imageFormat = VK_FORMAT_B8G8R8A8_UNORM;
		CreateTextureArrayImage(atlasData, atlasWidth, atlasHeight, tileWidth, tileHeight, tilesX, tilesY, layerCount, imageFormat);

		stbi_image_free(atlasData);

		CreateArrayImageView(image, imageFormat, VK_IMAGE_ASPECT_COLOR_BIT, layerCount);

		VkFilter MinFilter = VK_FILTER_NEAREST;
		VkFilter MagFilter = VK_FILTER_NEAREST;
		VkSamplerAddressMode AddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		CreateSampler(MinFilter, MagFilter, AddressMode);
	}
	void VulkanTexture::CreateTextureArrayImage(const void* atlasData, uint32_t atlasWidth, uint32_t atlasHeight, uint32_t tileWidth, uint32_t tileHeight, uint32_t tilesX, uint32_t tilesY, uint32_t layerCount, VkFormat imageFormat) {
		int bytesPerPixel = GetBytesPerPixel(imageFormat);
		VkDeviceSize tileSize = tileWidth * tileHeight * bytesPerPixel;
		VkDeviceSize imageSize = tileSize * layerCount;

		VkMemoryPropertyFlags memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		BufferAndMemory stagingBuffer;
		stagingBuffer.CreateBuffer(physicalDevices->GetSelectedPhysicalDevice().device, logicalDevice, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, memoryProperties);

		uint8_t* stagingData = new uint8_t[imageSize];
		uint32_t layerIndex = 0;

		for (uint32_t y = 0; y < tilesY; ++y) {
			for (uint32_t x = 0; x < tilesX; ++x) {
				uint32_t offsetX = x * tileWidth;
				uint32_t offsetY = y * tileHeight;

				for (uint32_t py = 0; py < tileHeight; ++py) {
					uint32_t atlasRow = (offsetY + py) * atlasWidth;
					uint32_t stagingRow = py * tileWidth;

					size_t srcOffset = (atlasRow + offsetX) * bytesPerPixel;
					size_t dstOffset = (layerIndex * tileSize) + (stagingRow * bytesPerPixel);
					size_t copySize = tileWidth * bytesPerPixel;

					memcpy(&stagingData[dstOffset], &((uint8_t*)atlasData)[srcOffset], copySize);
				}

				layerIndex++;
			}
		}

		stagingBuffer.AddData(logicalDevice, stagingData, imageSize);
		delete[] stagingData;

		uint32_t mipLevels = CalculateMipLevels(tileWidth, tileHeight);

		VkImageCreateInfo imageCreateInfo{
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.imageType = VK_IMAGE_TYPE_2D,
			.format = imageFormat,
			.extent = { tileWidth, tileHeight, 1 },
			.mipLevels = mipLevels,
			.arrayLayers = layerCount,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.tiling = VK_IMAGE_TILING_OPTIMAL,
			.usage = (VkImageUsageFlagBits)(VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT),
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount = 0,
			.pQueueFamilyIndices = nullptr,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		};

		VkResult result = vkCreateImage(logicalDevice, &imageCreateInfo, nullptr, &image);
		CHECK_VK_RESULT(result, "vkCreateImage");

		VkMemoryRequirements memRequirements = { 0 };
		vkGetImageMemoryRequirements(logicalDevice, image, &memRequirements);
		uint32_t memoryTypeIndex = GetMemoryTypeIndex(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		VkMemoryAllocateInfo allocInfo{
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.pNext = nullptr,
			.allocationSize = memRequirements.size,
			.memoryTypeIndex = memoryTypeIndex,
		};

		result = vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &imageMemory);
		CHECK_VK_RESULT(result, "vkAllocateMemory");

		result = vkBindImageMemory(logicalDevice, image, imageMemory, 0);
		CHECK_VK_RESULT(result, "vkBindImageMemory");

		TransitionImageLayout(image, imageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, layerCount, mipLevels);
		CopyBufferToImageArray(stagingBuffer.buffer, image, tileWidth, tileHeight, layerCount);
		GenerateMipmapsArray(image, imageFormat, tileWidth, tileHeight, layerCount, mipLevels);

		stagingBuffer.Cleanup(logicalDevice);
	}

	uint32_t VulkanTexture::GetMemoryTypeIndex(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevices->GetSelectedPhysicalDevice().device, &memProperties);
		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}
		throw std::runtime_error("Failed to find suitable memory type!");
	}

	void VulkanTexture::Cleanup(VkDevice device) {
		if (sampler != VK_NULL_HANDLE) {
			vkDestroySampler(device, sampler, nullptr);
		}
		if (imageView != VK_NULL_HANDLE) {
			vkDestroyImageView(device, imageView, nullptr);
		}
		if (image != VK_NULL_HANDLE) {
			vkDestroyImage(device, image, nullptr);
		}
		if (imageMemory != VK_NULL_HANDLE) {
			vkFreeMemory(device, imageMemory, nullptr);
		}
	}
}
