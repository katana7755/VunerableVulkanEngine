#pragma once

#include "VulkanGraphicsResourceBase.h"
#include "VulkanGraphicsResourceShaderManager.h"

struct VulkanGraphicsPipelineInputData
{
	// TODO: render pass (this will be changed after a render pass manager comes out
	size_t		m_RenderPassIdentifier;
	uint32_t	m_SubPassIndex;

	// shaders
	size_t		m_ShaderIdentifiers[EVulkanShaderType::MAX];

	// properties for the fixed pipeline

	VulkanGraphicsPipelineInputData()
	{
		memset(m_ShaderIdentifiers, -1, EVulkanShaderType::MAX);
	}

	bool operator==(const VulkanGraphicsPipelineInputData& rhs)
	{
		if (memcmp(m_ShaderIdentifiers, &rhs.m_ShaderIdentifiers, sizeof(size_t) * EVulkanShaderType::MAX) != 0)
		{
			return false;
		}

		return true;
	}
};

struct VulkanGraphicsPipelineOutputData
{
	size_t		m_DescriptorSetLayoutIdentifiers[EVulkanShaderType::MAX];
	size_t		m_PipelineLayoutIdentifier;
	size_t		m_ShaderIdentifiers[EVulkanShaderType::MAX];
	VkPipeline	m_Pipeline;

	VulkanGraphicsPipelineOutputData()
	{
		memset(m_DescriptorSetLayoutIdentifiers, -1, EVulkanShaderType::MAX);
		m_PipelineLayoutIdentifier = -1;
	}

	VkPipelineBindPoint GetBindPoint()
	{
		return VK_PIPELINE_BIND_POINT_GRAPHICS;
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