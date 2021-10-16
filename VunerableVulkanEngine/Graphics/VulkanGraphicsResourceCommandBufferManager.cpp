#include "VulkanGraphicsResourceCommandBufferManager.h"
#include "../DebugUtility.h"
#include "VulkanGraphicsResourceDevice.h"
#include "VulkanGraphicsResourceSwapchain.h"
#include "VulkanGraphicsResourceRenderPassManager.h"
#include "VulkanGraphicsResourceGraphicsPipelineManager.h"
#include "VulkanGraphicsResourcePipelineLayoutManager.h"
#include <algorithm>

VulkanGraphicsResourceCommandBufferManager g_Instance;

void VulkanQueueSubmitNode::Initialize()
{
	m_SignalSemaphoreIdentifier = VulkanGraphicsResourceSemaphoreManager::GetInstance().AllocateIdentifier();
	VulkanGraphicsResourceSemaphoreManager::GetInstance().CreateResource(m_SignalSemaphoreIdentifier, m_SignalSemaphoreIdentifier, m_SignalSemaphoreIdentifier);
}

void VulkanQueueSubmitNode::Deinitialize()
{
	VulkanGraphicsResourceSemaphoreManager::GetInstance().DestroyResource(m_SignalSemaphoreIdentifier);
	m_SignalSemaphoreIdentifier = (size_t)-1;
}

bool VulkanQueueSubmitNode::AggregateCommandBuffer(EVulkanCommandType::TYPE commandType, const VkCommandBuffer& commandBuffer, const VulkanGfxObjectUsage& gfxObjectUsage)
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
		printf_console("[VulkanGraphics] failed to aggregate command buffers into a submission node\n");
		throw;
	}

	m_AggregatedCommandBufferArrayMap[queue].push_back(commandBuffer);
	m_AggregatedGfxObjectUsage.Aggregate(gfxObjectUsage);

	return true;
}

const VkSemaphore& VulkanQueueSubmitNode::GetSemaphore()
{
	return VulkanGraphicsResourceSemaphoreManager::GetInstance().GetResource(m_SignalSemaphoreIdentifier);
}

namespace VulkanGfxExecution
{
	ExecutionPoolType g_ExecutionPool;

	ExecutionPoolType& GetExecutionPool()
	{
		return g_ExecutionPool;
	}

