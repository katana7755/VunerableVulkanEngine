#pragma once

#include "VulkanGraphicsResourceBase.h"
#include "VulnerableUploadBufferManager.h"

class VulkanGraphicsResourceShaderManager : public VulkanGraphicsResourceManagerBase<VkShaderModule, size_t, std::string>
{
public:
	static VulkanGraphicsResourceShaderManager& GetInstance();

protected:
	VkShaderModule CreateResourcePhysically(const size_t& uploadBufferIdentifier) override;
	void DestroyResourcePhysicially(const VkShaderModule& shaderModule) override;
};