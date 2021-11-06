#pragma once
#include <vulkan/vulkan.h>
#include <vector>
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
	void SubmitPrimary();
	void PresentFrame();

private:
	void BuildRenderLoop();
	void TransferAllStagingBuffers();

#ifdef _WIN32
	void InitializeGUI(HWND hWnd);
#endif

	void DeinitializeGUI();
	void DrawGUI(VkSemaphore& acquireNextImageSemaphore);

private:
	VulkanGraphicsObjectMesh m_CharacterMesh;
	VulkanGraphicsObjectTexture m_CharacterHeadTexture;
	VulkanGraphicsObjectSampler m_CharacterHeadSampler;
	VulkanGraphicsObjectTexture m_CharacterBodyTexture;
	VulkanGraphicsObjectSampler m_CharacterBodySampler;
	std::vector<VulkanGraphicsObjectTexture> m_ColorBufferArray;
	VulkanGraphicsObjectSampler m_ColorBufferSampler;
	VulkanGraphicsObjectTexture m_DepthBuffer;
	VulkanGraphicsObjectUniformBuffer m_MVPMatrixUniformBuffer;

	size_t				m_DescriptorPoolIdentifier;
	size_t				m_DescriptorSetIdentifier;
	size_t				m_DefaultRenderPassIdentifier;
	std::vector<size_t> m_BackBufferIdentifierArray;
	std::vector<size_t> m_FrontBufferIdentifierArray;

	size_t m_VertexShaderIdentifier;
	size_t m_fragmentShaderIdentifier;
	size_t m_PipelineIdentifier;
	size_t m_RenderingCommandBufferIdentifier;

	size_t	m_ImGuiDescriptorPoolIdentifier;
	size_t	m_ImGuiRenderPassIdentifier;
	bool	m_ImGuiFontUpdated;
};

