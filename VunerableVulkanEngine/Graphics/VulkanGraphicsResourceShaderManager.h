#pragma once
#include "VulkanGraphicsResourceBase.h"

class VulkanGraphicsResourceShaderManager : public VulkanGraphicsResourceManagerBase<VkShaderModule>
{
public:
	static VulkanGraphicsResourceShaderManager& GetInstance();

public:
	void CreateResourcePhysically(const size_t identifier, const char* data, const unsigned int size) override;
	void DestroyResourcePhysicially(const size_t identifier) override;
};