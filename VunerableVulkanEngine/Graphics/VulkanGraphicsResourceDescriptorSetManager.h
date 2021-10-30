#pragma once

#include "VulkanGraphicsResourceBase.h"
#include "VulkanGraphicsResourceShaderManager.h"

struct VulkanDescriptorSetInputData
{
	size_t					m_PipelineIdentifier;
	EVulkanShaderType::TYPE	m_ShaderType;
};

struct VulkanDescriptorSetOutputData
{
	VkDescriptorSet m_DescriptorSet;
	size_t			m_ShaderIdentifier;

	void UpdateCombinedSampler(uint32_t binding, const VkImage& image, const VkImageView& imageView, const VkSampler& sampler)
	{
	}

	void Flush()
	{
	}

private:
	std::vector<VkWriteDescriptorSet> m_WriteSetArray;
};

class VulkanGraphicsResourceDescriptorSetManager : public VulkanGraphicsResourceManagerBase<VulkanDescriptorSetOutputData, VulkanDescriptorSetInputData, size_t>
{
public:
	static VulkanGraphicsResourceDescriptorSetManager& GetInstance();

protected:
	VulkanDescriptorSetOutputData CreateResourcePhysically(const VulkanDescriptorSetInputData& inputData) override;
	void DestroyResourcePhysicially(const VulkanDescriptorSetOutputData& outputData) override;
};