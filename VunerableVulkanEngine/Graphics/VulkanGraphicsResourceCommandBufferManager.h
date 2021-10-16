#pragma once

#include "VulkanGraphicsResourceBase.h"
#include "VulkanGraphicsResourceSemaphoreManager.h"
#include "VulkanGraphicsResourceShaderManager.h"
#include <typeindex>
#include <vector>
#include <tuple>

typedef std::vector<VkCommandBuffer> VulkanCommandBufferArray;
typedef std::vector<size_t> IndexArray;
typedef std::vector<VkSubmitInfo> SubmitInfoArray;
typedef std::vector<uint8_t> VulkanPushContstansRawData;

namespace EVulkanCommandType
{
	enum TYPE
	{
		NONE = -1,

		GRAPHICS,
		COMPUTE, 
		TRANSFER,

		MAX,
	};
};

class VulkanGraphicsResourceCommandBufferManager;

struct VulkanGfxObjectUsage
{
	std::vector<VkImage>	m_ReadTextureArray;		// TODO: this can be unified after applying identifier
	std::vector<VkImage>	m_WriteTextureArray;	// TODO: this can be unified after applying identifier
	std::vector<VkBuffer>	m_ReadBufferArray;		// TODO: this can be unified after applying identifier
	std::vector<VkBuffer>	m_WriteBufferArray;		// TODO: this can be unified after applying identifier

	VulkanGfxObjectUsage()
	{
	}

	VulkanGfxObjectUsage(const VulkanGfxObjectUsage& input)
		: m_ReadTextureArray(input.m_ReadTextureArray)
		, m_WriteTextureArray(input.m_WriteTextureArray)
		, m_ReadBufferArray(input.m_ReadBufferArray)
		, m_WriteBufferArray(input.m_WriteBufferArray)
	{
	}

	void Aggregate(const VulkanGfxObjectUsage& input)
	{
		m_ReadTextureArray.insert(m_ReadTextureArray.end(), input.m_ReadTextureArray.begin(), input.m_ReadTextureArray.end());
		m_WriteTextureArray.insert(m_WriteTextureArray.end(), input.m_WriteTextureArray.begin(), input.m_WriteTextureArray.end());
		m_ReadBufferArray.insert(m_ReadBufferArray.end(), input.m_ReadBufferArray.begin(), input.m_ReadBufferArray.end());
		m_WriteBufferArray.insert(m_WriteBufferArray.end(), input.m_WriteBufferArray.begin(), input.m_WriteBufferArray.end());
	}
};

struct VulkanQueueSubmitNode
{
	std::unordered_map<VkQueue, VulkanCommandBufferArray>	m_AggregatedCommandBufferArrayMap;
	VulkanGfxObjectUsage									m_AggregatedGfxObjectUsage;
	size_t													m_SignalSemaphoreIdentifier;
	std::unordered_map<VkQueue, VkSubmitInfo>				m_ResultSubmitInfoMap;

	void Initialize();
	void Deinitialize();
	bool AggregateCommandBuffer(EVulkanCommandType::TYPE commandType, const VkCommandBuffer& commandBuffer, const VulkanGfxObjectUsage& gfxObjectUsage);
	const VkSemaphore& GetSemaphore();
};

namespace VulkanGfxExecution
{
	struct ExecutionBase
	{
		friend class VulkanGraphicsResourceCommandBufferManager;

	private:
		virtual void Execute(const VkCommandBuffer& commandBuffer, VulkanGfxObjectUsage& gfxObjectUsage) {};
	};

	// TODO: this should be changed after implementing the render pass manager, frame buffer manager???
	struct BeginRenderPassExecution : ExecutionBase
	{
		int			m_RenderPassIndex;
		int			m_FramebufferIndex;
		VkImage		m_FrameBufferColor;
		VkImage		m_FrameBufferDepth;
		VkRect2D	m_RenderArea;

	private:
		void Execute(const VkCommandBuffer& commandBuffer, VulkanGfxObjectUsage& gfxObjectUsage) override;
	};

	struct BindPipelineExecution : ExecutionBase
	{
		size_t m_PipelineIdentifier;

	private:
		void Execute(const VkCommandBuffer& commandBuffer, VulkanGfxObjectUsage& gfxObjectUsage) override;
	};

	struct PushConstantsExecution : ExecutionBase
	{
		size_t									m_PipelineIdentifier;
		std::vector<VulkanPushContstansRawData>	m_RawDataArrays[EVulkanShaderType::MAX];

	private:
		void Execute(const VkCommandBuffer& commandBuffer, VulkanGfxObjectUsage& gfxObjectUsage) override;
	};

	// TODO: this will be reimplemented with new descriptor set manager
	struct BindDescriptorSetsExecution : ExecutionBase
	{
		size_t							m_PipelineIdentifier;
		std::vector<VkDescriptorSet>	m_DescriptorSetArray;
		VulkanGfxObjectUsage			m_GfxObjectUsage;

	private:
		void Execute(const VkCommandBuffer& commandBuffer, VulkanGfxObjectUsage& gfxObjectUsage) override;
	};

	// TODO: this will be reimplemented with new gfx object manager?
	struct DrawIndexedExectuion : ExecutionBase
	{
		size_t					m_PipelineIdentifier;
		VkBuffer				m_VertexBuffer;
		VkBuffer				m_IndexBuffer;
		uint32_t				m_InstanceCount;
		uint32_t				m_IndexCount;
		VulkanGfxObjectUsage	m_GfxObjectUsage;

	private:
		void Execute(const VkCommandBuffer& commandBuffer, VulkanGfxObjectUsage& gfxObjectUsage) override;
	};

