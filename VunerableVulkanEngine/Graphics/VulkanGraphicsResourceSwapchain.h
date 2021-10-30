#pragma once
#include "VulkanGraphicsResourceBase.h"
#include <vector>

class VulkanGraphicsResourceSwapchain : public VulkanGraphicsResourceBase
{
public:
	static VulkanGraphicsResourceSwapchain& GetInstance();

public:
	const VkSwapchainKHR& GetSwapchain()
	{
		return m_Swapchain;
	}

	void AcquireNextImage(const VkSemaphore& waitSemaphore, const VkFence& waitFence);

	int GetImageCount()
	{
		return m_ImageArray.size();
	}
	
	const VkImage& GetImage(int index)
	{
		if (index < 0 || index >= m_ImageArray.size())
		{
			printf_console("[VulkanGraphics] failed to get image pointer (%d / %d)\n", index, m_ImageArray.size());
			throw;
		}

		return m_ImageArray[index];
	}

	int GetImageViewCount()
	{
		return m_ImageViewArray.size();
	}

	const VkImageView& GetImageView(int index)
	{
		if (index < 0 || index >= m_ImageViewArray.size())
		{
			printf_console("[VulkanGraphics] failed to get image view pointer (%d / %d)\n", index, m_ImageArray.size());
			throw;
		}

		return m_ImageViewArray[index];
	}

	const uint32_t& GetAcquiredImageIndex()
	{
		return m_AcquiredImageIndex;
	}

	const VkFormat& GetSwapchainFormat()
	{
		return m_SwapchainFormat;
	}

	void GetSwapchainSize(uint32_t& outWidth, uint32_t& outHeight)
	{
		outWidth = m_SwapchainWith;
		outHeight = m_SwapchainHeight;
	}

protected:
	virtual bool CreateInternal() override;
	virtual bool DestroyInternal() override;

private:
	bool CreateSwapchain();
	bool DestroySwapchain();
	bool CreateImageViews();
	bool DestroyImageViews();

private:
	VkSwapchainKHR m_Swapchain;
	bool m_IsImageViewArrayCreated;
	std::vector<VkImage> m_ImageArray;
	std::vector<VkImageView> m_ImageViewArray;
	uint32_t m_AcquiredImageIndex;
	VkFormat m_SwapchainFormat;
	uint32_t m_SwapchainWith;
	uint32_t m_SwapchainHeight;
};

