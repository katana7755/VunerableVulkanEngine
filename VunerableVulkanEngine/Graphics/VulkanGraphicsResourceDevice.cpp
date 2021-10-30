#include "VulkanGraphicsResourceDevice.h"
#include "../DebugUtility.h"
#include "VulkanGraphicsResourceInstance.h"
#include "VulkanGraphicsResourceSurface.h"
#include <set>

VulkanGraphicsResourceDevice g_Instance;

VulkanGraphicsResourceDevice& VulkanGraphicsResourceDevice::GetInstance()
{
	return g_Instance;
}

bool VulkanGraphicsResourceDevice::CreateInternal()
{
	if (!EnumeratePhysicalDevices())
		return false;

	return CreateLogicalDevice();
}

bool VulkanGraphicsResourceDevice::DestroyInternal()
{
	return DestroyLogicalDevice();
}

bool VulkanGraphicsResourceDevice::EnumeratePhysicalDevices()
{
	uint32_t count = 0;
	auto result = vkEnumeratePhysicalDevices(VulkanGraphicsResourceInstance::GetInstance().GetVkInstance(), &count, NULL);

	if (result)
	{
		printf_console("[VulkanGraphics] failed to enumerate physical devices with %d\n", result);
		return false;
	}

	m_PhysicalDeviceArray.resize(count);
	result = vkEnumeratePhysicalDevices(VulkanGraphicsResourceInstance::GetInstance().GetVkInstance(), &count, m_PhysicalDeviceArray.data());

	if (result)
	{
		printf_console("[VulkanGraphics] failed to enumerate physical devices with error code %d\n", result);
		return false;
	}

	printf_console("[VulkanGraphics] physical device count is %d\n", m_PhysicalDeviceArray.size());
	return true;
}

class PotentialLocalDeviceOption
{
public:
	int _GraphicsQueueFamilyIndex = -1;
	int _PresentQueueFamilyIndex = -1;
	int _TransferQueueFamilyIndex = -1;
	int _ComputeQueueFamilyIndex = -1;

	void TryToSetGraphicsQueueFamilyIndex(int& outputIndex)
	{
		TryToSetQueueFamilyIndex(_GraphicsQueueFamilyIndex, (_PresentQueueFamilyIndex < 0) && (_TransferQueueFamilyIndex < 0) && (_ComputeQueueFamilyIndex < 0), outputIndex);
	}

	void TryToSetPresentQueueFamilyIndex(int& outputIndex)
	{
		TryToSetQueueFamilyIndex(_PresentQueueFamilyIndex, (_GraphicsQueueFamilyIndex < 0) && (_TransferQueueFamilyIndex < 0) && (_ComputeQueueFamilyIndex < 0), outputIndex);
	}

	void TryToSetComputeQueueFamilyIndex(int& outputIndex)
	{
		TryToSetQueueFamilyIndex(_ComputeQueueFamilyIndex, (_GraphicsQueueFamilyIndex < 0) && (_PresentQueueFamilyIndex < 0) && (_TransferQueueFamilyIndex < 0), outputIndex);
	}

private:
	void TryToSetQueueFamilyIndex(const int& mainIndex, const bool& areAllOtherIndicesNull, int& outputIndex)
	{
		if (mainIndex < 0)
		{
			return;
		}

		if (outputIndex > 0 && !areAllOtherIndicesNull)
		{
			return;
		}

		outputIndex = mainIndex;
	}
};

