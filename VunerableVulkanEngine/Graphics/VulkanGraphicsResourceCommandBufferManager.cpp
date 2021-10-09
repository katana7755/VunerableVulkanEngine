#include "VulkanGraphicsResourceCommandBufferManager.h"
#include "../DebugUtility.h"
#include "VulkanGraphicsResourceDevice.h"
#include "VulkanGraphicsResourceSwapchain.h"
#include <algorithm>

VulkanGraphicsResourceCommandBufferManager g_Instance;

VulkanQueueSubmitNode::VulkanQueueSubmitNode()
{
	size_t semaphoreIdentifier = VulkanGraphicsResourceSemaphoreManager::GetInstance().AllocateIdentifier();
	VulkanGraphicsResourceSemaphoreManager::GetInstance().CreateResource(semaphoreIdentifier, semaphoreIdentifier, semaphoreIdentifier);
	m_SignalSemaphore = VulkanGraphicsResourceSemaphoreManager::GetInstance().GetResource(semaphoreIdentifier);
}

bool VulkanQueueSubmitNode::Aggregate(EVulkanCommandType::TYPE commandType, const VkCommandBuffer& commandBuffer, const VulkanGfxObjectUsage& gfxObjectUsage)
{
	auto iterTextureBegin = m_AggregatedGfxObjectUsage.m_WriteTextureArray.begin();
	auto iterTextureEnd = m_AggregatedGfxObjectUsage.m_WriteTextureArray.end();

	for (auto texture : gfxObjectUsage.m_ReadTextureArray)
	{
		if (std::find(iterTextureBegin, iterTextureEnd, texture) != iterTextureEnd)
		{
			return false;
		}
	}

	for (auto texture : gfxObjectUsage.m_WriteTextureArray)
	{
		if (std::find(iterTextureBegin, iterTextureEnd, texture) != iterTextureEnd)
		{
			return false;
		}
	}

	auto iterBufferBegin = m_AggregatedGfxObjectUsage.m_WriteBufferArray.begin();
	auto iterBufferEnd = m_AggregatedGfxObjectUsage.m_WriteBufferArray.end();

	for (auto buffer : gfxObjectUsage.m_ReadBufferArray)
	{
		if (std::find(iterBufferBegin, iterBufferEnd, buffer) != iterBufferEnd)
		{
			return false;
		}
	}

	for (auto buffer : gfxObjectUsage.m_WriteBufferArray)
	{
		if (std::find(iterBufferBegin, iterBufferEnd, buffer) != iterBufferEnd)
		{
			return false;
		}
	}

	VkQueue queue = VK_NULL_HANDLE;

	switch (commandType)
	{
	case EVulkanCommandType::GRAPHICS:
		queue = VulkanGraphicsResourceDevice::GetGraphicsQueue();
		break;

	case EVulkanCommandType::COMPUTE:
		queue = VulkanGraphicsResourceDevice::GetComputeQueue();
		break;

	case EVulkanCommandType::TRANSFER:
		queue = VulkanGraphicsResourceDevice::GetTransferQueue();
		break;

	default:
		printf_console("[VulkanGraphics] failed to aggregate command buffers into a submission node");
		throw;
	}

	m_AggregatedCommandBufferArrayMap[queue].push_back(commandBuffer);
	m_AggregatedGfxObjectUsage.Aggregate(gfxObjectUsage);

	return true;
}

VulkanGraphicsResourceCommandBufferManager& VulkanGraphicsResourceCommandBufferManager::GetInstance()
{
	return g_Instance;
}

void VulkanGraphicsResourceCommandBufferManager::Initialize()
{
	CreateSingleCommandPool(EVulkanCommandType::GRAPHICS, true);
	CreateSingleCommandPool(EVulkanCommandType::COMPUTE, true);
	CreateSingleCommandPool(EVulkanCommandType::TRANSFER, true);

	CreateSingleCommandPool(EVulkanCommandType::GRAPHICS, false);
	CreateSingleCommandPool(EVulkanCommandType::COMPUTE, false);
	CreateSingleCommandPool(EVulkanCommandType::TRANSFER, false);
}

