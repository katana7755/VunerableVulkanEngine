#pragma once
#include "VulkanGraphicsResourceBase.h"
#include <vector>

class VulkanGraphicsResourceSwapchain : public VulkanGraphicsResourceBase
{
public:
	static const VkSwapchainKHR& GetSwapchain();
	static int GetImageCount();
	static const VkImage& GetImage(int index);
	static int GetImageViewCount();
	static const VkImageView& GetImageView(int index);
	static void AcquireNextImage(const VkSemaphore& waitSemaphore, const VkFence& waitFence);

	static const uint32_t& GetAcquiredImageIndex()
	{
		return s_AcquiredImageIndex;
	}

	static const VkFormat& GetSwapchainFormat()
	{
		return s_SwapchainFormat;
	}

	static void GetSwapchainSize(uint32_t& outWidth, uint32_t& outHeight)
	{
		outWidth = s_SwapchainWith;
		outHeight = s_SwapchainHeight;
	}

private:
	static VkSwapchainKHR s_Swapchain;
	static bool s_IsImageViewArrayCreated;
	static std::vector<VkImage> s_ImageArray;
	static std::vector<VkImageView> s_ImageViewArray;
	static uint32_t s_AcquiredImageIndex;
	static VkFormat s_SwapchainFormat;
	static uint32_t s_SwapchainWith;
	static uint32_t s_SwapchainHeight;

protected:
	virtual bool CreateInternal() override;
	virtual bool DestroyInternal() override;

private:
	bool CreateSwapchain();
	bool DestroySwapchain();
	bool CreateImageViews();
	bool DestroyImageViews();
};

