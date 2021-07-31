#pragma once
#include "VulkanGraphicsResourceBase.h"
#include <vector>

class VulkanGraphicsResourcePipelineManager : public VulkanGraphicsResourceBase
{
public:
	// TODO: also we need to take care of actual descriptor sets...(NECESSARY!!!)

	static int CreateGfxFence();
	static const VkFence& GetGfxFence(int index);
	static void DestroyGfxFence(int index);
	static int CreateGfxSemaphore();
	static const VkSemaphore& GetGfxSemaphore(int index);
	static void DestroyGfxSemaphore(int index);
	static int CreateShaderModule(const char* strFileName);
	static void DestroyShaderModule(int index);
	static int CreateDescriptorSetLayout();
	static void DestroyDescriptorSetLayout(int index);
	static int CreatePushConstantRange(VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size);
	static void DestroyPushConstantRange(int index);
	static int CreatePipelineLayout(std::vector<int> desciptorSetLayoutIndexArray, std::vector<int> pushConstantRangeIndexArray);
	static const VkPipelineLayout& GetPipelineLayout(int index);
	static void DestroyPipelineLayout(int index);
	static void BeginToCreateGraphicsPipeline();
	static int CreateGraphicsPipeline(int vertexShaderModuleIndex, int fragmentShaderModuleIndex, int pipelineLayoutIndex, int renderPassIndex, int subPassIndex);
	static void EndToCreateGraphicsPipeline();
	static const VkPipeline& GetGraphicsPipeline(int index);
	static void DestroyGraphicsPipeline(int index);
	static int CreateDescriptorPool();
	static void DestroyDescriptorPool(int index);
	static int AllocateDescriptorSet(int poolIndex, int layoutIndex);
	static void UpdateDescriptorSet(int index, const VkImageView& imageView, const VkSampler& sampler);
	static void ReleaseDescriptorSet(int index);
	static const std::vector<VkDescriptorSet>& GetDescriptorSetArray();

private:
	static VkPipelineCache s_PipelineCache;
	static std::vector<VkFence> s_GfxFenceArray;
	static std::vector<VkSemaphore> s_GfxSemaphoreArray;
	static std::vector<VkShaderModule> s_ShaderModuleArray;
	static std::vector<VkDescriptorSetLayout> s_DescriptorSetLayoutArray;
	static std::vector<VkPushConstantRange> s_PushConstantRangeArray;
	static std::vector<VkPipelineLayout> s_PipelineLayoutArray;
	static std::vector<VkGraphicsPipelineCreateInfo> s_GraphicsPipelineCreateInfoArray;
	static std::vector<VkPipeline> s_GraphicsPipelineArray;
	static std::vector<VkDescriptorPool> s_DescriptorPoolArray;
	static std::vector<int> s_DescriptorPoolIndexArray;
	static std::vector<VkDescriptorSet> s_DescriptorSetArray;

protected:
	virtual bool CreateInternal() override;
	virtual bool DestroyInternal() override;
};

