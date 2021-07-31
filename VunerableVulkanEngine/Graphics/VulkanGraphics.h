#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include "VulkanGraphicsResourceInstance.h"
#include "VulkanGraphicsResourceSurface.h"
#include "VulkanGraphicsResourceDevice.h"
#include "VulkanGraphicsResourceSwapchain.h"
#include "VulkanGraphicsResourceCommandBufferManager.h"
#include "VulkanGraphicsResourceRenderPassManager.h"
#include "VulkanGraphicsResourcePipelineManager.h"
#include "VulkanGraphicsObjectMesh.h"
#include "VulkanGraphicsObjectTexture.h"
#include "VulkanGraphicsObjectUniformBuffer.h"
#include "VulkanGraphicsObjectSampler.h"

class VulkanGraphics
{
public:
	VulkanGraphics();
	~VulkanGraphics();

#ifdef _WIN32
	void Initialize(HINSTANCE hInstance, HWND hWnd);
#endif

	void Invalidate();
	void TransferAllStagingBuffers();
	void DrawFrame();

private:
	void BuildRenderLoop();

	VulkanGraphicsResourceInstance m_ResourceInstance;
	VulkanGraphicsResourceSurface m_ResourceSurface;
	VulkanGraphicsResourceDevice m_ResourceDevice;
	VulkanGraphicsResourceSwapchain m_ResourceSwapchain;
	VulkanGraphicsResourceCommandBufferManager m_ResourceCommandBufferMgr;
	VulkanGraphicsResourceRenderPassManager m_ResourceRenderPassMgr;
	VulkanGraphicsResourcePipelineManager m_ResourcePipelineMgr;

	VulkanGraphicsObjectMesh m_CharacterMesh;
	VulkanGraphicsObjectTexture m_CharacterBodyTexture;
	VulkanGraphicsObjectSampler m_CharacterBodySampler;
	VulkanGraphicsObjectTexture m_DepthTexture;
	VulkanGraphicsObjectUniformBuffer m_MVPMatrixUniformBuffer;

	int m_AcquireNextImageSemaphoreIndex;
	int m_QueueSubmitFenceIndex;
	int m_QueueSubmitSemaphoreIndex;

private:
	// TODO: Need to convert this into another object...
	void CreateDescriptorSet();
	void DestroyDescriptorSet();
	void CreateShaderModule();
	void DestroyShaderModule();

	VkDescriptorSetLayout m_DescriptorSetLayout;

	VkDescriptorPool m_DescriptorPool;
	VkDescriptorSet m_DescriptorSet;
};

