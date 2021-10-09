#pragma once
#include "VulkanGraphicsResourceBase.h"
#include <vector>
#include <unordered_map>

class VulkanGraphicsResourceDevice : public VulkanGraphicsResourceBase
{
public:
	static const int& GetGraphicsQueueFamilyIndex()
	{
		return s_GraphicsQueueFamilyIndex;
	}

	static const int& GetPresentQueueFamilyIndex()
	{
		return s_PresentQueueFamilyIndex;
	}

	static const int& GetTransferQueueFamilyIndex()
	{
		return s_TransferQueueFamilyIndex;
	}

	static const int& GetComputeQueueFamilyIndex()
	{
		return s_ComputeQueueFamilyIndex;
	}

	static const VkPhysicalDevice& GetPhysicalDevice()
	{
		return s_PhysicalDeviceArray[s_PhysicalDeviceIndex];
	}

	static const VkDevice& GetLogicalDevice()
	{
		return s_LogicalDevice;
	}

	static const VkPhysicalDeviceProperties& GetPhysicalDeviceProperties()
	{
		return s_PhysicalDeviceProperties;
	}

	static const VkSurfaceCapabilitiesKHR& GetSurfaceCapabilities()
	{
		return s_SurfaceCapabilities;
	}

	static const std::vector<VkSurfaceFormatKHR>& GetSurfaceFormatArray()
	{
		return s_SurfaceFormatArray;
	}

	static bool GetMemoryTypeIndex(uint32_t typeBits, VkFlags memoryFlags, uint32_t* outIndex)
	{
		for (int i = 0; i < s_MemoryProperties.memoryTypeCount; ++i)
		{
			if (typeBits & 1)
			{
				if (s_MemoryProperties.memoryTypes[i].propertyFlags & memoryFlags)
				{
					*outIndex = i;
					return true;
				}
			}

			typeBits >>= 1;
		}

		return false;
	}

	static const VkQueue& GetGraphicsQueue()
	{
		return s_QueueMap[s_GraphicsQueueFamilyIndex];
	}

	static const VkQueue& GetPresentQueue()
	{
		return s_QueueMap[s_PresentQueueFamilyIndex];
	}

	static const VkQueue& GetTransferQueue()
	{
		return s_QueueMap[s_TransferQueueFamilyIndex];
	}

	static const VkQueue& GetComputeQueue()
	{
		return s_QueueMap[s_ComputeQueueFamilyIndex];
	}

private:
	static std::vector<VkPhysicalDevice> s_PhysicalDeviceArray; // TODO: Think about removing this because it seems redundant...
	static int s_PhysicalDeviceIndex;
	static int s_GraphicsQueueFamilyIndex;
	static int s_PresentQueueFamilyIndex;
	static int s_TransferQueueFamilyIndex;
	static int s_ComputeQueueFamilyIndex;
	static VkDevice s_LogicalDevice;
	static VkPhysicalDeviceProperties s_PhysicalDeviceProperties;
	static VkSurfaceCapabilitiesKHR s_SurfaceCapabilities;
	static std::vector<VkSurfaceFormatKHR> s_SurfaceFormatArray;
	static VkPhysicalDeviceMemoryProperties s_MemoryProperties;
	static std::unordered_map<int, VkQueue> s_QueueMap;

protected:
	virtual bool CreateInternal() override;
	virtual bool DestroyInternal() override;

private:
	bool EnumeratePhysicalDevices();
	bool CreateLogicalDevice();
	bool DestroyLogicalDevice();
	void GetDeviceQueueOnlyIfNotExist(int queueFamilyIndex);
};

