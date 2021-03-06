#pragma once

#include "VulkanGraphicsResourceGraphicsPipelineManager.h"
#include "VulkanGraphicsResourceShaderManager.h"
#include "VulkanGraphicsResourceCommandBufferManager.h"
#include <typeindex>
#include <unordered_map>
#include <queue>
#include <functional>

namespace VulnerableEnum
{
	enum ShaderStage
	{
		Vertex,
		Fragment,
	};

	enum PipelineType
	{
		Graphics,
		Transfer,
		Compute,
	};
}

namespace VulnerableLayer
{
	void Initialize();
	void Deinitialize();
	void ExecuteAllCommands();
}

namespace VulnerableCommand
{
	struct CommandBase
	{
		friend void VulnerableLayer::ExecuteAllCommands();

	private:
		virtual void Execute() {};
	};

	struct HeaderCommand : CommandBase
	{
	};

	struct BodyCommand : CommandBase
	{
	};

	struct ClearAllTemporaryResources : public HeaderCommand
	{
	private:
		void Execute() override;
	};

	struct CreateShader : public HeaderCommand
	{
		size_t m_Identifier;
		int m_UploadBufferID;
		VulkanShaderMetaData m_MetaData;

	private:
		void Execute() override;
	};

	struct DestroyShader : public HeaderCommand
	{
		size_t m_Identifier;

	private:
		void Execute() override;
	};

	struct CreateGraphicsPipeline : public HeaderCommand
	{
		size_t m_Identifier;
		VulkanGraphicsPipelineInputData m_InputData;

	private:
		void Execute() override;
	};

	struct DestroyGraphicsPipeline : public HeaderCommand
	{
		size_t m_Identifier;

	private:
		void Execute() override;
	};

	struct CreateCommandBuffer : public BodyCommand
	{
		size_t							m_Identifier;
		VulkanCommandBufferInputData	m_InputData;

	private:
		void Execute() override;
	};

	struct RecordCommandBuffer : public BodyCommand
	{
		size_t											m_Identifier;
		std::vector<VulkanGfxExecution::ExecutionBase*>	m_ExecutionPtrArray;

	private:
		void Execute() override;
	};

	struct DestroyCommandBuffer : public BodyCommand
	{
		size_t m_Identifier;

	private:
		void Execute() override;
	};

	// TODO: this needs to be reimplemted too...especially for how we handle additional wait semaphores...
	struct SubmitAllCommandBuffers : public BodyCommand
	{
		std::vector<VkSemaphore> m_WaitSemaphoreArray;

	private:
		void Execute() override;
	};
};

typedef std::unordered_multimap<std::type_index, VulnerableCommand::CommandBase*> CommandPoolType;

namespace VulnerableLayer
{
	CommandPoolType& GetCommandPool();
	std::queue<VulnerableCommand::HeaderCommand*>& GetCurrentHeaderCommandQueue();
	std::queue<VulnerableCommand::BodyCommand*>& GetCurrentBodyCommandQueue();
	void IncrementCurrentCommandQueueIndex();

	template <class TCommand>
	TCommand* AllocateCommand();

	template <class TCommand>
	void AllocateCommandWithSetter(std::function<void(TCommand* commandPtr)> funcSetter);
};

template <class TCommand>
TCommand* VulnerableLayer::AllocateCommand()
{
	// TODO: need to consider multex for supporting multi threading?
	static_assert(std::is_base_of<VulnerableCommand::CommandBase, TCommand>::value, "this function should be called with a class derived by VulnerableCommand::Base.");

	TCommand* commandPtr = NULL;
	std::type_index typeIndex = typeid(commandPtr);

	auto range = GetCommandPool().equal_range(typeIndex);

	if (range.first == range.second)
	{
		GetCommandPool().insert(CommandPoolType::value_type(typeIndex, new TCommand()));
	}

	range = GetCommandPool().equal_range(typeIndex);

	auto iter = range.first;
	commandPtr = (TCommand*)iter->second;
	GetCommandPool().erase(iter);

	if (std::is_base_of<VulnerableCommand::HeaderCommand, TCommand>::value)
	{
		// TODO: need to consider multex for supporting multi threading?
		GetCurrentHeaderCommandQueue().push((VulnerableCommand::HeaderCommand*)commandPtr);
	}
	else
	{
		// TODO: need to consider multex for supporting multi threading?
		GetCurrentBodyCommandQueue().push((VulnerableCommand::BodyCommand*)commandPtr);
	}

	return commandPtr;
}

template <class TCommand>
static void VulnerableLayer::AllocateCommandWithSetter(std::function<void (TCommand* commandPtr)> funcSetter)
{
	auto* commandPtr = AllocateCommand<TCommand>();

	if (funcSetter != NULL)
	{
		funcSetter(commandPtr);
	}	
}