#pragma once

#include "VulkanGraphicsResourceBase.h"
#include "VulkanGraphicsResourceShaderManager.h"

class VulkanGraphicsResourceDescriptorSetLayoutManager : public VulkanGraphicsResourceManagerBase<VkDescriptorSetLayout, VulkanShaderMetaData, VulkanShaderMetaData>
{
public:
	static VulkanGraphicsResourceDescriptorSetLayoutManager& GetInstance();

protected:
	VkDescriptorSetLayout CreateResourcePhysically(const VulkanShaderMetaData& shaderMetaData) override;
	void DestroyResourcePhysicially(const VkDescriptorSetLayout& descriptorSetLayout) override;
};