bool VulkanGraphicsResourceDevice::CreateLogicalDevice()
{
	DestroyLogicalDevice();

	std::vector<PotentialLocalDeviceOption> potentialOptions;
	uint32_t count = 0;
	std::vector<VkQueueFamilyProperties> queueFamilyPropertiesArray;

	for (int i = 0; i < m_PhysicalDeviceArray.size(); ++i)
	{
		printf_console("\t%d device ++++++++++\n", i);
		m_PhysicalDeviceIndex = -1;
		m_GraphicsQueueFamilyIndex = -1;
		m_PresentQueueFamilyIndex = -1;
		m_ComputeQueueFamilyIndex = -1;
		potentialOptions.clear();
		vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDeviceArray[i], &count, NULL);
		queueFamilyPropertiesArray.resize(count);
		vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDeviceArray[i], &count, queueFamilyPropertiesArray.data());
		printf_console("\t+ queue famility count: %d\n", count);

		for (int j = 0; j < count; ++j)
		{
			printf_console("\t+++++ %d queue family\n", j);
			printf_console("\t++++++++++ flags: %x\n", queueFamilyPropertiesArray[j].queueFlags);
			printf_console("\t++++++++++ count: %d\n", queueFamilyPropertiesArray[j].queueCount);
			printf_console("\t++++++++++ timeBits: %d\n", queueFamilyPropertiesArray[j].timestampValidBits);
			printf_console("\t++++++++++ minImageWidth: %d\n", queueFamilyPropertiesArray[j].minImageTransferGranularity.width);
			printf_console("\t++++++++++ minImageHeight: %d\n", queueFamilyPropertiesArray[j].minImageTransferGranularity.height);
			printf_console("\t++++++++++ minImageDepth: %d\n", queueFamilyPropertiesArray[j].minImageTransferGranularity.depth);

			auto option = PotentialLocalDeviceOption();
			bool isPassed = false;			

			if (queueFamilyPropertiesArray[j].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				option._GraphicsQueueFamilyIndex = j;
				isPassed = true;
			}

			if (queueFamilyPropertiesArray[j].queueFlags & VK_QUEUE_COMPUTE_BIT)
			{
				option._ComputeQueueFamilyIndex = j;
				isPassed = true;
			}

			if (queueFamilyPropertiesArray[j].queueFlags & VK_QUEUE_TRANSFER_BIT)
			{
				option._TransferQueueFamilyIndex = j;
				isPassed = true;
			}

			{
				VkBool32 doesSupportPresent = VK_FALSE;
				vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDeviceArray[i], j, VulkanGraphicsResourceSurface::GetInstance().GetSurface(), &doesSupportPresent);

				if (doesSupportPresent == VK_TRUE)
				{
					option._PresentQueueFamilyIndex = j;
				}

				isPassed = true;
			}

			if (isPassed)
			{
				potentialOptions.push_back(option);
			}
		}

		if (potentialOptions.size() <= 0)
		{
			continue;
		}

		m_PhysicalDeviceIndex = i;

		for (int j = 0; j < potentialOptions.size(); ++j)
		{
			potentialOptions[j].TryToSetGraphicsQueueFamilyIndex(m_GraphicsQueueFamilyIndex);
			potentialOptions[j].TryToSetPresentQueueFamilyIndex(m_PresentQueueFamilyIndex);
			potentialOptions[j].TryToSetComputeQueueFamilyIndex(m_ComputeQueueFamilyIndex);
		}

		if (m_GraphicsQueueFamilyIndex == -1 || m_PresentQueueFamilyIndex == -1 || m_ComputeQueueFamilyIndex == -1)
		{
			continue;
		}

		// TODO: Let's set multiple queue creation and compare the performance with when it has single queue creating...
		auto queueCreateInfoArray = std::vector<VkDeviceQueueCreateInfo>();
		auto queueCreateInfo = VkDeviceQueueCreateInfo();
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.pNext = NULL;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = new float[] { 0.0 };
		queueCreateInfo.queueFamilyIndex = m_GraphicsQueueFamilyIndex;
		queueCreateInfoArray.push_back(queueCreateInfo);

		auto queueSet = std::set<int>();
		queueSet.insert(m_GraphicsQueueFamilyIndex);

		if (queueSet.find(m_PresentQueueFamilyIndex) == queueSet.end())
		{
			queueCreateInfo.queueFamilyIndex = m_PresentQueueFamilyIndex;
			queueCreateInfoArray.push_back(queueCreateInfo);
			queueSet.insert(m_PresentQueueFamilyIndex);
		}

		if (queueSet.find(m_ComputeQueueFamilyIndex) == queueSet.end())
		{
			queueCreateInfo.queueFamilyIndex = m_ComputeQueueFamilyIndex;
			queueCreateInfoArray.push_back(queueCreateInfo);
			queueSet.insert(m_ComputeQueueFamilyIndex);
		}

		auto extensionNameArray = std::vector<const char*>();
		extensionNameArray.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

		auto deviceCreateInfo = VkDeviceCreateInfo();
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.pNext = NULL;
		deviceCreateInfo.queueCreateInfoCount = queueCreateInfoArray.size();
		deviceCreateInfo.pQueueCreateInfos = queueCreateInfoArray.data();
		deviceCreateInfo.enabledExtensionCount = extensionNameArray.size();
		deviceCreateInfo.ppEnabledExtensionNames = extensionNameArray.data();
		deviceCreateInfo.enabledLayerCount = 0;
		deviceCreateInfo.ppEnabledLayerNames = NULL;
		deviceCreateInfo.pEnabledFeatures = NULL;

		auto result = vkCreateDevice(m_PhysicalDeviceArray[m_PhysicalDeviceIndex], &deviceCreateInfo, NULL, &m_LogicalDevice);

		if (result)
		{
			// Failed to create the potential logical device
			continue;
		}

		vkGetPhysicalDeviceProperties(m_PhysicalDeviceArray[i], &m_PhysicalDeviceProperties);
		vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDeviceArray[m_PhysicalDeviceIndex], VulkanGraphicsResourceSurface::GetInstance().GetSurface(), &count, NULL);
		m_SurfaceFormatArray.resize(count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDeviceArray[m_PhysicalDeviceIndex], VulkanGraphicsResourceSurface::GetInstance().GetSurface(), &count, m_SurfaceFormatArray.data());

		if (m_SurfaceFormatArray.size() <= 0)
		{
			continue;
		}

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDeviceArray[m_PhysicalDeviceIndex], VulkanGraphicsResourceSurface::GetInstance().GetSurface(), &m_SurfaceCapabilities);
		vkGetPhysicalDeviceMemoryProperties(m_PhysicalDeviceArray[m_PhysicalDeviceIndex], &m_MemoryProperties);
		printf_console("\t+ local device is created with the physical device %d\n", m_PhysicalDeviceIndex);
		printf_console("\t+++++ [Surface Formant] count : %d\n", count);

		for (int j = 0; j < count; ++j)
		{
			printf_console("\t++++++++++ index: %d\n", j);
			printf_console("\t+++++++++++++++ format: %d\n", m_SurfaceFormatArray[j].format);
			printf_console("\t+++++++++++++++ colorSpace: %d\n", m_SurfaceFormatArray[j].colorSpace);
		}

		printf_console("\t+++++ [Surface Capabilities]\n");
		printf_console("\t++++++++++ minImageCount: %d\n", m_SurfaceCapabilities.minImageCount);
		printf_console("\t++++++++++ maxImageCount: %d\n", m_SurfaceCapabilities.maxImageCount);
		printf_console("\t++++++++++ currentExtent (%d, %d)\n", m_SurfaceCapabilities.currentExtent.width, m_SurfaceCapabilities.currentExtent.height);
		printf_console("\t++++++++++ minImageExtent (%d, %d)\n", m_SurfaceCapabilities.minImageExtent.width, m_SurfaceCapabilities.minImageExtent.height);
		printf_console("\t++++++++++ maxImageExtent (%d, %d)\n", m_SurfaceCapabilities.maxImageExtent.width, m_SurfaceCapabilities.maxImageExtent.height);
		printf_console("\t++++++++++ maxImageArrayLayers: %d\n", m_SurfaceCapabilities.maxImageArrayLayers);
		printf_console("\t++++++++++ supportedTransforms: %x\n", m_SurfaceCapabilities.supportedTransforms);
		printf_console("\t++++++++++ currentTransform: %x\n", m_SurfaceCapabilities.currentTransform);
		printf_console("\t++++++++++ supportedCompositeAlpha: %x\n", m_SurfaceCapabilities.supportedCompositeAlpha);
		printf_console("\t++++++++++ supportedUsageFlags: %x\n", m_SurfaceCapabilities.supportedUsageFlags);
		printf_console("\t+++++ [Memory Capabilities]\n");
		printf_console("\t++++++++++ memoryTypeCount: %d\n", m_MemoryProperties.memoryTypeCount);

		for (int j = 0; j < m_MemoryProperties.memoryTypeCount; ++j)
		{
			printf_console("\t+++++++++++++++ memoryType: %d\n", m_MemoryProperties.memoryTypes[j]);
		}

		printf_console("\t++++++++++ memoryHeapCount: %d\n", m_MemoryProperties.memoryHeapCount);

		for (int j = 0; j < m_MemoryProperties.memoryHeapCount; ++j)
		{
			printf_console("\t+++++++++++++++ memoryHeap: %d\n", m_MemoryProperties.memoryHeaps[j]);
		}

		m_QueueMap.clear();
		GetDeviceQueueOnlyIfNotExist(m_GraphicsQueueFamilyIndex);
		GetDeviceQueueOnlyIfNotExist(m_PresentQueueFamilyIndex);
		GetDeviceQueueOnlyIfNotExist(m_ComputeQueueFamilyIndex);

		return true;
	}

	printf_console("[VulkanGraphics] failed to create a logical device\n");

	return false;
}

bool VulkanGraphicsResourceDevice::DestroyLogicalDevice()
{
	vkDestroyDevice(m_LogicalDevice, NULL);
	m_PhysicalDeviceIndex = -1;
	m_GraphicsQueueFamilyIndex = -1;
	m_PresentQueueFamilyIndex = -1;
	m_ComputeQueueFamilyIndex = -1;
	return true;
}

void VulkanGraphicsResourceDevice::GetDeviceQueueOnlyIfNotExist(int queueFamilyIndex)
{
	if (m_QueueMap.find(queueFamilyIndex) != m_QueueMap.end())
	{
		return;
	}

	auto newQueue = VkQueue();
	vkGetDeviceQueue(m_LogicalDevice, queueFamilyIndex, 0, &newQueue);
	m_QueueMap[queueFamilyIndex] = newQueue;
}