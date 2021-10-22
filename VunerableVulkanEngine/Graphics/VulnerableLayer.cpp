#include "VulnerableLayer.h"
#include "VulnerableUploadBufferManager.h"

#define COMMAND_QUEUE_COUNT 2

CommandPoolType g_CommandPool;
std::queue<VulnerableCommand::HeaderCommand*> g_HeaderCommandQueues[COMMAND_QUEUE_COUNT];
std::queue<VulnerableCommand::BodyCommand*> g_BodyCommandQueues[COMMAND_QUEUE_COUNT];
int g_CurrentCommandQueueIndex = 0;

namespace VulnerableCommand
{
	void CreateShader::Execute()
	{
		VulkanGraphicsResourceShaderManager::GetInstance().CreateResource(m_Identifier, m_UploadBufferID, m_MetaData);
	}

	void DestroyShader::Execute()
	{
		VulkanGraphicsResourceShaderManager::GetInstance().DestroyResource(m_Identifier);
		VulkanGraphicsResourceShaderManager::GetInstance().ReleaseIdentifier(m_Identifier);
	}

	void CreateGraphicsPipeline::Execute()
	{
		VulkanGraphicsResourceGraphicsPipelineManager::GetInstance().CreateResource(m_Identifier, m_InputData, m_InputData);
	}

	void DestroyGraphicsPipeline::Execute()
	{
		VulkanGraphicsResourceGraphicsPipelineManager::GetInstance().DestroyResource(m_Identifier);
		VulkanGraphicsResourceGraphicsPipelineManager::GetInstance().ReleaseIdentifier(m_Identifier);
	}

	void CreateCommandBuffer::Execute()
	{
		VulkanGraphicsResourceCommandBufferManager::GetInstance().CreateResource(m_Identifier, m_InputData, m_Identifier);
	}

	void RecordCommandBuffer::Execute()
	{
		if (m_ExecutionPtrArray.size() == 0)
		{
			return;
		}

		auto& outputData = VulkanGraphicsResourceCommandBufferManager::GetInstance().GetResource(m_Identifier);
		outputData.m_ExecutionPtrArray.insert(outputData.m_ExecutionPtrArray.end(), m_ExecutionPtrArray.begin(), m_ExecutionPtrArray.end());
	}

	void DestroyCommandBuffer::Execute()
	{
		VulkanGraphicsResourceCommandBufferManager::GetInstance().DestroyResource(m_Identifier);
	}

	void SubmitAllCommandBuffers::Execute()
	{
		VulkanGraphicsResourceCommandBufferManager::GetInstance().BuildAll();
		VulkanGraphicsResourceCommandBufferManager::GetInstance().SubmitAll(m_WaitSemaphoreArray);
	}
};

CommandPoolType& VulnerableLayer::GetCommandPool()
{
	return g_CommandPool;
}

std::queue<VulnerableCommand::HeaderCommand*>& VulnerableLayer::GetCurrentHeaderCommandQueue()
{
	return g_HeaderCommandQueues[g_CurrentCommandQueueIndex];
}

std::queue<VulnerableCommand::BodyCommand*>& VulnerableLayer::GetCurrentBodyCommandQueue()
{
	return g_BodyCommandQueues[g_CurrentCommandQueueIndex];
}

void VulnerableLayer::IncrementCurrentCommandQueueIndex()
{
	g_CurrentCommandQueueIndex = (g_CurrentCommandQueueIndex + 1) % COMMAND_QUEUE_COUNT;
}

void VulnerableLayer::Initialize()
{
	VulkanGraphicsResourceCommandBufferManager::GetInstance().Initialize();

	// TODO: we might need to consider to reserve pool size?
}

void VulnerableLayer::Deinitialize()
{
	auto headerCommandQueue = GetCurrentHeaderCommandQueue();
	auto bodyCommandQueue = GetCurrentBodyCommandQueue();

	while (!headerCommandQueue.empty())
	{
		delete headerCommandQueue.front();
		headerCommandQueue.pop();
	}

	while (!bodyCommandQueue.empty())
	{
		delete bodyCommandQueue.front();
		bodyCommandQueue.pop();
	}

	auto iter = g_CommandPool.begin();
	auto endIter = g_CommandPool.end();

	while (iter != endIter)
	{
		delete iter->second;
		++iter;
	}

	g_CommandPool.clear();

	VulkanGraphicsResourceCommandBufferManager::GetInstance().Deinitialize();
}

void VulnerableLayer::ExecuteAllCommands()
{
	// TODO: need to consider multex for supporting multi threading?
	auto& headerCommandQueue = GetCurrentHeaderCommandQueue();
	auto& bodyCommandQueue = GetCurrentBodyCommandQueue();
	IncrementCurrentCommandQueueIndex();

	while (!headerCommandQueue.empty())
	{
		auto commandPtr = headerCommandQueue.front();
		headerCommandQueue.pop();
		commandPtr->Execute();

		std::type_index typeIndex = typeid(commandPtr);
		g_CommandPool.insert(CommandPoolType::value_type(typeIndex, commandPtr));
	}

	// TODO: when we consider multithreaded command buffer recording, dequeue a group from begin to end of command buffer
	while (!bodyCommandQueue.empty())
	{
		auto commandPtr = bodyCommandQueue.front();
		bodyCommandQueue.pop();
		commandPtr->Execute();

		std::type_index typeIndex = typeid(commandPtr);
		g_CommandPool.insert(CommandPoolType::value_type(typeIndex, commandPtr));
	}

	VulnerableUploadBufferManager::ClearAllUploadBuffer();
}