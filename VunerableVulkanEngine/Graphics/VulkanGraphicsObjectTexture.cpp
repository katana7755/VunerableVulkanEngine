#include "VulkanGraphicsObjectTexture.h"
#include "../DebugUtility.h"
#include "VulkanGraphicsResourceDevice.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

void VulkanGraphicsObjectTexture::CreateAsDepthBuffer()
{
	m_Format = VK_FORMAT_D32_SFLOAT;
	m_Width = VulkanGraphicsResourceDevice::GetSurfaceCapabilities().currentExtent.width;
	m_Height = VulkanGraphicsResourceDevice::GetSurfaceCapabilities().currentExtent.height;
	m_MipLevel = 1;
	m_SampleCountBits = VK_SAMPLE_COUNT_1_BIT;
	m_Usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	m_AspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	VulkanGraphicsObjectBase::Create();
}

void VulkanGraphicsObjectTexture::CreateAsTexture(const char* strPngPath)
{
	int width, height, channelCount;
	auto pixelData = stbi_load(strPngPath, &width, &height, &channelCount, STBI_rgb_alpha);

	if (pixelData == NULL)
	{
		printf_console("[VulkanGraphics] failed to open the image located at %s\n", strPngPath);

		throw;
	}

	if (channelCount != STBI_rgb_alpha)
	{
		printf_console("[VulkanGraphics] the image located at %s doesn't have 4 channels instead of %d\n", strPngPath, channelCount);

		throw;
	}

	m_Format = VK_FORMAT_R8G8B8A8_SRGB;
	m_Width = (uint32_t)width;
	m_Height = (uint32_t)height;
	m_MipLevel = 1;
	m_SampleCountBits = VK_SAMPLE_COUNT_1_BIT;
	m_Usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	m_AspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	VulkanGraphicsObjectBase::Create();
	CreateStagingBuffer(pixelData, width * height * STBI_rgb_alpha);
}

void VulkanGraphicsObjectTexture::ApplyStagingBuffer(VkCommandBuffer& commandBuffer)
{
	if (m_IsStagingBufferExist == false)
	{
		return;
	}

	auto imageBarrier = VkImageMemoryBarrier();
	imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageBarrier.pNext = NULL;
	imageBarrier.srcAccessMask = 0;
	imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	imageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageBarrier.image = m_Image;
	imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageBarrier.subresourceRange.baseMipLevel = 0;
	imageBarrier.subresourceRange.levelCount = 1;
	imageBarrier.subresourceRange.baseArrayLayer = 0;
	imageBarrier.subresourceRange.layerCount = 1;
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &imageBarrier);

	auto copyInfo = VkBufferImageCopy();
	copyInfo.bufferOffset = 0;
	copyInfo.bufferRowLength = 0;
	copyInfo.bufferImageHeight = 0;
	copyInfo.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	copyInfo.imageSubresource.mipLevel = 0;
	copyInfo.imageSubresource.baseArrayLayer = 0;
	copyInfo.imageSubresource.layerCount = 1;
	copyInfo.imageOffset = { 0, 0, 0 };
	copyInfo.imageExtent = { m_Width, m_Height, 1 };
	vkCmdCopyBufferToImage(commandBuffer, m_StagingBuffer, m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyInfo);
}

void VulkanGraphicsObjectTexture::ClearStagingBuffer()
{
	m_IsStagingBufferExist = false;
	vkFreeMemory(VulkanGraphicsResourceDevice::GetLogicalDevice(), m_StagingMemory, NULL);
	vkDestroyBuffer(VulkanGraphicsResourceDevice::GetLogicalDevice(), m_StagingBuffer, NULL);
}

void VulkanGraphicsObjectTexture::CreateStagingBuffer(unsigned char* pixelData, VkDeviceSize imageSize)
{
	auto createInfo = VkBufferCreateInfo();
	createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	createInfo.pNext = NULL;
	createInfo.flags = 0;
	createInfo.size = imageSize;
	createInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.queueFamilyIndexCount = 0;
	createInfo.pQueueFamilyIndices = NULL;

	auto result = vkCreateBuffer(VulkanGraphicsResourceDevice::GetLogicalDevice(), &createInfo, NULL, &m_StagingBuffer);

	if (result)
	{
		printf_console("[VulkanGraphics] failed to create a staging buffer with error code %d\n", result);

		throw;
	}

	auto requirements = VkMemoryRequirements();
	vkGetBufferMemoryRequirements(VulkanGraphicsResourceDevice::GetLogicalDevice(), m_StagingBuffer, &requirements);

	auto allocationInfo = VkMemoryAllocateInfo();
	allocationInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocationInfo.pNext = NULL;

	if (!VulkanGraphicsResourceDevice::GetMemoryTypeIndex(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &allocationInfo.memoryTypeIndex))
	{
		printf_console("[VulkanGraphics] cannot find memoryTypeIndex matching VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT\n");

		throw;
	}

	allocationInfo.allocationSize = requirements.size;

	// TODO: find out which one is better between one big memory and seperate small allocation...
	result = vkAllocateMemory(VulkanGraphicsResourceDevice::GetLogicalDevice(), &allocationInfo, NULL, &m_StagingMemory);

	if (result)
	{
		printf_console("[VulkanGraphics] failed to create a staging buffer while allocating VkDeviceMemory with error code %d\n", result);

		throw;
	}

	result = vkBindBufferMemory(VulkanGraphicsResourceDevice::GetLogicalDevice(), m_StagingBuffer, m_StagingMemory, 0);

	if (result)
	{
		printf_console("[VulkanGraphics] failed to create a staging buffer while binding VkDeviceMemory to VkBuffer with error code %d\n", result);

		throw;
	}

	void* data;
	result = vkMapMemory(VulkanGraphicsResourceDevice::GetLogicalDevice(), m_StagingMemory, 0, imageSize, 0, &data);

	if (result)
	{
		printf_console("[VulkanGraphics] failed to create a staging buffer while mapping memory with error code %d\n", result);

		throw;
	}

	memcpy(data, pixelData, imageSize);
	vkUnmapMemory(VulkanGraphicsResourceDevice::GetLogicalDevice(), m_StagingMemory);
	m_IsStagingBufferExist = true;
}

