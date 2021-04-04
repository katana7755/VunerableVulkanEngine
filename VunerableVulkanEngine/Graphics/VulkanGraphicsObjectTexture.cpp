#include "VulkanGraphicsObjectTexture.h"
#include "../DebugUtility.h"
#include "VulkanGraphicsResourceDevice.h"

bool VulkanGraphicsObjectTexture::CreateInternal()
{
	// TODO: this is an example code which creates a depth texture. it will be changed to support all types of textures

	auto imageCreateInfo = VkImageCreateInfo();
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.pNext = NULL;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = VK_FORMAT_D32_SFLOAT;
	imageCreateInfo.extent.width = VulkanGraphicsResourceDevice::GetSurfaceCapabilities().currentExtent.width;
	imageCreateInfo.extent.height = VulkanGraphicsResourceDevice::GetSurfaceCapabilities().currentExtent.height;
	imageCreateInfo.extent.depth = 1;
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	imageCreateInfo.queueFamilyIndexCount = 0;
	imageCreateInfo.pQueueFamilyIndices = NULL;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.flags = 0;

	auto result = vkCreateImage(VulkanGraphicsResourceDevice::GetLogicalDevice(), &imageCreateInfo, NULL, &m_Image);

	if (result)
	{
		printf_console("[VulkanGraphics] failed to create a texture while creating VkImage with error code %d\n", result);
		return false;
	}

	auto requirements = VkMemoryRequirements();
	vkGetImageMemoryRequirements(VulkanGraphicsResourceDevice::GetLogicalDevice(), m_Image, &requirements);

	auto allocationInfo = VkMemoryAllocateInfo();
	allocationInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocationInfo.pNext = NULL;
	
	if (!VulkanGraphicsResourceDevice::GetMemoryTypeIndex(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &allocationInfo.memoryTypeIndex))
	{
		printf_console("[VulkanGraphics] cannot find memoryTypeIndex matching VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT\n");
		return false;
	}

	// TODO: find out which one is better between one big memory and seperate small allocation...
	allocationInfo.allocationSize = requirements.size;
	result = vkAllocateMemory(VulkanGraphicsResourceDevice::GetLogicalDevice(), &allocationInfo, NULL, &m_DeviceMemory);

	if (result)
	{
		printf_console("[VulkanGraphics] failed to create a texture while allocating VkDeviceMemory with error code %d\n", result);
		return false;
	}

	m_IsDeviceMemoryAllocated = true;
	result = vkBindImageMemory(VulkanGraphicsResourceDevice::GetLogicalDevice(), m_Image, m_DeviceMemory, 0);

	if (result)
	{
		printf_console("[VulkanGraphics] failed to create a texture while binding VkDeviceMemory to VkImage with error code %d\n", result);
		return false;
	}

	m_IsDeviceMemoryBound = true;

	auto imageViewCreateInfo = VkImageViewCreateInfo();
	imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCreateInfo.pNext = NULL;
	imageViewCreateInfo.image = m_Image;
	imageViewCreateInfo.format = VK_FORMAT_D32_SFLOAT;
	imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_R;
	imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_G;
	imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_B;
	imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_A;
	imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
	imageViewCreateInfo.subresourceRange.levelCount = 1;
	imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imageViewCreateInfo.subresourceRange.layerCount = 1;
	imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewCreateInfo.flags = 0;
	result = vkCreateImageView(VulkanGraphicsResourceDevice::GetLogicalDevice(), &imageViewCreateInfo, NULL, &m_ImageView);

	if (result)
	{
		printf_console("[VulkanGraphics] failed to create a texture while creating VkImageView with error code %d\n", result);
		return false;
	}

	m_IsImageViewCreated = true;
	return true;
}

bool VulkanGraphicsObjectTexture::DestroyInternal()
{
	if (m_IsImageViewCreated)
	{
		vkDestroyImageView(VulkanGraphicsResourceDevice::GetLogicalDevice(), m_ImageView, NULL);
		m_IsImageViewCreated = false;
	}

	if (m_IsDeviceMemoryBound)
	{
		m_IsDeviceMemoryBound = false;
	}

	if (m_IsDeviceMemoryAllocated)
	{
		vkFreeMemory(VulkanGraphicsResourceDevice::GetLogicalDevice(), m_DeviceMemory, NULL);
		m_IsDeviceMemoryAllocated = false;
	}

	vkDestroyImage(VulkanGraphicsResourceDevice::GetLogicalDevice(), m_Image, NULL);
	return true;
}