void VulkanGraphicsResourceCommandBufferManager::Deinitialize()
{
	FreeAllStaticCommandBuffers();
	FreeAllSemaphores();
	
	DestroySingleCommandPool(EVulkanCommandType::GRAPHICS, true);
	DestroySingleCommandPool(EVulkanCommandType::COMPUTE, true);
	DestroySingleCommandPool(EVulkanCommandType::TRANSFER, true);

	DestroySingleCommandPool(EVulkanCommandType::GRAPHICS, false);
	DestroySingleCommandPool(EVulkanCommandType::COMPUTE, false);
	DestroySingleCommandPool(EVulkanCommandType::TRANSFER, false);
}

VulkanCommandBufferOutputData VulkanGraphicsResourceCommandBufferManager::CreateResourcePhysically(const VulkanCommandBufferInputData& inputData)
{
	auto outputData = VulkanCommandBufferOutputData();
	outputData.m_CommandType = inputData.m_CommandType;
	outputData.m_IsTransient = inputData.m_IsTransient;
	outputData.m_SortOrder = inputData.m_SortOrder;
	outputData.m_RecordedCommandBuffer = VK_NULL_HANDLE;

	return outputData;
}

void VulkanGraphicsResourceCommandBufferManager::DestroyResourcePhysicially(const VulkanCommandBufferOutputData& outputData)
{
	if (outputData.m_IsTransient)
	{
		return;
	}

	if (outputData.m_RecordedCommandBuffer == VK_NULL_HANDLE)
	{
		return;
	}

	auto commandPool = outputData.m_IsTransient ? m_TransientCommandPoolMap[outputData.m_CommandType] : m_StaticCommandPoolMap[outputData.m_CommandType];
	vkFreeCommandBuffers(VulkanGraphicsResourceDevice::GetLogicalDevice(), commandPool, 1, &outputData.m_RecordedCommandBuffer);
}

void VulkanGraphicsResourceCommandBufferManager::FreeAllStaticCommandBuffers()
{
	std::unordered_map<EVulkanCommandType::TYPE, VulkanCommandBufferArray> commandBufferArrayMap;

	for (int i = m_ResourceArray.size() - 1; i >= 0; --i)
	{
		auto& outputData = m_ResourceArray[i];

		if (outputData.m_IsTransient)
		{
			continue;
		}

		if (outputData.m_RecordedCommandBuffer == VK_NULL_HANDLE)
		{
			continue;
		}

		commandBufferArrayMap[outputData.m_CommandType].push_back(outputData.m_RecordedCommandBuffer);		
		outputData.m_RecordedCommandBuffer = VK_NULL_HANDLE;
	}

	for (int i = 0; i < EVulkanCommandType::MAX; ++i)
	{
		auto commandPool = m_StaticCommandPoolMap[(EVulkanCommandType::TYPE)i];
		auto commandBufferArray = commandBufferArrayMap[(EVulkanCommandType::TYPE)i];

		if (commandBufferArray.size() <= 0)
		{
			continue;
		}

		vkFreeCommandBuffers(VulkanGraphicsResourceDevice::GetLogicalDevice(), commandPool, commandBufferArray.size(), commandBufferArray.data());
		commandBufferArray.clear();
	}
}

void VulkanGraphicsResourceCommandBufferManager::FreeAllSemaphores()
{
	for (size_t identifier : m_SemaphoreIdentifierArray)
	{
		VulkanGraphicsResourceSemaphoreManager::GetInstance().DestroyResource(identifier);
		VulkanGraphicsResourceSemaphoreManager::GetInstance().ReleaseIdentifier(identifier);
	}
}

void VulkanGraphicsResourceCommandBufferManager::ReserveIndexForCommandBuffer(EVulkanCommandType::TYPE commandType, int index, std::unordered_map<EVulkanCommandType::TYPE, IndexArray>& resourceIndexArrayMap, size_t countArray[EVulkanCommandType::MAX])
{
	resourceIndexArrayMap[commandType].push_back(index);
	++countArray[commandType];
}

