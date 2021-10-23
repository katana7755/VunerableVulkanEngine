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
	void InitializeFrame();
	void SubmitPrimary();
	void PresentFrame();
	void BeginRenderPass(const VkCommandBuffer& commandBuffer, int renderPassIndex);
	void EndRenderPass(const VkCommandBuffer& commandBuffer);

private:
	void BuildRenderLoop();
	void TransferAllStagingBuffers();

#ifdef _WIN32
	void InitializeGUI(HWND hWnd);
#endif

	void DeinitializeGUI();
	void DrawGUI();

private:
	VulkanGraphicsResourceInstance m_ResourceInstance;
	VulkanGraphicsResourceSurface m_ResourceSurface;
	VulkanGraphicsResourceDevice m_ResourceDevice;
	VulkanGraphicsResourceSwapchain m_ResourceSwapchain;
	OldVulkanGraphicsResourceCommandBufferManager m_ResourceCommandBufferMgr;
	VulkanGraphicsResourceRenderPassManager m_ResourceRenderPassMgr;
	VulkanGraphicsResourcePipelineManager m_ResourcePipelineMgr;

	VulkanGraphicsObjectMesh m_CharacterMesh;
	VulkanGraphicsObjectTexture m_CharacterHeadTexture;
	VulkanGraphicsObjectSampler m_CharacterHeadSampler;
	VulkanGraphicsObjectTexture m_CharacterBodyTexture;
	VulkanGraphicsObjectSampler m_CharacterBodySampler;
	std::vector<VulkanGraphicsObjectTexture> m_ColorBufferArray;
	VulkanGraphicsObjectSampler m_ColorBufferSampler;
	VulkanGraphicsObjectTexture m_DepthBuffer;
	VulkanGraphicsObjectUniformBuffer m_MVPMatrixUniformBuffer;

	int m_AcquireNextImageSemaphoreIndex;
	int m_QueueSubmitFenceIndex;
	int m_QueueSubmitPrimarySemaphoreIndex;
	int m_QueueSubmitAdditionalSemaphoreIndex;
	std::vector<int> m_BackBufferIndexArray;
	std::vector<int> m_FrontBufferIndexArray;

private:
	// TODO: Need to convert this into another object...
	void CreateDescriptorSet();
	void DestroyDescriptorSet();

	VkDescriptorSetLayout m_DescriptorSetLayout;

	VkDescriptorPool m_DescriptorPool;
	VkDescriptorSet m_DescriptorSet;
};

