#pragma once

#include "VulkanGraphicsResourceBase.h"
#include "VulkanGraphicsResourceSemaphoreManager.h"
#include <vector>

typedef std::vector<VkCommandBuffer> VulkanCommandBufferArray;
typedef std::vector<size_t> IndexArray;
typedef std::vector<VkSubmitInfo> SubmitInfoArray;

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
	std::vector<VkImageView>	m_ReadTextureArray;		// TODO: this can be unified after applying identifier
	std::vector<VkImageView>	m_WriteTextureArray;	// TODO: this can be unified after applying identifier
	std::vector<VkBufferView>	m_ReadBufferArray;		// TODO: this can be unified after applying identifier
	std::vector<VkBufferView>	m_WriteBufferArray;		// TODO: this can be unified after applying identifier

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
	VkSemaphore												m_SignalSemaphore;

	VulkanQueueSubmitNode();

	bool Aggregate(EVulkanCommandType::TYPE commandType, const VkCommandBuffer& commandBuffer, const VulkanGfxObjectUsage& gfxObjectUsage);
};

struct VulkanGfxExecutionBase
{
	friend class VulkanGraphicsResourceCommandBufferManager;

private:
	virtual void Execute(const VkCommandBuffer& commandBuffer, VulkanGfxObjectUsage& gfxObjectUsage) {};
};

struct VulkanCommandBufferInputData
{
	EVulkanCommandType::TYPE	m_CommandType;
	bool						m_IsTransient;
	int							m_SortOrder;
};

struct VulkanCommandBufferOutputData
{
	EVulkanCommandType::TYPE			m_CommandType;
	bool								m_IsTransient;
	int									m_SortOrder;
	std::vector<VulkanGfxExecutionBase>	m_ExecutionArray;
	VkCommandBuffer						m_RecordedCommandBuffer;
	VulkanGfxObjectUsage				m_RecordedGfxObjectUsage;
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
	void FreeAllSemaphores();
	void BuildRenderGraph();
	void SubmitAll();

private:
	void CreateSingleCommandPool(EVulkanCommandType::TYPE commandType, bool isTransient);
	void DestroySingleCommandPool(EVulkanCommandType::TYPE commandType, bool isTransient);
	void ReserveIndexForCommandBuffer(EVulkanCommandType::TYPE commandType, int index, std::unordered_map<EVulkanCommandType::TYPE, IndexArray>& resourceIndexArrayMap, size_t countArray[EVulkanCommandType::MAX]);
	void AllocateAndRecordCommandBuffer(std::unordered_map<EVulkanCommandType::TYPE, IndexArray>& resourceIndexArrayMap, const size_t countArray[EVulkanCommandType::MAX]);

private:
	std::unordered_map<EVulkanCommandType::TYPE, VkCommandPool>	m_StaticCommandPoolMap;
	std::unordered_map<EVulkanCommandType::TYPE, VkCommandPool>	m_TransientCommandPoolMap;
	std::vector<size_t>											m_SemaphoreIdentifierArray;
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