void VulkanGraphicsResourceCommandBufferManager::AllocateAndRecordCommandBuffer(std::unordered_map<EVulkanCommandType::TYPE, IndexArray>& resourceIndexArrayMap, const size_t countArray[EVulkanCommandType::MAX])
{
	for (size_t iType = 0; iType < EVulkanCommandType::MAX; ++iType)
	{
		if (countArray[(EVulkanCommandType::TYPE)iType] <= 0)
		{
			return;
		}

		auto commandBufferArray = std::vector<VkCommandBuffer>();
		int offset = commandBufferArray.size();
		size_t count = countArray[(EVulkanCommandType::TYPE)iType];
		commandBufferArray.resize(offset + count);

		auto indexArray = resourceIndexArrayMap[(EVulkanCommandType::TYPE)iType];
		auto allocateInfo = VkCommandBufferAllocateInfo();
		allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocateInfo.pNext = NULL;
		allocateInfo.commandPool = m_ResourceArray[indexArray[0]].m_IsTransient ? m_TransientCommandPoolMap[(EVulkanCommandType::TYPE)iType] : m_StaticCommandPoolMap[(EVulkanCommandType::TYPE)iType];
		allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocateInfo.commandBufferCount = count;
		vkAllocateCommandBuffers(VulkanGraphicsResourceDevice::GetLogicalDevice(), &allocateInfo, commandBufferArray.data() + offset);

		auto flags = m_ResourceArray[indexArray[0]].m_IsTransient ? VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT : VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

		for (size_t i = 0; i < count; ++i)
		{
			auto& outputData = m_ResourceArray[indexArray[i]];
			outputData.m_RecordedCommandBuffer = commandBufferArray[offset + i];

			// Record the current command buffer???
			auto beginInfo = VkCommandBufferBeginInfo();
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.pNext = NULL;
			beginInfo.flags = flags;
			beginInfo.pInheritanceInfo = NULL; // No secondary buffer
			vkBeginCommandBuffer(outputData.m_RecordedCommandBuffer, &beginInfo);

			for (auto execution : outputData.m_ExecutionArray)
			{
				execution.Execute(outputData.m_RecordedCommandBuffer, outputData.m_RecordedGfxObjectUsage);
			}

			vkEndCommandBuffer(outputData.m_RecordedCommandBuffer);
		}
	}
}

void VulkanGraphicsResourceCommandBufferManager::BuildRenderGraph()
{
	FreeAllSemaphores();

	// Calculate how many command buffers we need
	std::unordered_map<EVulkanCommandType::TYPE, IndexArray> staticResourceIndexArrayMap;
	std::unordered_map<EVulkanCommandType::TYPE, IndexArray> transientResourceIndexArrayMap;
	size_t staticCountArray[EVulkanCommandType::MAX] = { 0 };
	size_t transientCountArray[EVulkanCommandType::MAX] = { 0 };

	for (size_t i = 0; i < m_ResourceArray.size(); ++i)
	{
		auto& outputData = m_ResourceArray[i];

		if (outputData.m_ExecutionArray.size() <= 0)
		{
			continue;
		}

		if (outputData.m_RecordedCommandBuffer != VK_NULL_HANDLE)
		{
			continue;
		}

		if (outputData.m_IsTransient)
		{
			ReserveIndexForCommandBuffer(outputData.m_CommandType, i, transientResourceIndexArrayMap, transientCountArray);
		}
		else
		{
			ReserveIndexForCommandBuffer(outputData.m_CommandType, i, staticResourceIndexArrayMap, staticCountArray);
		}
	}

	// Allocate command buffers	
	AllocateAndRecordCommandBuffer(transientResourceIndexArrayMap, transientCountArray);
	AllocateAndRecordCommandBuffer(staticResourceIndexArrayMap, staticCountArray);

	// Sort output datas
	auto sortedOutputDataArray = std::vector<VulkanCommandBufferOutputData>(m_ResourceArray);
	std::sort(sortedOutputDataArray.begin(), sortedOutputDataArray.end(), [](const auto& lhs, const auto& rhs) -> bool
	{
		return lhs.m_SortOrder < lhs.m_SortOrder;
	});

	// Generate queue submit data array
	std::vector<VulkanQueueSubmitNode> queueSubmitNodeArray;
	size_t aggregateIndex = 0;
	queueSubmitNodeArray.push_back(VulkanQueueSubmitNode());

	for (auto outputData : sortedOutputDataArray)
	{
		if (queueSubmitNodeArray[aggregateIndex].Aggregate(outputData.m_CommandType, outputData.m_RecordedCommandBuffer, outputData.m_RecordedGfxObjectUsage))
		{
			continue;
		}

		queueSubmitNodeArray.push_back(VulkanQueueSubmitNode());
		++aggregateIndex;

		if (queueSubmitNodeArray[aggregateIndex].Aggregate(outputData.m_CommandType, outputData.m_RecordedCommandBuffer, outputData.m_RecordedGfxObjectUsage) == false)
		{
			printf_console("[VulkanGraphics] failed to aggregate command buffers");

			throw;
		}
	}


	// Submit here??? or seperately in a different function????
	for (size_t i = 0; i < queueSubmitNodeArray.size(); ++i)
	{
		auto node = queueSubmitNodeArray[i];

		for (auto pair : node.m_AggregatedCommandBufferArrayMap)
		{
			auto& commandBufferArray = pair.second;
			auto submitInfo = VkSubmitInfo();
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.pNext = NULL;

			if (i > 0)
			{
				submitInfo.waitSemaphoreCount = 1;
				submitInfo.pWaitSemaphores = &queueSubmitNodeArray[i - 1].m_SignalSemaphore;
			}

			submitInfo.pWaitDstStageMask = NULL; // TODO: check how to use this in the future...
			submitInfo.commandBufferCount = commandBufferArray.size();
			submitInfo.pCommandBuffers = commandBufferArray.data();
			submitInfo.signalSemaphoreCount = 1;
			submitInfo.pSignalSemaphores = &node.m_SignalSemaphore;
			vkQueueSubmit(pair.first, 1, &submitInfo, VK_NULL_HANDLE);
		}
	}

	// Remove all transient requests
	for (size_t i = m_ResourceArray.size() - 1; i >= 0; --i)
	{
		auto& outputData = m_ResourceArray[i];

		if (!outputData.m_IsTransient)
		{
			continue;
		}

		DestroyResourceByDirectIndex(i);
	}
}

