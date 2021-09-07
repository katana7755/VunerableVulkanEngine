#include "VulnerableLayer.h"
#include "VulnerableUploadBufferManager.h"
#include "VulkanGraphicsResourceShaderManager.h"

#define COMMAND_QUEUE_COUNT 2

CommandPoolType g_CommandPool;
std::queue<VulnerableCommand::HeaderCommand*> g_HeaderCommandQueues[COMMAND_QUEUE_COUNT];
std::queue<VulnerableCommand::BodyCommand*> g_BodyCommandQueues[COMMAND_QUEUE_COUNT];
int g_CurrentCommandQueueIndex = 0;

namespace VulnerableCommand
{
	void CreateShader::Execute()
	{
		auto uploadBuffer = VulnerableUploadBufferManager::GetUploadBuffer(m_UploadBufferID);
		VulkanGraphicsResourceShaderManager::GetInstance().CreateResourcePhysically(m_Identifier, uploadBuffer.m_Data, uploadBuffer.m_Size);
	}

	void DestroyShader::Execute()
	{
		VulkanGraphicsResourceShaderManager::GetInstance().DestroyResourcePhysicially(m_Identifier);
		VulkanGraphicsResourceShaderManager::GetInstance().ReleaseResource(m_Identifier);
	}

	void CreateGraphicsPipeline::Execute()
	{
	}

	void DestroyGraphicsPipeline::Execute()
	{
	}

	void BeginCommandBuffer::Execute()
	{
	}

	void CmdBindGraphicsPipeline::Execute()
	{
	}

	void EndCommandBuffer::Execute()
	{
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
}

void VulnerableLayer::ExecuteAllCommands()
{
	// TODO: need to consider multex for supporting multi threading?
	auto headerCommandQueue = GetCurrentHeaderCommandQueue();
	auto bodyCommandQueue = GetCurrentBodyCommandQueue();
	IncrementCurrentCommandQueueIndex();

	while (!headerCommandQueue.empty())
	{
		auto commandPtr = headerCommandQueue.front();
		headerCommandQueue.pop();
		commandPtr->Execute();

		std::type_index typeIndex = typeid(commandPtr);
		g_CommandPool.insert(CommandPoolType::value_type(typeIndex, commandPtr));
	}

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