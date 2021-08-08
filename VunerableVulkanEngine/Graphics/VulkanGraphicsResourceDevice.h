#pragma once
#include "VulkanGraphicsResourceBase.h"
#include <vector>

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
		return s_GraphicsQueue;
	}

	static const VkQueue& GetPresentQueue()
	{
		return s_PresentQueue;
	}

	static const VkQueue& GetTransferQueue()
	{
		return s_TransferQueue;
	}

private:
	static std::vector<VkPhysicalDevice> s_PhysicalDeviceArray; // TODO: Think about removing this because it seems redundant...
	static int s_PhysicalDeviceIndex;
	static int s_GraphicsQueueFamilyIndex;
	static int s_PresentQueueFamilyIndex;
	static int s_TransferQueueFamilyIndex;
	static VkDevice s_LogicalDevice;
	static VkPhysicalDeviceProperties s_PhysicalDeviceProperties;
	static VkSurfaceCapabilitiesKHR s_SurfaceCapabilities;
	static std::vector<VkSurfaceFormatKHR> s_SurfaceFormatArray;
	static VkPhysicalDeviceMemoryProperties s_MemoryProperties;
	static VkQueue s_GraphicsQueue;
	static VkQueue s_PresentQueue;
	static VkQueue s_TransferQueue;

protected:
	virtual bool CreateInternal() override;
	virtual bool DestroyInternal() override;

private:
	bool EnumeratePhysicalDevices();
	bool CreateLogicalDevice();
	bool DestroyLogicalDevice();
};

