#include "VulkanGraphicsResourceDevice.h"
#include "../DebugUtility.h"
#include "VulkanGraphicsResourceInstance.h"
#include "VulkanGraphicsResourceSurface.h"

std::vector<VkPhysicalDevice> VulkanGraphicsResourceDevice::s_PhysicalDeviceArray; // TODO: Think about removing this because it seems redundant...
int VulkanGraphicsResourceDevice::s_PhysicalDeviceIndex = -1;
int VulkanGraphicsResourceDevice::s_GraphicsQueueFamilyIndex = -1;
int VulkanGraphicsResourceDevice::s_PresentQueueFamilyIndex = -1;
int VulkanGraphicsResourceDevice::s_TransferQueueFamilyIndex = -1;
VkDevice VulkanGraphicsResourceDevice::s_LogicalDevice;
VkPhysicalDeviceProperties VulkanGraphicsResourceDevice::s_PhysicalDeviceProperties;
VkSurfaceCapabilitiesKHR VulkanGraphicsResourceDevice::s_SurfaceCapabilities;
std::vector<VkSurfaceFormatKHR> VulkanGraphicsResourceDevice::s_SurfaceFormatArray;
VkPhysicalDeviceMemoryProperties VulkanGraphicsResourceDevice::s_MemoryProperties;
VkQueue VulkanGraphicsResourceDevice::s_GraphicsQueue;
VkQueue VulkanGraphicsResourceDevice::s_PresentQueue;
VkQueue VulkanGraphicsResourceDevice::s_TransferQueue;

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
	auto result = vkEnumeratePhysicalDevices(VulkanGraphicsResourceInstance::GetInstance(), &count, NULL);

	if (result)
	{
		printf_console("[VulkanGraphics] failed to enumerate physical devices with %d\n", result);
		return false;
	}

	s_PhysicalDeviceArray.resize(count);
	result = vkEnumeratePhysicalDevices(VulkanGraphicsResourceInstance::GetInstance(), &count, s_PhysicalDeviceArray.data());

	if (result)
	{
		printf_console("[VulkanGraphics] failed to enumerate physical devices with error code %d\n", result);
		return false;
	}

	printf_console("[VulkanGraphics] physical device count is %d\n", s_PhysicalDeviceArray.size());
	return true;
}

class PotentialLocalDeviceOption
{
public:
	int _GraphicsQueueFamilyIndex = -1;
	int _PresentQueueFamilyIndex = -1;
	int _TransferQueueFamilyIndex = -1;
};