bool VulkanGraphicsObjectTexture::CreateInternal()
{
	auto imageCreateInfo = VkImageCreateInfo();
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.pNext = NULL;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = m_Format;
	imageCreateInfo.extent.width = m_Width;
	imageCreateInfo.extent.height = m_Height;
	imageCreateInfo.extent.depth = 1;
	imageCreateInfo.mipLevels = m_MipLevel;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.samples = m_SampleCountBits;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCreateInfo.usage = m_Usage;
	imageCreateInfo.queueFamilyIndexCount = 0;
	imageCreateInfo.pQueueFamilyIndices = NULL;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.flags = 0;

	auto result = vkCreateImage(VulkanGraphicsResourceDevice::GetLogicalDevice(), &imageCreateInfo, NULL, &m_Image);

	if (result)
	{
		printf_console("[VulkanGraphics] failed to create a texture while creating VkImage with error code %d\n", result);

		throw;
	}

	auto requirements = VkMemoryRequirements();
	vkGetImageMemoryRequirements(VulkanGraphicsResourceDevice::GetLogicalDevice(), m_Image, &requirements);

	auto allocationInfo = VkMemoryAllocateInfo();
	allocationInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocationInfo.pNext = NULL;
	
	if (!VulkanGraphicsResourceDevice::GetMemoryTypeIndex(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &allocationInfo.memoryTypeIndex))
	{
		printf_console("[VulkanGraphics] cannot find memoryTypeIndex matching VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT\n");
		
		throw;
	}

	allocationInfo.allocationSize = requirements.size;

	// TODO: find out which one is better between one big memory and seperate small allocation...
	result = vkAllocateMemory(VulkanGraphicsResourceDevice::GetLogicalDevice(), &allocationInfo, NULL, &m_ImageMemory);

	if (result)
	{
		printf_console("[VulkanGraphics] failed to create a texture while allocating VkDeviceMemory with error code %d\n", result);
		
		throw;
	}

	result = vkBindImageMemory(VulkanGraphicsResourceDevice::GetLogicalDevice(), m_Image, m_ImageMemory, 0);

	if (result)
	{
		printf_console("[VulkanGraphics] failed to create a texture while binding VkDeviceMemory to VkImage with error code %d\n", result);

		throw;
	}

	auto imageViewCreateInfo = VkImageViewCreateInfo();
	imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCreateInfo.pNext = NULL;
	imageViewCreateInfo.image = m_Image;
	imageViewCreateInfo.format = m_Format;
	imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_R;
	imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_G;
	imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_B;
	imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_A;
	imageViewCreateInfo.subresourceRange.aspectMask = m_AspectMask;
	imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
	imageViewCreateInfo.subresourceRange.levelCount = m_MipLevel;
	imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imageViewCreateInfo.subresourceRange.layerCount = 1;
	imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewCreateInfo.flags = 0;
	result = vkCreateImageView(VulkanGraphicsResourceDevice::GetLogicalDevice(), &imageViewCreateInfo, NULL, &m_ImageView);

	if (result)
	{
		printf_console("[VulkanGraphics] failed to create a texture while creating VkImageView with error code %d\n", result);
		
		throw;
	}

	return true;
}

bool VulkanGraphicsObjectTexture::DestroyInternal()
{
	ClearStagingBuffer();
	vkDestroyImageView(VulkanGraphicsResourceDevice::GetLogicalDevice(), m_ImageView, NULL);
	vkFreeMemory(VulkanGraphicsResourceDevice::GetLogicalDevice(), m_ImageMemory, NULL);
	vkDestroyImage(VulkanGraphicsResourceDevice::GetLogicalDevice(), m_Image, NULL);

	return true;
}