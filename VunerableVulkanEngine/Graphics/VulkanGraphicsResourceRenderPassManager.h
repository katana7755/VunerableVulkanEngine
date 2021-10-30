#pragma once
#include "VulkanGraphicsResourceBase.h"
#include <vector>

struct VulkanRenderPassInputData
{
	std::vector<VkAttachmentDescription>	m_AttachmentDescriptionArray;
	std::vector<VkSubpassDescription>		m_SubpassDescriptionArray;
	std::vector<VkSubpassDependency>		m_SubpassDependencyArray;
};

class VulkanGraphicsResourceRenderPassManager : public VulkanGraphicsResourceManagerBase<VkRenderPass, VulkanRenderPassInputData, size_t>
{
public:
	static VulkanGraphicsResourceRenderPassManager& GetInstance();

protected:
	VkRenderPass CreateResourcePhysically(const VulkanRenderPassInputData& inputData) override;
	void DestroyResourcePhysicially(const VkRenderPass& renderPass) override;
};