	void BeginRenderPassExecution::Execute(const VkCommandBuffer& commandBuffer, VulkanGfxObjectUsage& gfxObjectUsage)
	{
		auto beginInfo = VkRenderPassBeginInfo();
		beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		beginInfo.pNext = NULL;
		beginInfo.renderPass = VulkanGraphicsResourceRenderPassManager::GetRenderPass(m_RenderPassIndex);
		beginInfo.framebuffer = VulkanGraphicsResourceRenderPassManager::GetFramebuffer(m_FramebufferIndex);
		beginInfo.renderArea = m_RenderArea;

		auto clearValueArray = std::vector<VkClearValue>();

		if (m_FrameBufferColor != VK_NULL_HANDLE)
		{
			auto clearValue = VkClearValue();
			clearValue.color = { 0.0f, 0.0f, 0.0f, 1.0f };
			clearValue.depthStencil = { 0.0f, 0 };
			clearValueArray.push_back(clearValue);
			gfxObjectUsage.m_WriteTextureArray.push_back(m_FrameBufferColor);
		}

		if (m_FrameBufferDepth != VK_NULL_HANDLE)
		{
			auto clearValue = VkClearValue();
			clearValue.color = { 0.0f, 0.0f, 0.0f, 1.0f };
			clearValue.depthStencil = { 1.0f, 0 };
			clearValueArray.push_back(clearValue);
			gfxObjectUsage.m_WriteTextureArray.push_back(m_FrameBufferDepth);
		}

		beginInfo.clearValueCount = clearValueArray.size();
		beginInfo.pClearValues = clearValueArray.data();		
		vkCmdBeginRenderPass(commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
	}

	void BindPipelineExecution::Execute(const VkCommandBuffer& commandBuffer, VulkanGfxObjectUsage& gfxObjectUsage)
	{
		auto& pipelineData = VulkanGraphicsResourceGraphicsPipelineManager::GetInstance().GetResource(m_PipelineIdentifier);
		vkCmdBindPipeline(commandBuffer, pipelineData.GetBindPoint(), pipelineData.m_Pipeline);
	}

	void PushConstantsExecution::Execute(const VkCommandBuffer& commandBuffer, VulkanGfxObjectUsage& gfxObjectUsage)
	{
		auto& pipelineData = VulkanGraphicsResourceGraphicsPipelineManager::GetInstance().GetResource(m_PipelineIdentifier);
		auto& pipelineLayout = VulkanGraphicsResourcePipelineLayoutManager::GetInstance().GetResource(pipelineData.m_PipelineLayoutIdentifier);

		for (int i = 0; i < EVulkanShaderType::MAX; ++i)
		{
			auto rawDataArray = m_RawDataArrays[i];

			if (rawDataArray.size() == 0)
			{
				continue;
			}

			auto shaderMetaData = VulkanGraphicsResourceShaderManager::GetInstance().GetResourceKey(pipelineData.m_ShaderIdentifiers[i]);
			auto stageFlags = shaderMetaData.GetShaderStageBits();
			size_t offset = 0;

			for (int j = 0; j < rawDataArray.size(); ++j)
			{
				auto& rawData = rawDataArray[j];

				if (offset + rawData.size() > shaderMetaData.m_PushConstantsSize)
				{
					printf_console("[VulkanGraphics] found unmatched size data while pushing constants\n");

					throw;
				}

				vkCmdPushConstants(commandBuffer, pipelineLayout, stageFlags, offset, rawData.size(), rawData.data());
				offset += rawData.size();
			}
		}
	}

	void BindDescriptorSetsExecution::Execute(const VkCommandBuffer& commandBuffer, VulkanGfxObjectUsage& gfxObjectUsage)
	{
		auto& pipelineData = VulkanGraphicsResourceGraphicsPipelineManager::GetInstance().GetResource(m_PipelineIdentifier);
		auto& pipelineLayout = VulkanGraphicsResourcePipelineLayoutManager::GetInstance().GetResource(pipelineData.m_PipelineLayoutIdentifier);
		vkCmdBindDescriptorSets(commandBuffer, pipelineData.GetBindPoint(), pipelineLayout, 0, m_DescriptorSetArray.size(), m_DescriptorSetArray.data(), 0, NULL); // TODO: what is dynamic offset?
		gfxObjectUsage.Aggregate(m_GfxObjectUsage);
	}

	void DrawIndexedExectuion::Execute(const VkCommandBuffer& commandBuffer, VulkanGfxObjectUsage& gfxObjectUsage)
	{
		auto& pipelineData = VulkanGraphicsResourceGraphicsPipelineManager::GetInstance().GetResource(m_PipelineIdentifier);

		if (pipelineData.GetBindPoint() != VK_PIPELINE_BIND_POINT_GRAPHICS)
		{
			printf_console("[VulkanGraphics] failed to call draw indexed unless it is the graphics pipeline\n");

			throw;
		}

		std::vector<VkDeviceSize> offsetArray;
		offsetArray.push_back(0);
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &m_VertexBuffer, offsetArray.data());
		vkCmdBindIndexBuffer(commandBuffer, m_IndexBuffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(commandBuffer, m_IndexCount, m_InstanceCount, 0, 0, 0);
		gfxObjectUsage.Aggregate(m_GfxObjectUsage);
	}

	void CopyImageExecution::Execute(const VkCommandBuffer& commandBuffer, VulkanGfxObjectUsage& gfxObjectUsage)
	{
		auto imageBarrierArray = std::vector<VkImageMemoryBarrier>();
		{
			auto imageBarrier = VkImageMemoryBarrier();
			imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageBarrier.pNext = NULL;
			imageBarrier.srcAccessMask = m_SourceAccessMaskFrom;
			imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			imageBarrier.oldLayout = m_SourceLayoutFrom;
			imageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageBarrier.image = m_SourceImage;
			imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageBarrier.subresourceRange.baseMipLevel = 0;
			imageBarrier.subresourceRange.levelCount = 1;
			imageBarrier.subresourceRange.baseArrayLayer = 0;
			imageBarrier.subresourceRange.layerCount = 1;
			imageBarrierArray.push_back(imageBarrier);
		}
		{
			auto imageBarrier = VkImageMemoryBarrier();
			imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageBarrier.pNext = NULL;
			imageBarrier.srcAccessMask = m_DestinationAccessMaskFrom;
			imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			imageBarrier.oldLayout = m_DestinationLayoutFrom;
			imageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageBarrier.image = m_DestinationImage;
			imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageBarrier.subresourceRange.baseMipLevel = 0;
			imageBarrier.subresourceRange.levelCount = 1;
			imageBarrier.subresourceRange.baseArrayLayer = 0;
			imageBarrier.subresourceRange.layerCount = 1;
			imageBarrierArray.push_back(imageBarrier);
		}

		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, NULL, 0, NULL, imageBarrierArray.size(), imageBarrierArray.data());

		auto imageRegion = VkImageCopy();
		imageRegion.srcSubresource = VkImageSubresourceLayers{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
		imageRegion.srcOffset = VkOffset3D{ 0, 0, 0 };
		imageRegion.dstSubresource = VkImageSubresourceLayers{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
		imageRegion.dstOffset = VkOffset3D{ 0, 0, 0 };
		imageRegion.extent = VkExtent3D{ m_Width, m_Height, 1 };
		vkCmdCopyImage(commandBuffer, m_SourceImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_DestinationImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageRegion);
		imageBarrierArray.clear();
		{
			auto imageBarrier = VkImageMemoryBarrier();
			imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageBarrier.pNext = NULL;
			imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			imageBarrier.dstAccessMask = m_SourceAccessMaskTo;
			imageBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			imageBarrier.newLayout = m_SourceLayoutTo;
			imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageBarrier.image = m_SourceImage;
			imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageBarrier.subresourceRange.baseMipLevel = 0;
			imageBarrier.subresourceRange.levelCount = 1;
			imageBarrier.subresourceRange.baseArrayLayer = 0;
			imageBarrier.subresourceRange.layerCount = 1;
			imageBarrierArray.push_back(imageBarrier);
		}
		{
			auto imageBarrier = VkImageMemoryBarrier();
			imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageBarrier.pNext = NULL;
			imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			imageBarrier.dstAccessMask = m_DestinationAccessMaskTo;
			imageBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			imageBarrier.newLayout = m_DestinationLayoutTo;
			imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageBarrier.image = m_DestinationImage;
			imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageBarrier.subresourceRange.baseMipLevel = 0;
			imageBarrier.subresourceRange.levelCount = 1;
			imageBarrier.subresourceRange.baseArrayLayer = 0;
			imageBarrier.subresourceRange.layerCount = 1;
			imageBarrierArray.push_back(imageBarrier);
		}

		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, NULL, 0, NULL, imageBarrierArray.size(), imageBarrierArray.data());
	}

	void EndRenderPassExecution::Execute(const VkCommandBuffer& commandBuffer, VulkanGfxObjectUsage& gfxObjectUsage)
	{
		vkCmdEndRenderPass(commandBuffer);
	}
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
	FreeAllQueueSubmitNodes();
	
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
	for (auto executionPtr : outputData.m_ExecutionPtrArray)
	{
		std::type_index typeIndex = typeid(executionPtr);
		VulkanGfxExecution::GetExecutionPool().insert(VulkanGfxExecution::ExecutionPoolType::value_type(typeIndex, executionPtr));
	}

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

	for (size_t i = m_ResourceArray.size(); i > 0; --i)
	{
		auto& outputData = m_ResourceArray[i - 1];

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

		if (commandBufferArray.size() == 0)
		{
			continue;
		}

		vkFreeCommandBuffers(VulkanGraphicsResourceDevice::GetLogicalDevice(), commandPool, commandBufferArray.size(), commandBufferArray.data());
		commandBufferArray.clear();
	}
}

void VulkanGraphicsResourceCommandBufferManager::FreeAllQueueSubmitNodes()
{
	for (auto& node : m_QueueSubmitNodeArray)
	{
		node.Deinitialize();
	}

	m_QueueSubmitNodeArray.clear();
}

void VulkanGraphicsResourceCommandBufferManager::BuildAll()
{
	// Calculate how many command buffers we need
	std::unordered_map<EVulkanCommandType::TYPE, IndexArray> staticResourceIndexArrayMap;
	std::unordered_map<EVulkanCommandType::TYPE, IndexArray> transientResourceIndexArrayMap;
	size_t staticCountArray[EVulkanCommandType::MAX] = { 0 };
	size_t transientCountArray[EVulkanCommandType::MAX] = { 0 };

	for (size_t i = 0; i < m_ResourceArray.size(); ++i)
	{
		auto& outputData = m_ResourceArray[i];

		if (outputData.m_ExecutionPtrArray.size() == 0)
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
	size_t aggregateIndex = 0;
	m_QueueSubmitNodeArray.push_back(VulkanQueueSubmitNode());
	m_QueueSubmitNodeArray[aggregateIndex].Initialize();

	for (auto outputData : sortedOutputDataArray)
	{
		auto& node = m_QueueSubmitNodeArray[aggregateIndex];

		if (node.AggregateCommandBuffer(outputData.m_CommandType, outputData.m_RecordedCommandBuffer, outputData.m_RecordedGfxObjectUsage))
		{
			continue;
		}

		m_QueueSubmitNodeArray.push_back(VulkanQueueSubmitNode());
		m_QueueSubmitNodeArray[aggregateIndex].Initialize();
		++aggregateIndex;
		node = m_QueueSubmitNodeArray[aggregateIndex];

		if (node.AggregateCommandBuffer(outputData.m_CommandType, outputData.m_RecordedCommandBuffer, outputData.m_RecordedGfxObjectUsage) == false)
		{
			printf_console("[VulkanGraphics] failed to aggregate command buffers\n");

			throw;
		}
	}

	// Submit here??? or seperately in a different function????
	for (size_t i = 0; i < m_QueueSubmitNodeArray.size(); ++i)
	{
		auto& node = m_QueueSubmitNodeArray[i];

		for (auto& pair : node.m_AggregatedCommandBufferArrayMap)
		{
			auto& commandBufferArray = pair.second;
			auto& submitInfo = node.m_ResultSubmitInfoMap[pair.first];
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.pNext = NULL;

			if (i > 0)
			{
				auto& prevNode = m_QueueSubmitNodeArray[i - 1];
				submitInfo.waitSemaphoreCount = 1;
				submitInfo.pWaitSemaphores = &prevNode.GetSemaphore();
			}

			submitInfo.pWaitDstStageMask = NULL;
			submitInfo.commandBufferCount = commandBufferArray.size();
			submitInfo.pCommandBuffers = commandBufferArray.data();
			submitInfo.signalSemaphoreCount = 1;
			submitInfo.pSignalSemaphores = &node.GetSemaphore();
		}
	}

	// Remove all transient requests
	for (size_t i = m_ResourceArray.size(); i > 0; --i)
	{
		auto& outputData = m_ResourceArray[i - 1];

		if (!outputData.m_IsTransient)
		{
			continue;
		}

		DestroyResourceByDirectIndex(i);
	}
}

void VulkanGraphicsResourceCommandBufferManager::SubmitAll(const std::vector<VkSemaphore>& additionalWaitSemaphoreArray)
{
	// Submit here??? or seperately in a different function????
	for (size_t i = 0; i < m_QueueSubmitNodeArray.size(); ++i)
	{
		auto& node = m_QueueSubmitNodeArray[i];

		for (auto pair : node.m_ResultSubmitInfoMap)
		{
			VkSubmitInfo submitInfo = pair.second;
			std::vector<VkSemaphore> waitSemaphoreArray;
			std::vector<VkPipelineStageFlags> waitDstStageMaskArray;

			if (submitInfo.waitSemaphoreCount > 0 && submitInfo.pWaitSemaphores != NULL)
			{
				waitSemaphoreArray.insert(waitSemaphoreArray.end(), submitInfo.pWaitSemaphores, submitInfo.pWaitSemaphores + submitInfo.waitSemaphoreCount);

				for (size_t j = 0; j < submitInfo.waitSemaphoreCount; ++j)
				{
					waitDstStageMaskArray.push_back(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
				}
			}
			
			if (additionalWaitSemaphoreArray.size() > 0)
			{
				waitSemaphoreArray.insert(waitSemaphoreArray.end(), additionalWaitSemaphoreArray.begin(), additionalWaitSemaphoreArray.end());

				for (size_t j = 0; j < additionalWaitSemaphoreArray.size(); ++j)
				{
					waitDstStageMaskArray.push_back(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
				}
			}

			submitInfo.waitSemaphoreCount = waitSemaphoreArray.size();
			submitInfo.pWaitSemaphores = waitSemaphoreArray.data();
			submitInfo.pWaitDstStageMask = waitDstStageMaskArray.data();
			vkQueueSubmit(pair.first, 1, &submitInfo, VK_NULL_HANDLE);
		}
	}
}

VkSemaphore VulkanGraphicsResourceCommandBufferManager::GetSemaphoreForSubmission()
{
	if (m_QueueSubmitNodeArray.size() == 0)
	{
		return VK_NULL_HANDLE;
	}

	auto& lastNode = m_QueueSubmitNodeArray[m_QueueSubmitNodeArray.size() - 1];

	return VulkanGraphicsResourceSemaphoreManager::GetInstance().GetResource(lastNode.m_SignalSemaphoreIdentifier);
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

void VulkanGraphicsResourceCommandBufferManager::ReserveIndexForCommandBuffer(EVulkanCommandType::TYPE commandType, int index, std::unordered_map<EVulkanCommandType::TYPE, IndexArray>& resourceIndexArrayMap, size_t countArray[EVulkanCommandType::MAX])
{
	resourceIndexArrayMap[commandType].push_back(index);
	++countArray[commandType];
}

void VulkanGraphicsResourceCommandBufferManager::AllocateAndRecordCommandBuffer(std::unordered_map<EVulkanCommandType::TYPE, IndexArray>& resourceIndexArrayMap, const size_t countArray[EVulkanCommandType::MAX])
{
	for (size_t iType = 0; iType < EVulkanCommandType::MAX; ++iType)
	{
		if (countArray[(EVulkanCommandType::TYPE)iType] == 0)
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

			for (auto executionPtr : outputData.m_ExecutionPtrArray)
			{
				executionPtr->Execute(outputData.m_RecordedCommandBuffer, outputData.m_RecordedGfxObjectUsage);
			}

			vkEndCommandBuffer(outputData.m_RecordedCommandBuffer);
		}
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