bool VulkanGraphicsResourceDevice::CreateLogicalDevice()
{
	DestroyLogicalDevice();

	std::vector<PotentialLocalDeviceOption> potentialOptions;
	uint32_t count = 0;
	std::vector<VkQueueFamilyProperties> queueFamilyPropertiesArray;

	for (int i = 0; i < s_PhysicalDeviceArray.size(); ++i)
	{
		printf_console("\t%d device ++++++++++\n", i);
		s_PhysicalDeviceIndex = -1;
		s_GraphicsQueueFamilyIndex = -1;
		s_PresentQueueFamilyIndex = -1;
		potentialOptions.clear();
		vkGetPhysicalDeviceQueueFamilyProperties(s_PhysicalDeviceArray[i], &count, NULL);
		queueFamilyPropertiesArray.resize(count);
		vkGetPhysicalDeviceQueueFamilyProperties(s_PhysicalDeviceArray[i], &count, queueFamilyPropertiesArray.data());
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

			if (queueFamilyPropertiesArray[j].queueFlags & VK_QUEUE_TRANSFER_BIT)
			{
				option._TransferQueueFamilyIndex = j;
				isPassed = true;
			}

			{
				VkBool32 doesSupportPresent = VK_FALSE;
				vkGetPhysicalDeviceSurfaceSupportKHR(s_PhysicalDeviceArray[i], j, VulkanGraphicsResourceSurface::GetSurface(), &doesSupportPresent);

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

		s_PhysicalDeviceIndex = i;

		for (int j = 0; j < potentialOptions.size(); ++j)
		{
			if (potentialOptions[j]._GraphicsQueueFamilyIndex != -1 && potentialOptions[j]._PresentQueueFamilyIndex != -1)
			{
				s_GraphicsQueueFamilyIndex = potentialOptions[j]._GraphicsQueueFamilyIndex;
				s_PresentQueueFamilyIndex = potentialOptions[j]._PresentQueueFamilyIndex;
			}

			if (potentialOptions[j]._GraphicsQueueFamilyIndex == -1 && potentialOptions[j]._TransferQueueFamilyIndex != -1)
			{
				s_TransferQueueFamilyIndex = potentialOptions[j]._TransferQueueFamilyIndex;
			}

			if (s_GraphicsQueueFamilyIndex == -1 && potentialOptions[j]._GraphicsQueueFamilyIndex != -1)
			{
				s_GraphicsQueueFamilyIndex = potentialOptions[j]._GraphicsQueueFamilyIndex;
			}

			if (s_PresentQueueFamilyIndex == -1 && potentialOptions[j]._PresentQueueFamilyIndex != -1)
			{
				s_PresentQueueFamilyIndex = potentialOptions[j]._PresentQueueFamilyIndex;
			}

			if (s_TransferQueueFamilyIndex == -1 && potentialOptions[j]._TransferQueueFamilyIndex != -1)
			{
				s_TransferQueueFamilyIndex = potentialOptions[j]._TransferQueueFamilyIndex;
			}
		}

		if (s_GraphicsQueueFamilyIndex == -1 || s_PresentQueueFamilyIndex == -1 || s_TransferQueueFamilyIndex == -1)
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
		queueCreateInfo.queueFamilyIndex = s_GraphicsQueueFamilyIndex;
		queueCreateInfoArray.push_back(queueCreateInfo);

		if (s_PresentQueueFamilyIndex != s_GraphicsQueueFamilyIndex)
		{
			queueCreateInfo.queueFamilyIndex = s_PresentQueueFamilyIndex;
			queueCreateInfoArray.push_back(queueCreateInfo);
		}

		if (s_TransferQueueFamilyIndex != s_GraphicsQueueFamilyIndex && s_TransferQueueFamilyIndex != s_PresentQueueFamilyIndex)
		{
			queueCreateInfo.queueFamilyIndex = s_TransferQueueFamilyIndex;
			queueCreateInfoArray.push_back(queueCreateInfo);
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

		auto result = vkCreateDevice(s_PhysicalDeviceArray[s_PhysicalDeviceIndex], &deviceCreateInfo, NULL, &s_LogicalDevice);

		if (result)
		{
			// Failed to create the potential logical device
			continue;
		}

		vkGetPhysicalDeviceProperties(s_PhysicalDeviceArray[i], &s_PhysicalDeviceProperties);
		vkGetPhysicalDeviceSurfaceFormatsKHR(s_PhysicalDeviceArray[s_PhysicalDeviceIndex], VulkanGraphicsResourceSurface::GetSurface(), &count, NULL);
		s_SurfaceFormatArray.resize(count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(s_PhysicalDeviceArray[s_PhysicalDeviceIndex], VulkanGraphicsResourceSurface::GetSurface(), &count, s_SurfaceFormatArray.data());

		if (s_SurfaceFormatArray.size() <= 0)
		{
			continue;
		}

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(s_PhysicalDeviceArray[s_PhysicalDeviceIndex], VulkanGraphicsResourceSurface::GetSurface(), &s_SurfaceCapabilities);
		vkGetPhysicalDeviceMemoryProperties(s_PhysicalDeviceArray[s_PhysicalDeviceIndex], &s_MemoryProperties);
		printf_console("\t+ local device is created with the physical device %d\n", s_PhysicalDeviceIndex);
		printf_console("\t+++++ [Surface Formant] count : %d\n", count);

		for (int j = 0; j < count; ++j)
		{
			printf_console("\t++++++++++ index: %d\n", j);
			printf_console("\t+++++++++++++++ format: %d\n", s_SurfaceFormatArray[j].format);
			printf_console("\t+++++++++++++++ colorSpace: %d\n", s_SurfaceFormatArray[j].colorSpace);
		}

		printf_console("\t+++++ [Surface Capabilities]\n");
		printf_console("\t++++++++++ minImageCount: %d\n", s_SurfaceCapabilities.minImageCount);
		printf_console("\t++++++++++ maxImageCount: %d\n", s_SurfaceCapabilities.maxImageCount);
		printf_console("\t++++++++++ currentExtent (%d, %d)\n", s_SurfaceCapabilities.currentExtent.width, s_SurfaceCapabilities.currentExtent.height);
		printf_console("\t++++++++++ minImageExtent (%d, %d)\n", s_SurfaceCapabilities.minImageExtent.width, s_SurfaceCapabilities.minImageExtent.height);
		printf_console("\t++++++++++ maxImageExtent (%d, %d)\n", s_SurfaceCapabilities.maxImageExtent.width, s_SurfaceCapabilities.maxImageExtent.height);
		printf_console("\t++++++++++ maxImageArrayLayers: %d\n", s_SurfaceCapabilities.maxImageArrayLayers);
		printf_console("\t++++++++++ supportedTransforms: %x\n", s_SurfaceCapabilities.supportedTransforms);
		printf_console("\t++++++++++ currentTransform: %x\n", s_SurfaceCapabilities.currentTransform);
		printf_console("\t++++++++++ supportedCompositeAlpha: %x\n", s_SurfaceCapabilities.supportedCompositeAlpha);
		printf_console("\t++++++++++ supportedUsageFlags: %x\n", s_SurfaceCapabilities.supportedUsageFlags);
		printf_console("\t+++++ [Memory Capabilities]\n");
		printf_console("\t++++++++++ memoryTypeCount: %d\n", s_MemoryProperties.memoryTypeCount);

		for (int j = 0; j < s_MemoryProperties.memoryTypeCount; ++j)
		{
			printf_console("\t+++++++++++++++ memoryType: %d\n", s_MemoryProperties.memoryTypes[j]);
		}

		printf_console("\t++++++++++ memoryHeapCount: %d\n", s_MemoryProperties.memoryHeapCount);

		for (int j = 0; j < s_MemoryProperties.memoryHeapCount; ++j)
		{
			printf_console("\t+++++++++++++++ memoryHeap: %d\n", s_MemoryProperties.memoryHeaps[j]);
		}

		vkGetDeviceQueue(s_LogicalDevice, s_GraphicsQueueFamilyIndex, 0, &s_GraphicsQueue);
		vkGetDeviceQueue(s_LogicalDevice, s_PresentQueueFamilyIndex, 0, &s_PresentQueue);
		vkGetDeviceQueue(s_LogicalDevice, s_TransferQueueFamilyIndex, 0, &s_TransferQueue);
		return true;
	}

	printf_console("[VulkanGraphics] failed to create a logical device\n");
	return false;
}

bool VulkanGraphicsResourceDevice::DestroyLogicalDevice()
{
	vkDestroyDevice(s_LogicalDevice, NULL);
	s_PhysicalDeviceIndex = -1;
	s_GraphicsQueueFamilyIndex = -1;
	s_PresentQueueFamilyIndex = -1;
	return true;
}