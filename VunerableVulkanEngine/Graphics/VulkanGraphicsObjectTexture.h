#pragma once
#include "VulkanGraphicsObjectBase.h"

class VulkanGraphicsObjectTexture : public VulkanGraphicsObjectBase
{
public:
	const VkImageView& GetImageView()
	{
		return m_ImageView;
	}

	const VkImage& GetImage()
	{
		return m_Image;
	}

	const VkBuffer& GetStagingBuffer()
	{
		return m_StagingBuffer;
	}

	const uint32_t& GetWidth()
	{
		return m_Width;
	}

	const uint32_t& GetHeight()
	{
		return m_Height;
	}

	// TODO: need to support MipMap
	// TODO: need to support MSAA
	void CreateAsColorBuffer();
	void CreateAsColorBufferForGUI();
	void CreateAsDepthBuffer();
	void CreateAsTexture(const char* strPngPath);
	bool IsStagingBufferExist() { return m_IsStagingBufferExist; };
	void ResetStagingBufferExist() { m_IsStagingBufferExist = false; };
	void ApplyStagingBuffer(VkCommandBuffer& commandBuffer);
	void TryToClearStagingBuffer();

	// TODO: what else in various texture types do we need to support?

private:
	void Create() {}; // hide the base create function
	void CreateStagingBuffer(unsigned char* pixelData, VkDeviceSize imageSize);

protected:
	virtual bool CreateInternal() override;
	virtual bool DestroyInternal() override;

private:
	VkFormat m_Format;
	uint32_t m_Width;
	uint32_t m_Height;
	uint32_t m_MipLevel;
	VkSampleCountFlagBits m_SampleCountBits;
	uint32_t m_Usage;
	uint32_t m_AspectMask;

	VkImage m_Image;
	VkDeviceMemory m_ImageMemory;
	VkImageView m_ImageView;

	// We need a sampler. Let's make VulkanGraphicsObjectSampler!!!
	// Additionally, we need to declare the descriptor set layour in ResourceManager.
	// Lastly, we are going to the texture view and the sampler in the descriptor set....

	bool m_IsStagingBufferExist;
	VkDeviceMemory m_StagingMemory;
	VkBuffer m_StagingBuffer;
};

