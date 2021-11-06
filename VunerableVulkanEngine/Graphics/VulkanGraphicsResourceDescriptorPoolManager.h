#pragma once
#include "VulkanGraphicsResourceBase.h"
#include <vector>

struct VulkanDescriptorPoolInputData
{
	std::vector<VkDescriptorPoolSize> m_PoolSizeArray;

	void CreateDefault()
	{
		const uint32_t EACH_POOL_COUNT = 10;

		m_PoolSizeArray.clear();
		{
			auto poolSize = VkDescriptorPoolSize();
			poolSize.type = VK_DESCRIPTOR_TYPE_SAMPLER;
			poolSize.descriptorCount = EACH_POOL_COUNT;
			m_PoolSizeArray.push_back(poolSize);
		}
		{
			auto poolSize = VkDescriptorPoolSize();
			poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			poolSize.descriptorCount = EACH_POOL_COUNT;
			m_PoolSizeArray.push_back(poolSize);
		}
		{
			auto poolSize = VkDescriptorPoolSize();
			poolSize.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			poolSize.descriptorCount = EACH_POOL_COUNT;
			m_PoolSizeArray.push_back(poolSize);
		}
		{
			auto poolSize = VkDescriptorPoolSize();
			poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			poolSize.descriptorCount = EACH_POOL_COUNT;
			m_PoolSizeArray.push_back(poolSize);
		}
		{
			auto poolSize = VkDescriptorPoolSize();
			poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
			poolSize.descriptorCount = EACH_POOL_COUNT;
			m_PoolSizeArray.push_back(poolSize);
		}
		{
			auto poolSize = VkDescriptorPoolSize();
			poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
			poolSize.descriptorCount = EACH_POOL_COUNT;
			m_PoolSizeArray.push_back(poolSize);
		}
		{
			auto poolSize = VkDescriptorPoolSize();
			poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			poolSize.descriptorCount = EACH_POOL_COUNT;
			m_PoolSizeArray.push_back(poolSize);
		}
		{
			auto poolSize = VkDescriptorPoolSize();
			poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			poolSize.descriptorCount = EACH_POOL_COUNT;
			m_PoolSizeArray.push_back(poolSize);
		}
		{
			auto poolSize = VkDescriptorPoolSize();
			poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
			poolSize.descriptorCount = EACH_POOL_COUNT;
			m_PoolSizeArray.push_back(poolSize);
		}
		{
			auto poolSize = VkDescriptorPoolSize();
			poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
			poolSize.descriptorCount = EACH_POOL_COUNT;
			m_PoolSizeArray.push_back(poolSize);
		}
		{
			auto poolSize = VkDescriptorPoolSize();
			poolSize.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
			poolSize.descriptorCount = EACH_POOL_COUNT;
			m_PoolSizeArray.push_back(poolSize);
		}
	}
};

class VulkanGraphicsResourceDescriptorPoolManager : public VulkanGraphicsResourceManagerBase<VkDescriptorPool, VulkanDescriptorPoolInputData, size_t>
{
public:
	static VulkanGraphicsResourceDescriptorPoolManager& GetInstance();

protected:
	VkDescriptorPool CreateResourcePhysically(const VulkanDescriptorPoolInputData& inputData) override;
	void DestroyResourcePhysicially(const VkDescriptorPool& descriptorPool) override;
};
