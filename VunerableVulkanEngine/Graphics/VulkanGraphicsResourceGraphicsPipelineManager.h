#pragma once

#include "VulkanGraphicsResourceBase.h"
#include "VulkanGraphicsResourceShaderManager.h"

struct VulkanGraphicsPipelineInputData
{
	// shaders
	size_t m_ShaderIdentifiers[EVulkanShaderType_MAX];

	// properties for the fixed pipeline

	VulkanGraphicsPipelineInputData()
	{
		memset(m_ShaderIdentifiers, -1, EVulkanShaderType_MAX);
	}

	bool operator==(const VulkanGraphicsPipelineInputData& rhs)
	{
		if (memcmp(m_ShaderIdentifiers, &rhs.m_ShaderIdentifiers, sizeof(size_t) * EVulkanShaderType_MAX) != 0)
		{
			return false;
		}

		return true;
	}
};

struct VulkanGraphicsPipelineOutputData
{
	size_t m_DescriptorSetLayoutIdentifiers[EVulkanShaderType_MAX];
	size_t m_PipelineLayoutIdentifier;
	VkPipeline m_Pipeline;

	VulkanGraphicsPipelineOutputData()
	{
		memset(m_DescriptorSetLayoutIdentifiers, -1, EVulkanShaderType_MAX);
		m_PipelineLayoutIdentifier = -1;
	}
};

class VulkanGraphicsResourceGraphicsPipelineManager : public VulkanGraphicsResourceManagerBase<VulkanGraphicsPipelineOutputData, VulkanGraphicsPipelineInputData, VulkanGraphicsPipelineInputData>
{
public:
	static VulkanGraphicsResourceGraphicsPipelineManager& GetInstance();

protected:
	VulkanGraphicsPipelineOutputData CreateResourcePhysically(const VulkanGraphicsPipelineInputData& inputData) override;
	void DestroyResourcePhysicially(const VulkanGraphicsPipelineOutputData& outputData) override;

private:
	VkPipelineLayout GeneratePipelineLayout(const VulkanGraphicsPipelineInputData& inputData, const std::vector<VkDescriptorSetLayout>& descriptorSetLayoutArray, const std::vector<VkPushConstantRange>& pushConstantRangeArray);
	VkGraphicsPipelineCreateInfo GenerateCreateInfo(const VulkanGraphicsPipelineInputData& inputData, const VkPipelineLayout& pipelineLayout);
};