void VulkanGraphicsResourceCommandBufferManager::SubmitAll()
{
	// Allocate Command Buffers
	// Record Commands
	// Submit Command Buffers
	// Remove Transient Command Buffers
}

void VulkanGraphicsResourceCommandBufferManager::CreateSingleCommandPool(EVulkanCommandType::TYPE commandType, bool isTransient)
{
	auto createInfo = VkCommandPoolCreateInfo();
	createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	createInfo.pNext = NULL;
	createInfo.flags = isTransient ? VK_COMMAND_POOL_CREATE_TRANSIENT_BIT : 0;

	switch (commandType)
	{
		case EVulkanCommandType::GRAPHICS:
			createInfo.queueFamilyIndex = VulkanGraphicsResourceDevice::GetGraphicsQueueFamilyIndex();
			break;

		case EVulkanCommandType::COMPUTE:
			createInfo.queueFamilyIndex = VulkanGraphicsResourceDevice::GetComputeQueueFamilyIndex();
			break;

		case EVulkanCommandType::TRANSFER:
			createInfo.queueFamilyIndex = VulkanGraphicsResourceDevice::GetTransferQueueFamilyIndex();
			break;

		default:
			throw;
	}

	auto newCommandPool = VkCommandPool();
	auto result = vkCreateCommandPool(VulkanGraphicsResourceDevice::GetLogicalDevice(), &createInfo, NULL, &newCommandPool);

	if (result)
	{
		printf_console("[VulkanGraphics] failed to create a command pool with error code %d\n", result);

		throw;
	}

	if (isTransient)
	{
		m_TransientCommandPoolMap[commandType] = newCommandPool;
	}
	else
	{
		m_StaticCommandPoolMap[commandType] = newCommandPool;
	}
}

void VulkanGraphicsResourceCommandBufferManager::DestroySingleCommandPool(EVulkanCommandType::TYPE commandType, bool isTransient)
{
	if (isTransient)
	{
		vkDestroyCommandPool(VulkanGraphicsResourceDevice::GetLogicalDevice(), m_TransientCommandPoolMap[commandType], NULL);
	}
	else
	{
		vkDestroyCommandPool(VulkanGraphicsResourceDevice::GetLogicalDevice(), m_StaticCommandPoolMap[commandType], NULL);
	}
}


VkCommandPool OldVulkanGraphicsResourceCommandBufferManager::s_CommandGraphicsPool;
std::vector<VkCommandBuffer> OldVulkanGraphicsResourceCommandBufferManager::s_PrimaryBufferArray;
std::vector<VkCommandBuffer> OldVulkanGraphicsResourceCommandBufferManager::s_AdditionalBufferArray;

