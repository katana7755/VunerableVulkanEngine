#pragma once

#include "VulkanGraphicsResourceBase.h"
#include <vector>

struct VulkanPipelineLayoutInputData
{
	std::vector<VkDescriptorSetLayout> m_DescriptorSetLayoutArray;
	std::vector<VkPushConstantRange> m_PushConstantRangeArray;

	bool operator==(const VulkanPipelineLayoutInputData& rhs)
	{
		if (m_DescriptorSetLayoutArray.size() != rhs.m_DescriptorSetLayoutArray.size())
		{
			return false;
		}

		if (memcmp(m_DescriptorSetLayoutArray.data(), rhs.m_DescriptorSetLayoutArray.data(), sizeof(VkDescriptorSetLayout) * m_DescriptorSetLayoutArray.size()))
		{
			return false;
		}

		if (m_PushConstantRangeArray.size() != rhs.m_PushConstantRangeArray.size())
		{
			return false;
		}

		if (memcmp(m_PushConstantRangeArray.data(), rhs.m_PushConstantRangeArray.data(), sizeof(VkPushConstantRange) * m_PushConstantRangeArray.size()))
		{
			return false;
		}

		return true;
	}
};

class VulkanGraphicsResourcePipelineLayoutManager : public VulkanGraphicsResourceManagerBase<VkPipelineLayout, VulkanPipelineLayoutInputData, VulkanPipelineLayoutInputData>
{
public:
	static VulkanGraphicsResourcePipelineLayoutManager& GetInstance();

protected:
	VkPipelineLayout CreateResourcePhysically(const VulkanPipelineLayoutInputData& inputData) override;
	void DestroyResourcePhysicially(const VkPipelineLayout& pipelineLayout) override;
};