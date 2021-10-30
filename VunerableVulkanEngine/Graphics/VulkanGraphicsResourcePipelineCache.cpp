#include "VulkanGraphicsResourcePipelineCache.h"
#include "VulkanGraphicsResourceDevice.h"

#include "../DebugUtility.h"

const char* PIPELINE_CACHE_FILE_NAME = "vkpipeline_cache_data.bin";

VulkanGraphicsResourcePipelineCache g_Instance;

VulkanGraphicsResourcePipelineCache& VulkanGraphicsResourcePipelineCache::GetInstance()
{
	return g_Instance;
}

bool VulkanGraphicsResourcePipelineCache::CreateInternal()
{
    std::vector<uint8_t> dataArray;
    FILE* file = fopen(PIPELINE_CACHE_FILE_NAME, "rb");

    if (file)
    {
        fseek(file, 0, SEEK_END);

        size_t dataSize = ftell(file);
        dataArray.resize(dataSize);
        rewind(file);

        size_t readSize = fread(dataArray.data(), 1, dataSize, file);

        if (dataSize != readSize)
        {
            dataArray.clear();
        }

        fclose(file);
    }

    if (dataArray.size() > 0)
    {
        uint8_t* pStart = dataArray.data();
        uint32_t headerLength = 0;
        uint32_t cacheHeaderVersion = 0;
        uint32_t vendorID = 0;
        uint32_t deviceID = 0;
        uint8_t pipelineCacheUUID[VK_UUID_SIZE] = {};

        memcpy(&headerLength, pStart + 0, 4);
        memcpy(&cacheHeaderVersion, pStart + 4, 4);
        memcpy(&vendorID, pStart + 8, 4);
        memcpy(&deviceID, pStart + 12, 4);
        memcpy(pipelineCacheUUID, pStart + 16, VK_UUID_SIZE);

        // Check each field and report bad values before freeing existing cache
        bool badCache = false;

        if (headerLength <= 0)
        {
            badCache = true;
            printf_console("  Bad header length.\n");
            printf_console("    Cache contains: 0x%.8x\n", headerLength);
        }

        if (cacheHeaderVersion != VK_PIPELINE_CACHE_HEADER_VERSION_ONE)
        {
            badCache = true;
            printf_console("  Unsupported cache header version.\n");
            printf_console("    Cache contains: 0x%.8x\n", cacheHeaderVersion);
        }

        auto deviceProperties = VulkanGraphicsResourceDevice::GetInstance().GetPhysicalDeviceProperties();

        if (vendorID != deviceProperties.vendorID)
        {
            badCache = true;
            printf_console("  Vendor ID mismatch.\n");
            printf_console("    Cache contains: 0x%.8x\n", vendorID);
            printf_console("    Driver expects: 0x%.8x\n", deviceProperties.vendorID);
        }

        if (deviceID != deviceProperties.deviceID)
        {
            badCache = true;
            printf_console("  Device ID mismatch.\n");
            printf_console("    Cache contains: 0x%.8x\n", deviceID);
            printf_console("    Driver expects: 0x%.8x\n", deviceProperties.deviceID);
        }

        if (memcmp(pipelineCacheUUID, deviceProperties.pipelineCacheUUID, sizeof(pipelineCacheUUID)) != 0)
        {
            badCache = true;
            printf_console("  UUID mismatch.\n");
        }

        if (badCache)
        {
            printf_console("[VulkanGraphics] loaded an invalid pipeline cache.\n");
            dataArray.clear();
            //throw;  // TODO: need to check why invalid cache is created...(NECESSARY!!!)
        }
    }


    auto createInfo = VkPipelineCacheCreateInfo();
    createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    createInfo.pNext = NULL;
    createInfo.flags = 0; // TODO: check out what each option means...
    createInfo.initialDataSize = dataArray.size();
    createInfo.pInitialData = dataArray.data();

    auto result = vkCreatePipelineCache(VulkanGraphicsResourceDevice::GetInstance().GetLogicalDevice(), &createInfo, NULL, &m_PipelineCache);

    if (result)
    {
        printf_console("[VulkanGraphics] failed to create a pipeline cache with error code %d\n", result);
        throw;
    }

    return true;
}

bool VulkanGraphicsResourcePipelineCache::DestroyInternal()
{
    std::vector<uint8_t> dataArray;
    size_t size;
    vkGetPipelineCacheData(VulkanGraphicsResourceDevice::GetInstance().GetLogicalDevice(), m_PipelineCache, &size, NULL);
    dataArray.resize(size);
    vkGetPipelineCacheData(VulkanGraphicsResourceDevice::GetInstance().GetLogicalDevice(), m_PipelineCache, &size, dataArray.data());

    FILE* file = fopen(PIPELINE_CACHE_FILE_NAME, "wb");

    if (file)
    {
        fwrite(dataArray.data(), 1, size, file);
        fclose(file);
    }

    return true;
}