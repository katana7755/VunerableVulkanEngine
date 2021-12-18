#pragma once
#include "VulkanGraphicsResourceBase.h"
#include "VulkanGraphicsResourceSurface.h"
#include <vector>
#include <unordered_map>

class VulkanGraphicsResourceDevice : public VulkanGraphicsResourceBase
{
public:
	static VulkanGraphicsResourceDevice& GetInstance();

public:
	const VkPhysicalDevice& GetPhysicalDevice()
	{
		return m_PhysicalDeviceArray[m_PhysicalDeviceIndex];
	}

	const int& GetGraphicsQueueFamilyIndex()
	{
		return m_GraphicsQueueFamilyIndex;
	}

	const int& GetPresentQueueFamilyIndex()
	{
		return m_PresentQueueFamilyIndex;
	}

	const int& GetComputeQueueFamilyIndex()
	{
		return m_ComputeQueueFamilyIndex;
	}

	const VkQueue& GetGraphicsQueue()
	{
		return m_QueueMap[m_GraphicsQueueFamilyIndex];
	}

	const VkQueue& GetPresentQueue()
	{
		return m_QueueMap[m_PresentQueueFamilyIndex];
	}

	const VkQueue& GetComputeQueue()
	{
		return m_QueueMap[m_ComputeQueueFamilyIndex];
	}

	const VkDevice& GetLogicalDevice()
	{
		return m_LogicalDevice;
	}

	const VkPhysicalDeviceProperties& GetPhysicalDeviceProperties()
	{
		return m_PhysicalDeviceProperties;
	}

	const VkSurfaceCapabilitiesKHR& GetSurfaceCapabilities(bool updateCache = false)
	{
		if (updateCache)
		{
			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDeviceArray[m_PhysicalDeviceIndex], VulkanGraphicsResourceSurface::GetInstance().GetSurface(), &m_SurfaceCapabilities);
		}
		
		return m_SurfaceCapabilities;
	}

	const std::vector<VkSurfaceFormatKHR>& GetSurfaceFormatArray()
	{
		return m_SurfaceFormatArray;
	}

	bool GetMemoryTypeIndex(uint32_t typeBits, VkFlags memoryFlags, uint32_t* outIndex)
	{
		for (int i = 0; i < m_MemoryProperties.memoryTypeCount; ++i)
		{
			if (typeBits & 1)
			{
				if (m_MemoryProperties.memoryTypes[i].propertyFlags & memoryFlags)
				{
					*outIndex = i;
					return true;
				}
			}

			typeBits >>= 1;
		}

		return false;
	}

protected:
	virtual bool CreateInternal() override;
	virtual bool DestroyInternal() override;

private:
	bool EnumeratePhysicalDevices();
	bool CreateLogicalDevice();
	bool DestroyLogicalDevice();
	void GetDeviceQueueOnlyIfNotExist(int queueFamilyIndex);

private:
	std::vector<VkPhysicalDevice> m_PhysicalDeviceArray; // TODO: Think about removing this because it seems redundant...
	int m_PhysicalDeviceIndex;
	int m_GraphicsQueueFamilyIndex;
	int m_PresentQueueFamilyIndex;
	int m_ComputeQueueFamilyIndex;
	std::unordered_map<int, VkQueue> m_QueueMap;
	VkDevice m_LogicalDevice;
	VkPhysicalDeviceProperties m_PhysicalDeviceProperties;
	VkSurfaceCapabilitiesKHR m_SurfaceCapabilities;
	std::vector<VkSurfaceFormatKHR> m_SurfaceFormatArray;
	VkPhysicalDeviceMemoryProperties m_MemoryProperties;
};

