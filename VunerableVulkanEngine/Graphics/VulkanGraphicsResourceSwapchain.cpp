#include "VulkanGraphicsResourceSwapchain.h"
#include "../DebugUtility.h"
#include "VulkanGraphicsResourceSurface.h"
#include "VulkanGraphicsResourceDevice.h"

VkSwapchainKHR VulkanGraphicsResourceSwapchain::s_Swapchain;
bool VulkanGraphicsResourceSwapchain::s_IsImageViewArrayCreated = false;
std::vector<VkImage> VulkanGraphicsResourceSwapchain::s_ImageArray;
std::vector<VkImageView> VulkanGraphicsResourceSwapchain::s_ImageViewArray;
uint32_t VulkanGraphicsResourceSwapchain::s_AcquiredImageIndex;
VkFormat VulkanGraphicsResourceSwapchain::s_SwapchainFormat;
uint32_t VulkanGraphicsResourceSwapchain::s_SwapchainWith;
uint32_t VulkanGraphicsResourceSwapchain::s_SwapchainHeight;

const VkSwapchainKHR& VulkanGraphicsResourceSwapchain::GetSwapchain()
{
	return s_Swapchain;
}

int VulkanGraphicsResourceSwapchain::GetImageCount()
{
	return s_ImageArray.size();
}

const VkImage& VulkanGraphicsResourceSwapchain::GetImage(int index)
{
	if (index < 0 || index >= s_ImageArray.size())
	{
		printf_console("[VulkanGraphics] failed to get image pointer (%d / %d)\n", index, s_ImageArray.size());
		throw;
	}

	return s_ImageArray[index];
}

int VulkanGraphicsResourceSwapchain::GetImageViewCount()
{
	return s_ImageViewArray.size();
}

const VkImageView& VulkanGraphicsResourceSwapchain::GetImageView(int index)
{
	if (index < 0 || index >= s_ImageViewArray.size())
	{
		printf_console("[VulkanGraphics] failed to get image view pointer (%d / %d)\n", index, s_ImageArray.size());
		throw;
	}

	return s_ImageViewArray[index];
}

void VulkanGraphicsResourceSwapchain::AcquireNextImage(const VkSemaphore& waitSemaphore, const VkFence& waitFence)
{
	auto result = vkAcquireNextImageKHR(VulkanGraphicsResourceDevice::GetLogicalDevice(), s_Swapchain, UINT32_MAX, waitSemaphore, waitFence, &s_AcquiredImageIndex);

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

	uint32_t imageCount = VulkanGraphicsResourceDevice::GetSurfaceCapabilities().minImageCount + 1;

	if (imageCount > VulkanGraphicsResourceDevice::GetSurfaceCapabilities().maxImageCount)
	{
		imageCount = VulkanGraphicsResourceDevice::GetSurfaceCapabilities().maxImageCount;
	}

	auto surfaceCapabilities = VulkanGraphicsResourceDevice::GetSurfaceCapabilities();
	uint32_t surfaceWidth, surfaceHeight;
	VulkanGraphicsResourceSurface::GetSurfaceSize(surfaceWidth, surfaceHeight);
	s_SwapchainFormat = VulkanGraphicsResourceDevice::GetSurfaceFormatArray()[0].format;
	s_SwapchainWith = max(surfaceCapabilities.minImageExtent.width, min(surfaceCapabilities.maxImageExtent.width, surfaceWidth));
	s_SwapchainHeight = max(surfaceCapabilities.minImageExtent.height, min(surfaceCapabilities.maxImageExtent.height, surfaceHeight));

	auto swapchainCreateInfo = VkSwapchainCreateInfoKHR();
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.pNext = NULL;
	swapchainCreateInfo.surface = VulkanGraphicsResourceSurface::GetSurface();
	swapchainCreateInfo.minImageCount = imageCount;
	swapchainCreateInfo.imageFormat = s_SwapchainFormat;
	swapchainCreateInfo.imageExtent.width = s_SwapchainWith;
	swapchainCreateInfo.imageExtent.height = s_SwapchainHeight;
	swapchainCreateInfo.preTransform = VulkanGraphicsResourceDevice::GetSurfaceCapabilities().currentTransform;
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
	swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;
	swapchainCreateInfo.clipped = true;
	swapchainCreateInfo.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	if (VulkanGraphicsResourceDevice::GetGraphicsQueueFamilyIndex() != VulkanGraphicsResourceDevice::GetPresentQueueFamilyIndex())
	{
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchainCreateInfo.queueFamilyIndexCount = 2;
		swapchainCreateInfo.pQueueFamilyIndices = new uint32_t[]{ (uint32_t)VulkanGraphicsResourceDevice::GetGraphicsQueueFamilyIndex(), (uint32_t)VulkanGraphicsResourceDevice::GetPresentQueueFamilyIndex() };
	}
	else
	{
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchainCreateInfo.queueFamilyIndexCount = 0;
		swapchainCreateInfo.pQueueFamilyIndices = NULL;
	}

	auto result = vkCreateSwapchainKHR(VulkanGraphicsResourceDevice::GetLogicalDevice(), &swapchainCreateInfo, NULL, &s_Swapchain);

	if (result)
	{
		printf_console("[VulkanGraphics] failed to create a swapchain with error code %d\n", result);
		return false;
	}

	return true;
}

bool VulkanGraphicsResourceSwapchain::DestroySwapchain()
{
	vkDestroySwapchainKHR(VulkanGraphicsResourceDevice::GetLogicalDevice(), s_Swapchain, NULL);
	return true;
}

bool VulkanGraphicsResourceSwapchain::CreateImageViews()
{
	DestroyImageViews();

	uint32_t count;
	vkGetSwapchainImagesKHR(VulkanGraphicsResourceDevice::GetLogicalDevice(), s_Swapchain, &count, NULL);
	s_ImageArray.resize(count);
	s_ImageViewArray.resize(count);
	vkGetSwapchainImagesKHR(VulkanGraphicsResourceDevice::GetLogicalDevice(), s_Swapchain, &count, s_ImageArray.data());

	for (int i = 0; i < count; ++i)
	{
		auto imageViewCreateInfo = VkImageViewCreateInfo();
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.pNext = NULL;
		imageViewCreateInfo.flags = 0;
		imageViewCreateInfo.image = s_ImageArray[i];
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreateInfo.format = VulkanGraphicsResourceDevice::GetSurfaceFormatArray()[0].format;
		imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_R;
		imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_G;
		imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_B;
		imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_A;
		imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.layerCount = 1;

		auto result = vkCreateImageView(VulkanGraphicsResourceDevice::GetLogicalDevice(), &imageViewCreateInfo, NULL, &s_ImageViewArray[i]);

		if (result)
		{
			printf_console("[VulkanGraphics] failed to create a image view in %d with error code %d\n", i, result);
			return false;
		}
	}

	s_IsImageViewArrayCreated = true;
	return true;
}

bool VulkanGraphicsResourceSwapchain::DestroyImageViews()
{
	if (!s_IsImageViewArrayCreated)
	{
		return true;
	}

	for (int i = 0; i < s_ImageViewArray.size(); ++i)
	{
		vkDestroyImageView(VulkanGraphicsResourceDevice::GetLogicalDevice(), s_ImageViewArray[0], NULL);
	}

	s_IsImageViewArrayCreated = false;
	return true;
}