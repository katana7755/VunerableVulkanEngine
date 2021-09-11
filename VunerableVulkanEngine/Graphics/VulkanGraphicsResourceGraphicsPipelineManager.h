#pragma once

#include "VulkanGraphicsResourceBase.h"

struct VulkanGraphicsPipelineInputData
{
	// shaders
	size_t m_VertexShaderIdentifier;
	size_t m_PixelShaderIdentifier;

	// properties for the fixed pipeline
};

class VulkanGraphicsResourceGraphicsPipelineManager : public VulkanGraphicsResourceManagerBase<VkPipeline, VulkanGraphicsPipelineInputData, VulkanGraphicsPipelineInputData>
{
public:
	static VulkanGraphicsResourceGraphicsPipelineManager& GetInstance();

protected:
	VkPipeline CreateResourcePhysically(const VulkanGraphicsPipelineInputData& inputData) override;
	void DestroyResourcePhysicially(const VkPipeline& pipeline) override;

private:
	VkGraphicsPipelineCreateInfo GenerateCreateInfo(const VulkanGraphicsPipelineInputData& inputData);
};