void OldVulkanGraphicsResourceCommandBufferManager::AllocatePrimaryBufferArray()
{
	int count = VulkanGraphicsResourceSwapchain::GetImageViewCount();
	s_PrimaryBufferArray.resize(count);

	auto allocateInfo = VkCommandBufferAllocateInfo();
	allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocateInfo.pNext = NULL;
	allocateInfo.commandPool = s_CommandGraphicsPool;
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocateInfo.commandBufferCount = count;

	auto result = vkAllocateCommandBuffers(VulkanGraphicsResourceDevice::GetLogicalDevice(), &allocateInfo, s_PrimaryBufferArray.data());

	if (result)
	{
		printf_console("[VulkanGraphics] failed to create the primary command buffer with error code %d\n", result);

		throw;
	}
}

void OldVulkanGraphicsResourceCommandBufferManager::FreePrimaryBufferArray()
{
	vkFreeCommandBuffers(VulkanGraphicsResourceDevice::GetLogicalDevice(), s_CommandGraphicsPool, s_PrimaryBufferArray.size(), s_PrimaryBufferArray.data());
}

const VkCommandBuffer& OldVulkanGraphicsResourceCommandBufferManager::GetPrimaryCommandBuffer(int index)
{
	return s_PrimaryBufferArray[index];
}

const VkCommandBuffer& OldVulkanGraphicsResourceCommandBufferManager::AllocateAdditionalCommandBuffer()
{
	size_t offset = s_AdditionalBufferArray.size();
	s_AdditionalBufferArray.emplace_back();

	auto allocateInfo = VkCommandBufferAllocateInfo();
	allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocateInfo.pNext = NULL;
	allocateInfo.commandPool = s_CommandGraphicsPool;
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocateInfo.commandBufferCount = 1;

	auto result = vkAllocateCommandBuffers(VulkanGraphicsResourceDevice::GetLogicalDevice(), &allocateInfo, s_AdditionalBufferArray.data() + offset);

	if (result)
	{
		printf_console("[VulkanGraphics] failed to create the primary command buffer with error code %d\n", result);

		throw;
	}

	return s_AdditionalBufferArray[offset];
}

void OldVulkanGraphicsResourceCommandBufferManager::FreeAdditionalCommandBuffer(const VkCommandBuffer& commandBuffer)
{
	auto iter = s_AdditionalBufferArray.begin();
	auto end = s_AdditionalBufferArray.end();

	while (iter != end)
	{
		if ((*iter) == commandBuffer)
		{
			break;
		}

		++iter;
	}

	if (iter == end)
	{
		return;
	}

	//vkFreeCommandBuffers(VulkanGraphicsResourceDevice::GetLogicalDevice(), s_CommandGraphicsPool, 1, &commandBuffer);
	s_AdditionalBufferArray.erase(iter);
}

const std::vector<VkCommandBuffer>& OldVulkanGraphicsResourceCommandBufferManager::GetAllAdditionalCommandBuffers()
{
	return s_AdditionalBufferArray;
}

void OldVulkanGraphicsResourceCommandBufferManager::ClearAdditionalCommandBuffers()
{
	//vkFreeCommandBuffers(VulkanGraphicsResourceDevice::GetLogicalDevice(), s_CommandGraphicsPool, s_AdditionalBufferArray.size(), s_AdditionalBufferArray.data());
	s_AdditionalBufferArray.clear();
}

bool OldVulkanGraphicsResourceCommandBufferManager::CreateInternal()
{
	auto createInfo = VkCommandPoolCreateInfo();
	createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	createInfo.pNext = NULL;
	createInfo.flags = 0;
	createInfo.queueFamilyIndex = VulkanGraphicsResourceDevice::GetGraphicsQueueFamilyIndex();

	auto result = vkCreateCommandPool(VulkanGraphicsResourceDevice::GetLogicalDevice(), &createInfo, NULL, &s_CommandGraphicsPool);

	if (result)
	{
		printf_console("[VulkanGraphics] failed to create a command pool with error code %d\n", result);

		throw;
	}

	return true;
}

bool OldVulkanGraphicsResourceCommandBufferManager::DestroyInternal()
{
	ClearAdditionalCommandBuffers();
	FreePrimaryBufferArray();
	vkDestroyCommandPool(VulkanGraphicsResourceDevice::GetLogicalDevice(), s_CommandGraphicsPool, NULL);

	return true;
}