#pragma once
#include "VulkanGraphicsResourceBase.h"
#include <vector>

struct VulkanDescriptorPoolInputData
{
	std::vector<VkDescriptorPoolSize> m_PoolSizeArray;
};

class VulkanGraphicsResourceDescriptorPoolManager : public VulkanGraphicsResourceManagerBase<VkDescriptorPool, VulkanDescriptorPoolInputData, size_t>
{
public:
	static VulkanGraphicsResourceDescriptorPoolManager& GetInstance();

protected:
	VkDescriptorPool CreateResourcePhysically(const VulkanDescriptorPoolInputData& inputData) override;
	void DestroyResourcePhysicially(const VkDescriptorPool& descriptorPool) override;
};
