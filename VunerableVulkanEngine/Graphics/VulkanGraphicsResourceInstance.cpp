#include "VulkanGraphicsResourceInstance.h"
#include <vector>
#include "../DebugUtility.h"

#ifdef _WIN32
#include <Windows.h>
#include "vulkan/vulkan_win32.h" // TODO: need to consider other platfroms such as Android, Linux etc... in the future
#endif

const char* APPLICATION_NAME = "VunerableVulkanEngine";
const char* ENGINE_NAME = "VunerableVulkanEngine";

VulkanGraphicsResourceInstance g_Instance;

VulkanGraphicsResourceInstance& VulkanGraphicsResourceInstance::GetInstance()
{
	return g_Instance;
}

bool VulkanGraphicsResourceInstance::CreateInternal()
{
	auto appInfo = VkApplicationInfo();
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pNext = NULL;
	appInfo.pApplicationName = APPLICATION_NAME;
	appInfo.applicationVersion = 1;
	appInfo.pEngineName = ENGINE_NAME;
	appInfo.engineVersion = 1;
	appInfo.apiVersion = VK_API_VERSION_1_2;

	auto extensionNameArray = std::vector<const char*>();
	extensionNameArray.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

#ifdef _WIN32
	extensionNameArray.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME); // TODO: need to consider other platfroms such as Android, Linux etc... in the future
#endif

	auto layerNameArray = std::vector<const char*>();

#ifdef _DEBUG
	layerNameArray.push_back("VK_LAYER_KHRONOS_validation");

	uint32_t availableLayerCount;
	vkEnumerateInstanceLayerProperties(&availableLayerCount, NULL);
	auto availableLayerArray = std::vector<VkLayerProperties>(availableLayerCount);
	vkEnumerateInstanceLayerProperties(&availableLayerCount, availableLayerArray.data());

	for (const char* layerName : layerNameArray)
	{
		bool found = false;

		for (const auto& availableLayer : availableLayerArray)
		{
			if (strcmp(availableLayer.layerName, layerName))
			{
				continue;
			}

			found = true;
			break;
		}

		if (!found)
		{
			printf_console("[VulkanGraphics] failed to create instance because %s layer is not possible to be created\n", layerName);
			return false;
		}
	}

	extensionNameArray.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

	auto createInfo = VkInstanceCreateInfo();
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pNext = NULL;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = extensionNameArray.size();
	createInfo.ppEnabledExtensionNames = extensionNameArray.data();
	createInfo.enabledLayerCount = layerNameArray.size();
	createInfo.ppEnabledLayerNames = layerNameArray.data();

	auto result = vkCreateInstance(&createInfo, NULL, &m_VkInstance);

	if (result)
	{
		printf_console("[VulkanGraphics] failed to create instance with %d\n", result);
	}
#ifdef _DEBUG
	else
	{
		result = CreateDebugUtilsMessenger();

		if (result)
		{
			printf_console("[VulkanGraphics] failed to create instance with %d while creating debug messenger\n", result);
		}
	}
#endif

	return result == VK_SUCCESS;
}

bool VulkanGraphicsResourceInstance::DestroyInternal()
{
#ifdef _DEBUG
	DestroyDebugUtilsMessenger();
#endif

	vkDestroyInstance(m_VkInstance, NULL);

	return true;
}

#ifdef _DEBUG
static VKAPI_ATTR VkBool32 VKAPI_CALL PrintDebugMessage(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
	printf_console("[VulkanGraphics] message from vulkan - [%d, %d] %s\n", severity, type, pCallbackData->pMessage);

	return VK_FALSE;
}

VkResult VulkanGraphicsResourceInstance::CreateDebugUtilsMessenger()
{
	auto createInfo = VkDebugUtilsMessengerCreateInfoEXT();
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
	createInfo.pfnUserCallback = PrintDebugMessage;
	createInfo.pUserData = NULL;

	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_VkInstance, "vkCreateDebugUtilsMessengerEXT");

	if (func == NULL)
	{
		return VkResult::VK_ERROR_EXTENSION_NOT_PRESENT;
	}

	return func(m_VkInstance, &createInfo, NULL, &m_DebugCallbackMessenger);
}

void VulkanGraphicsResourceInstance::DestroyDebugUtilsMessenger()
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_VkInstance, "vkDestroyDebugUtilsMessengerEXT");

	if (func == NULL)
	{
		printf_console("[VulkanGraphics] failed to find vkDestroyDebugUtilsMessengerEXT\n");
		return;
	}

	func(m_VkInstance, m_DebugCallbackMessenger, NULL);
}
#endif
