#include "VulkanGraphicsResourceSwapchain.h"
#include "../DebugUtility.h"
#include "VulkanGraphicsResourceSurface.h"
#include "VulkanGraphicsResourceDevice.h"

VulkanGraphicsResourceSwapchain g_Instance;

VulkanGraphicsResourceSwapchain& VulkanGraphicsResourceSwapchain::GetInstance()
{
	return g_Instance;
}

void VulkanGraphicsResourceSwapchain::AcquireNextImage(const VkSemaphore& waitSemaphore, const VkFence& waitFence)
{
	auto result = vkAcquireNextImageKHR(VulkanGraphicsResourceDevice::GetInstance().GetLogicalDevice(), m_Swapchain, UINT32_MAX, waitSemaphore, waitFence, &m_AcquiredImageIndex);

	if (result)
	{
		printf_console("[VulkanGraphics] failed to acquire the next image with error code %d\n", result);
		throw;
	}
}

bool VulkanGraphicsResourceSwapchain::CreateInternal()
{
	if (!CreateSwapchain())
		return false;

	return CreateImageViews();
}

bool VulkanGraphicsResourceSwapchain::DestroyInternal()
{
	if (!DestroyImageViews())
		return false;

	return DestroySwapchain();
}

bool VulkanGraphicsResourceSwapchain::CreateSwapchain()
{
	DestroySwapchain();

	uint32_t imageCount = VulkanGraphicsResourceDevice::GetInstance().GetSurfaceCapabilities().minImageCount + 1;

	if (imageCount > VulkanGraphicsResourceDevice::GetInstance().GetSurfaceCapabilities().maxImageCount)
	{
		imageCount = VulkanGraphicsResourceDevice::GetInstance().GetSurfaceCapabilities().maxImageCount;
	}

	auto surfaceCapabilities = VulkanGraphicsResourceDevice::GetInstance().GetSurfaceCapabilities();
	uint32_t surfaceWidth, surfaceHeight;
	VulkanGraphicsResourceSurface::GetInstance().GetSurfaceSize(surfaceWidth, surfaceHeight);
	m_SwapchainFormat = VulkanGraphicsResourceDevice::GetInstance().GetSurfaceFormatArray()[0].format;
	m_SwapchainWith = max(surfaceCapabilities.minImageExtent.width, min(surfaceCapabilities.maxImageExtent.width, surfaceWidth));
	m_SwapchainHeight = max(surfaceCapabilities.minImageExtent.height, min(surfaceCapabilities.maxImageExtent.height, surfaceHeight));

	auto swapchainCreateInfo = VkSwapchainCreateInfoKHR();
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.pNext = NULL;
	swapchainCreateInfo.surface = VulkanGraphicsResourceSurface::GetInstance().GetSurface();
	swapchainCreateInfo.minImageCount = imageCount;
	swapchainCreateInfo.imageFormat = m_SwapchainFormat;
	swapchainCreateInfo.imageExtent.width = m_SwapchainWith;
	swapchainCreateInfo.imageExtent.height = m_SwapchainHeight;
	swapchainCreateInfo.preTransform = VulkanGraphicsResourceDevice::GetInstance().GetSurfaceCapabilities().currentTransform;
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
	swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;
	swapchainCreateInfo.clipped = true;
	swapchainCreateInfo.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	if (VulkanGraphicsResourceDevice::GetInstance().GetGraphicsQueueFamilyIndex() != VulkanGraphicsResourceDevice::GetInstance().GetPresentQueueFamilyIndex())
	{
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchainCreateInfo.queueFamilyIndexCount = 2;
		swapchainCreateInfo.pQueueFamilyIndices = new uint32_t[]{ (uint32_t)VulkanGraphicsResourceDevice::GetInstance().GetGraphicsQueueFamilyIndex(), (uint32_t)VulkanGraphicsResourceDevice::GetInstance().GetPresentQueueFamilyIndex() };
	}
	else
	{
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchainCreateInfo.queueFamilyIndexCount = 0;
		swapchainCreateInfo.pQueueFamilyIndices = NULL;
	}

	auto result = vkCreateSwapchainKHR(VulkanGraphicsResourceDevice::GetInstance().GetLogicalDevice(), &swapchainCreateInfo, NULL, &m_Swapchain);

	if (result)
	{
		printf_console("[VulkanGraphics] failed to create a swapchain with error code %d\n", result);
		return false;
	}

	return true;
}

bool VulkanGraphicsResourceSwapchain::DestroySwapchain()
{
	vkDestroySwapchainKHR(VulkanGraphicsResourceDevice::GetInstance().GetLogicalDevice(), m_Swapchain, NULL);
	return true;
}

bool VulkanGraphicsResourceSwapchain::CreateImageViews()
{
	DestroyImageViews();

	uint32_t count;
	vkGetSwapchainImagesKHR(VulkanGraphicsResourceDevice::GetInstance().GetLogicalDevice(), m_Swapchain, &count, NULL);
	m_ImageArray.resize(count);
	m_ImageViewArray.resize(count);
	vkGetSwapchainImagesKHR(VulkanGraphicsResourceDevice::GetInstance().GetLogicalDevice(), m_Swapchain, &count, m_ImageArray.data());

	for (int i = 0; i < count; ++i)
	{
		auto imageViewCreateInfo = VkImageViewCreateInfo();
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.pNext = NULL;
		imageViewCreateInfo.flags = 0;
		imageViewCreateInfo.image = m_ImageArray[i];
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreateInfo.format = VulkanGraphicsResourceDevice::GetInstance().GetSurfaceFormatArray()[0].format;
		imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_R;
		imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_G;
		imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_B;
		imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_A;
		imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.layerCount = 1;

		auto result = vkCreateImageView(VulkanGraphicsResourceDevice::GetInstance().GetLogicalDevice(), &imageViewCreateInfo, NULL, &m_ImageViewArray[i]);

		if (result)
		{
			printf_console("[VulkanGraphics] failed to create a image view in %d with error code %d\n", i, result);
			return false;
		}
	}

	m_IsImageViewArrayCreated = true;
	return true;
}

bool VulkanGraphicsResourceSwapchain::DestroyImageViews()
{
	if (!m_IsImageViewArrayCreated)
	{
		return true;
	}

	for (int i = 0; i < m_ImageViewArray.size(); ++i)
	{
		vkDestroyImageView(VulkanGraphicsResourceDevice::GetInstance().GetLogicalDevice(), m_ImageViewArray[0], NULL);
	}

	m_IsImageViewArrayCreated = false;
	return true;
}