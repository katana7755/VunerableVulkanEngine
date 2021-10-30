#pragma once
#include "VulkanGraphicsResourceBase.h"
#include <vector>

struct VulkanFrameBufferInputData
{
	size_t						m_RenderPassIdentifier;
	std::vector<VkImageView>	m_AttachmentArray;
	uint32_t					m_Width;
	uint32_t					m_Height;
	uint32_t					m_Layers;
};

class VulkanGraphicsResourceFrameBufferManager : public VulkanGraphicsResourceManagerBase<VkFramebuffer, VulkanFrameBufferInputData, size_t>
{
public:
	static VulkanGraphicsResourceFrameBufferManager& GetInstance();

protected:
	VkFramebuffer CreateResourcePhysically(const VulkanFrameBufferInputData& inputData) override;
	void DestroyResourcePhysicially(const VkFramebuffer& renderPass) override;
};