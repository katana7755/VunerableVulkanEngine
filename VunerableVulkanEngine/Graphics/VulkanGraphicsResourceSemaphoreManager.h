#pragma
#include "VulkanGraphicsResourceBase.h"

class VulkanGraphicsResourceSemaphoreManager : public VulkanGraphicsResourceManagerBase<VkSemaphore, size_t, size_t>
{
public:
	static VulkanGraphicsResourceSemaphoreManager& GetInstance();

protected:
	VkSemaphore CreateResourcePhysically(const size_t& inputData) override;
	void DestroyResourcePhysicially(const VkSemaphore& outputData) override;
};