	// TODO: currently only handling color images, but in the future other image type will be supported
	// TODO: it is necessary to implement abstract image class which contains every status values such as a layout and a access mask
	struct CopyImageExecution : ExecutionBase
	{
		VkImage			m_SourceImage;
		VkImageLayout	m_SourceLayoutFrom;
		VkImageLayout	m_SourceLayoutTo;
		VkAccessFlags	m_SourceAccessMaskFrom;
		VkAccessFlags	m_SourceAccessMaskTo;
		VkImage			m_DestinationImage;
		VkImageLayout	m_DestinationLayoutFrom;
		VkImageLayout	m_DestinationLayoutTo;
		VkAccessFlags	m_DestinationAccessMaskFrom;
		VkAccessFlags	m_DestinationAccessMaskTo;
		uint32_t		m_Width;
		uint32_t		m_Height;

	private:
		void Execute(const VkCommandBuffer& commandBuffer, VulkanGfxObjectUsage& gfxObjectUsage) override;
	};

	struct EndRenderPassExecution : ExecutionBase
	{
	private:
		void Execute(const VkCommandBuffer& commandBuffer, VulkanGfxObjectUsage& gfxObjectUsage) override;
	};

	typedef std::unordered_multimap<std::type_index, ExecutionBase*> ExecutionPoolType;

	ExecutionPoolType& GetExecutionPool();

	template <class TExecution>
	TExecution* AllocateExecution()
	{
		// TODO: need to consider multex for supporting multi threading?
		static_assert(std::is_base_of<ExecutionBase, TExecution>::value, "this function should be called with a class derived by VulkanGfxExecution::ExecutionBase.");

		TExecution* executionPtr = NULL;
		std::type_index typeIndex = typeid(executionPtr);

		auto range = GetExecutionPool().equal_range(typeIndex);

		if (range.first == range.second)
		{
			GetExecutionPool().insert(ExecutionPoolType::value_type(typeIndex, new TExecution()));
		}

		range = GetExecutionPool().equal_range(typeIndex);

		auto iter = range.first;
		executionPtr = (TExecution*)iter->second;
		GetExecutionPool().erase(iter);

		return executionPtr;
	}
}

struct VulkanCommandBufferInputData
{
	EVulkanCommandType::TYPE	m_CommandType;
	bool						m_IsTransient;
	int							m_SortOrder;
};

struct VulkanCommandBufferOutputData
{
	EVulkanCommandType::TYPE						m_CommandType;
	bool											m_IsTransient;
	int												m_SortOrder;
	std::vector<VulkanGfxExecution::ExecutionBase*>	m_ExecutionPtrArray;
	VkCommandBuffer									m_RecordedCommandBuffer;
	VulkanGfxObjectUsage							m_RecordedGfxObjectUsage;
};

class VulkanGraphicsResourceCommandBufferManager : public VulkanGraphicsResourceManagerBase<VulkanCommandBufferOutputData, VulkanCommandBufferInputData, size_t>
{
public:
	static VulkanGraphicsResourceCommandBufferManager& GetInstance();

public:
	void Initialize() override;
	void Deinitialize() override;

protected:
	VulkanCommandBufferOutputData CreateResourcePhysically(const VulkanCommandBufferInputData& inputData) override;
	void DestroyResourcePhysicially(const VulkanCommandBufferOutputData& outputData) override;

public:
	void FreeAllStaticCommandBuffers();
	void FreeAllQueueSubmitNodes();
	void BuildAll();
	void SubmitAll(const std::vector<VkSemaphore>& additionalWaitSemaphoreArray);
	VkSemaphore GetSemaphoreForSubmission();

private:
	void CreateSingleCommandPool(EVulkanCommandType::TYPE commandType, bool isTransient);
	void DestroySingleCommandPool(EVulkanCommandType::TYPE commandType, bool isTransient);
	void ReserveIndexForCommandBuffer(EVulkanCommandType::TYPE commandType, int index, std::unordered_map<EVulkanCommandType::TYPE, IndexArray>& resourceIndexArrayMap, size_t countArray[EVulkanCommandType::MAX]);
	void AllocateAndRecordCommandBuffer(std::unordered_map<EVulkanCommandType::TYPE, IndexArray>& resourceIndexArrayMap, const size_t countArray[EVulkanCommandType::MAX]);

private:
	std::unordered_map<EVulkanCommandType::TYPE, VkCommandPool>	m_StaticCommandPoolMap;
	std::unordered_map<EVulkanCommandType::TYPE, VkCommandPool>	m_TransientCommandPoolMap;
	std::vector<VulkanQueueSubmitNode>							m_QueueSubmitNodeArray;
};

class OldVulkanGraphicsResourceCommandBufferManager : public VulkanGraphicsResourceBase
{
public:
	static const VkCommandPool& GetCommandPool()
	{
		return s_CommandGraphicsPool;
	}

public:
	static void AllocatePrimaryBufferArray();
	static void FreePrimaryBufferArray();
	static const VkCommandBuffer& GetPrimaryCommandBuffer(int index);
	static const VkCommandBuffer& AllocateAdditionalCommandBuffer();
	static void FreeAdditionalCommandBuffer(const VkCommandBuffer& commandBuffer);
	static const std::vector<VkCommandBuffer>& GetAllAdditionalCommandBuffers();
	static void ClearAdditionalCommandBuffers();

private:
	static VkCommandPool s_CommandGraphicsPool;
	static std::vector<VkCommandBuffer> s_PrimaryBufferArray;
	static std::vector<VkCommandBuffer> s_AdditionalBufferArray;
	

protected:
	virtual bool CreateInternal() override;
	virtual bool DestroyInternal() override;

private:
};