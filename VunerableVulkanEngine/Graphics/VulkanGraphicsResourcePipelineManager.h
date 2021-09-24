#pragma once
#include "VulkanGraphicsResourceBase.h"
#include <vector>

class VulkanGraphicsResourcePipelineManager : public VulkanGraphicsResourceBase
{
public:
	static const VkPipelineCache& GetPipelineCache();
	static const VkDescriptorPool& GetDescriptorPool(int index);
	static void DestroyShaderModule(const size_t identifier);

public:
	// TODO: also we need to take care of actual descriptor sets...(NECESSARY!!!)

	static int CreateGfxFence();
	static const VkFence& GetGfxFence(int index);
	static void DestroyGfxFence(int index);
	static int CreateGfxSemaphore();
	static const VkSemaphore& GetGfxSemaphore(int index);
	static void DestroyGfxSemaphore(int index);
	static int CreateDescriptorSetLayout();
	static void DestroyDescriptorSetLayout(int index);
	static int CreatePushConstantRange(VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size);
	static void DestroyPushConstantRange(int index);
	static int CreatePipelineLayout(std::vector<int> desciptorSetLayoutIndexArray, std::vector<int> pushConstantRangeIndexArray);
	static const VkPipelineLayout& GetPipelineLayout(int index);
	static void DestroyPipelineLayout(int index);
	static void BeginToCreateGraphicsPipeline();
	static int CreateGraphicsPipeline(const size_t vertexShaderIdentifier, const size_t fragmentShaderIdentifier, int pipelineLayoutIndex, int renderPassIndex, int subPassIndex);
	//static int CreateGraphicsPipeline(int vertexShaderModuleIndex, int fragmentShaderModuleIndex, int pipelineLayoutIndex, int renderPassIndex, int subPassIndex);
	static void EndToCreateGraphicsPipeline();
	static const VkPipeline& GetGraphicsPipeline(int index);
	static void DestroyGraphicsPipeline(int index);
	static int CreateDescriptorPool(const std::vector<VkDescriptorPoolSize>& poolSizeArray);
	static void DestroyDescriptorPool(int index);
	static int AllocateDescriptorSet(int poolIndex, const VkDescriptorSetLayout& descriptorSetLayout);
	static void UpdateDescriptorSet(int index, int binding, const VkImageView& imageView, const VkSampler& sampler);
	static void ReleaseDescriptorSet(int index);
	static const std::vector<VkDescriptorSet>& GetDescriptorSetArray();

protected:
	virtual bool CreateInternal() override;
	virtual bool DestroyInternal() override;
};

