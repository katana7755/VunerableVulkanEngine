#pragma once

#include <typeindex>
#include <unordered_map>
#include <queue>

class VulnerableLayer;

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

namespace VulnerableCommand
{
	struct CommandBase
	{
		friend class VulnerableLayer;

	private:
		virtual void Execute() = 0;
	};

	struct HeaderCommand : CommandBase
	{
	};

	struct BodyCommand : CommandBase
	{
	};

	struct CreateShader : public HeaderCommand
	{
		size_t m_Identifier;
		int m_UploadBufferID;

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

	private:
		void Execute() override;
	};

	struct DestroyGraphicsPipeline : public HeaderCommand
	{
		size_t m_Identifier;

	private:
		void Execute() override;
	};

	struct BeginCommandBuffer : public BodyCommand
	{
	private:
		void Execute() override;
	};

	struct CmdBindGraphicsPipeline : public BodyCommand
	{
		size_t m_Identifier;

	private:
		void Execute() override;
	};

	struct EndCommandBuffer : public BodyCommand
	{
	private:
		void Execute() override;
	};
};

typedef std::unordered_multimap<std::type_index, VulnerableCommand::CommandBase*> CommandPoolType;

class VulnerableLayer
{
private:
	VulnerableLayer() {};

public:
	static CommandPoolType& GetCommandPool();
	static std::queue<VulnerableCommand::HeaderCommand*>& GetCurrentHeaderCommandQueue();
	static std::queue<VulnerableCommand::BodyCommand*>& GetCurrentBodyCommandQueue();
	static void IncrementCurrentCommandQueueIndex();

	static void Initialize();
	static void Deinitialize();
	static void ExecuteAllCommands();

	template <class TCommand>
	static TCommand* AllocateCommand();
};

template <class TCommand>
TCommand* VulnerableLayer::AllocateCommand()
{
	// TODO: need to consider multex for supporting multi threading?
	static_assert(std::is_base_of<VulnerableCommand::CommandBase, TCommand>::value, "this function should be called with a class derived by VulnerableCommand::Base.");

	TCommand* commandPtr = NULL;
	std::type_index typeIndex = typeid(commandPtr);

	if (GetCommandPool().find(typeIndex) == GetCommandPool().end())
	{
		GetCommandPool().insert(CommandPoolType::value_type(typeIndex, new TCommand()));
	}

	auto iter = GetCommandPool().find(typeIndex);
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