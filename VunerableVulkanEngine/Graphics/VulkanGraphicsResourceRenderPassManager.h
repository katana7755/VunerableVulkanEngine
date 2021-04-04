#pragma once
#include "VulkanGraphicsResourceBase.h"
#include <vector>

class VulkanGraphicsResourceRenderPassManager : public VulkanGraphicsResourceBase
{
public:
	static int CreateRenderPass(const std::vector<VkAttachmentDescription>& attachmentDescArray, const std::vector<VkSubpassDescription>& subpassDescArray, const std::vector<VkSubpassDependency>& subpassDepArray);
	static const VkRenderPass& GetRenderPass(int index);
	static void DestroyRenderPass(int index);
	static int CreateFramebuffer(int renderPassIndex, std::vector<VkImageView> attachmentArray, uint32_t width, uint32_t height, uint32_t layers);
	static const VkFramebuffer& GetFramebuffer(int index);
	static void DestroyFramebuffer(int index);

private:
	static std::vector<VkRenderPass> s_RenderPassArray;
	static std::vector<VkFramebuffer> s_FramebufferArray;

protected:
	virtual bool CreateInternal() override;
	virtual bool DestroyInternal() override;
};