#define GLM_FORCE_RADIANS

#include <Windows.h>
#include <stdio.h>
#include "VulkanGraphics.h"
#include "vulkan/vulkan_win32.h" // TODO: need to consider other platfroms such as Android, Linux etc... in the future
#include "../DebugUtility.h"
#include "glm/gtc/matrix_transform.hpp"

#include "../IMGUI/imgui_impl_vulkan.h"
#include "../Editor/EditorGameView.h"

#include "VulnerableUploadBufferManager.h"
#include "VulnerableLayer.h"
#include "VulkanGraphicsResourceShaderManager.h"

const int MAX_COMMAND_BUFFER_COUNT = 1;

size_t g_VertexShaderIdentifier = -1;
size_t g_fragmentShaderIdentifier = -1;

VulkanGraphics::VulkanGraphics()
{
	m_ResourceInstance.Create();
}

VulkanGraphics::~VulkanGraphics()
{
	EditorGameView::SetTexture(NULL, NULL);

	vkDeviceWaitIdle(VulkanGraphicsResourceDevice::GetLogicalDevice());

	m_MVPMatrixUniformBuffer.Destroy();

	for (auto colorBuffer : m_ColorBufferArray)
	{
		colorBuffer.Destroy();
	}

	m_ColorBufferArray.clear();
	m_DepthBuffer.Destroy();
	m_CharacterHeadSampler.Destroy();
	m_CharacterHeadTexture.Destroy();
	m_CharacterBodySampler.Destroy();
	m_CharacterBodyTexture.Destroy();
	m_CharacterMesh.Destroy();

	m_ResourcePipelineMgr.Destroy();
	m_ResourceRenderPassMgr.Destroy();
	m_ResourceCommandBufferMgr.Destroy();
	m_ResourceSwapchain.Destroy();
	m_ResourceDevice.Destroy();
	m_ResourceSurface.Destroy();
	m_ResourceInstance.Destroy();
}

#ifdef _WIN32
void VulkanGraphics::Initialize(HINSTANCE hInstance, HWND hWnd)
{
	m_ResourceSurface.Setup(hInstance, hWnd);
	m_ResourceSurface.Create();
	m_ResourceDevice.Create();
	m_ResourceSwapchain.Create();
	m_ResourceCommandBufferMgr.Create();
	m_ResourceRenderPassMgr.Create();
	m_ResourcePipelineMgr.Create();

	m_CharacterMesh.CreateFromFBX("../FBXs/free_male_1.FBX");
	m_CharacterHeadTexture.CreateAsTexture("../PNGs/free_male_1_head_diffuse.png");
	m_CharacterHeadSampler.Create();
	m_CharacterBodyTexture.CreateAsTexture("../PNGs/free_male_1_body_diffuse.png");
	m_CharacterBodySampler.Create();
	m_DepthBuffer.CreateAsDepthBuffer();
	m_MVPMatrixUniformBuffer.Create();

	m_AcquireNextImageSemaphoreIndex = m_ResourcePipelineMgr.CreateGfxSemaphore();
	m_QueueSubmitFenceIndex = m_ResourcePipelineMgr.CreateGfxFence();
	m_QueueSubmitPrimarySemaphoreIndex = m_ResourcePipelineMgr.CreateGfxSemaphore();
	m_QueueSubmitAdditionalSemaphoreIndex = m_ResourcePipelineMgr.CreateGfxSemaphore();

	char buffer[1024];
	GetCurrentDirectory(1024, buffer);
	printf_console("[VulkanGraphics] ***** current directory is ... %s\n", buffer);

	BuildRenderLoop();
}
#endif

void VulkanGraphics::Invalidate()
{
	{
		auto commandPtr = VulnerableLayer::AllocateCommand<VulnerableCommand::DestroyShader>();
		commandPtr->m_Identifier = g_VertexShaderIdentifier;
	}

	{
		auto commandPtr = VulnerableLayer::AllocateCommand<VulnerableCommand::DestroyShader>();
		commandPtr->m_Identifier = g_fragmentShaderIdentifier;
	}

	VulnerableLayer::ExecuteAllCommands();


	EditorGameView::SetTexture(NULL, NULL);

	m_MVPMatrixUniformBuffer.Destroy();

	for (auto colorBuffer : m_ColorBufferArray)
	{
		colorBuffer.Destroy();
	}

	m_ColorBufferArray.clear();
	m_DepthBuffer.Destroy();
	m_CharacterHeadSampler.Destroy();
	m_CharacterHeadTexture.Destroy();
	m_CharacterBodySampler.Destroy();
	m_CharacterBodyTexture.Destroy();
	m_CharacterMesh.Destroy();

	m_ResourcePipelineMgr.Destroy();
	m_ResourceRenderPassMgr.Destroy();
	m_ResourceCommandBufferMgr.Destroy();
	m_ResourceSwapchain.Destroy();
	m_ResourceSwapchain.Create();
	m_ResourceCommandBufferMgr.Create();
	m_ResourceRenderPassMgr.Create();
	m_ResourcePipelineMgr.Create();

	m_CharacterMesh.CreateFromFBX("../FBXs/free_male_1.FBX");
	m_CharacterHeadTexture.CreateAsTexture("../PNGs/free_male_1_head_diffuse.png");
	m_CharacterHeadSampler.Create();
	m_CharacterBodyTexture.CreateAsTexture("../PNGs/free_male_1_body_diffuse.png");
	m_CharacterBodySampler.Create();
	m_DepthBuffer.CreateAsDepthBuffer();
	m_MVPMatrixUniformBuffer.Create();

	m_AcquireNextImageSemaphoreIndex = m_ResourcePipelineMgr.CreateGfxSemaphore();
	m_QueueSubmitFenceIndex = m_ResourcePipelineMgr.CreateGfxFence();
	m_QueueSubmitPrimarySemaphoreIndex = m_ResourcePipelineMgr.CreateGfxSemaphore();
	m_QueueSubmitAdditionalSemaphoreIndex = m_ResourcePipelineMgr.CreateGfxSemaphore();

	BuildRenderLoop();
}

void VulkanGraphics::TransferAllStagingBuffers()
{
	if (!m_CharacterHeadTexture.IsStagingBufferExist())
	{
		return;
	}

	if (!m_CharacterBodyTexture.IsStagingBufferExist())
	{
		return;
	}

	auto commandBuffer = m_ResourceCommandBufferMgr.AllocateAdditionalCommandBuffer();
	auto commandBufferArray = m_ResourceCommandBufferMgr.GetAllAdditionalCommandBuffers();

	auto beginInfo = VkCommandBufferBeginInfo();
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.pNext = NULL;
	beginInfo.flags = NULL;
	beginInfo.pInheritanceInfo = NULL; // TODO: use this when building a secondary command buffer...(NECESSARY!!!)

	auto result = vkBeginCommandBuffer(commandBuffer, &beginInfo);

	if (result)
	{
		printf_console("[VulkanGraphics] failed to begin a command buffer with error code %d\n", result);

		throw;
	}

	m_CharacterHeadTexture.ApplyStagingBuffer(commandBuffer);
	m_CharacterBodyTexture.ApplyStagingBuffer(commandBuffer);

	result = vkEndCommandBuffer(commandBuffer);

	if (result)
	{
		printf_console("[VulkanGraphics] failed to record a command buffer with error code %d\n", result);

		throw;
	}

	auto submitInfo = VkSubmitInfo();
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = NULL;
	submitInfo.waitSemaphoreCount = 0;
	submitInfo.pWaitSemaphores = NULL;
	submitInfo.pWaitDstStageMask = NULL;
	submitInfo.commandBufferCount = commandBufferArray.size(); // TODO: in the future think about submitting multiple queues simultaneously...
	submitInfo.pCommandBuffers = commandBufferArray.data();
	submitInfo.signalSemaphoreCount = 0;
	submitInfo.pSignalSemaphores = NULL;

	result = vkQueueSubmit(m_ResourceDevice.GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);

	if (result)
	{
		printf_console("[VulkanGraphics] failed to submit a queue with error code %d\n", result);

		throw;
	}

	result = vkQueueWaitIdle(m_ResourceDevice.GetGraphicsQueue());

	if (result)
	{
		printf_console("[VulkanGraphics] failed to wait a queue with error code %d\n", result);

		throw;
	}

	m_ResourceCommandBufferMgr.ClearAdditionalCommandBuffers();
	m_CharacterHeadTexture.ClearStagingBuffer();
	m_CharacterBodyTexture.ClearStagingBuffer();
}

void VulkanGraphics::InitializeFrame()
{
	m_ResourceCommandBufferMgr.ClearAdditionalCommandBuffers();
}

void VulkanGraphics::SubmitPrimary()
{
	TransferAllStagingBuffers();

	auto acquireNextImageSemaphore = m_ResourcePipelineMgr.GetGfxSemaphore(m_AcquireNextImageSemaphoreIndex);
	auto queueSubmitFence = m_ResourcePipelineMgr.GetGfxFence(m_QueueSubmitFenceIndex);
	auto queueSubmitPrimarySemaphore = m_ResourcePipelineMgr.GetGfxSemaphore(m_QueueSubmitPrimarySemaphoreIndex);
	vkWaitForFences(m_ResourceDevice.GetLogicalDevice(), 1, &queueSubmitFence, VK_TRUE, UINT64_MAX);
	vkResetFences(m_ResourceDevice.GetLogicalDevice(), 1, &queueSubmitFence);
	VulkanGraphicsResourceSwapchain::AcquireNextImage(acquireNextImageSemaphore, VK_NULL_HANDLE);

	auto imageIndex = VulkanGraphicsResourceSwapchain::GetAcquiredImageIndex();

	std::vector<VkSemaphore> waitSemaphoreArray;
	{
		waitSemaphoreArray.push_back(acquireNextImageSemaphore);
	}

	std::vector<VkPipelineStageFlags> waitDstStageMaskArray;
	{
		waitDstStageMaskArray.push_back(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
	}

	std::vector<VkCommandBuffer> commandBufferArray;
	{
		commandBufferArray.push_back(m_ResourceCommandBufferMgr.GetPrimaryCommandBuffer(imageIndex));
	}

	std::vector<VkSemaphore> signalSemaphoreArray;
	{
		signalSemaphoreArray.push_back(queueSubmitPrimarySemaphore);
	}

	auto submitInfo = VkSubmitInfo();
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = NULL;
	submitInfo.waitSemaphoreCount = waitSemaphoreArray.size();
	submitInfo.pWaitSemaphores = waitSemaphoreArray.data();
	submitInfo.pWaitDstStageMask = waitDstStageMaskArray.data();
	submitInfo.commandBufferCount = commandBufferArray.size(); // TODO: in the future think about submitting multiple queues simultaneously...
	submitInfo.pCommandBuffers = commandBufferArray.data();
	submitInfo.signalSemaphoreCount = signalSemaphoreArray.size();
	submitInfo.pSignalSemaphores = signalSemaphoreArray.data();

	auto result = vkQueueSubmit(m_ResourceDevice.GetGraphicsQueue(), 1, &submitInfo, queueSubmitFence);

	if (result)
	{
		printf_console("[VulkanGraphics] failed to submit a queue with error code %d\n", result);

		throw;
	}
}

void VulkanGraphics::SubmitAdditional()
{
	auto queueSubmitPrimarySemaphore = m_ResourcePipelineMgr.GetGfxSemaphore(m_QueueSubmitPrimarySemaphoreIndex);
	auto queueSubmitAdditionalSemaphore = m_ResourcePipelineMgr.GetGfxSemaphore(m_QueueSubmitAdditionalSemaphoreIndex);
	auto imageIndex = VulkanGraphicsResourceSwapchain::GetAcquiredImageIndex();

	std::vector<VkSemaphore> waitSemaphoreArray;
	{
		waitSemaphoreArray.push_back(queueSubmitPrimarySemaphore);
	}

	std::vector<VkPipelineStageFlags> waitDstStageMaskArray;
	{
		waitDstStageMaskArray.push_back(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
	}

	std::vector<VkCommandBuffer> commandBufferArray;
	{
		auto additionalCommandBuffers = m_ResourceCommandBufferMgr.GetAllAdditionalCommandBuffers();

		for (auto commandBuffer : additionalCommandBuffers)
		{
			commandBufferArray.push_back(commandBuffer);
		}
	}

	std::vector<VkSemaphore> signalSemaphoreArray;
	{
		signalSemaphoreArray.push_back(queueSubmitAdditionalSemaphore);
	}

	auto submitInfo = VkSubmitInfo();
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = NULL;
	submitInfo.waitSemaphoreCount = waitSemaphoreArray.size();
	submitInfo.pWaitSemaphores = waitSemaphoreArray.data();
	submitInfo.pWaitDstStageMask = waitDstStageMaskArray.data();
	submitInfo.commandBufferCount = commandBufferArray.size(); // TODO: in the future think about submitting multiple queues simultaneously...
	submitInfo.pCommandBuffers = commandBufferArray.data();
	submitInfo.signalSemaphoreCount = signalSemaphoreArray.size();
	submitInfo.pSignalSemaphores = signalSemaphoreArray.data();

	auto result = vkQueueSubmit(m_ResourceDevice.GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);

	if (result)
	{
		printf_console("[VulkanGraphics] failed to submit a queue with error code %d\n", result);

		throw;
	}
}

void VulkanGraphics::PresentFrame()
{
	//auto queueSubmitPrimarySemaphore = m_ResourcePipelineMgr.GetGfxSemaphore(m_QueueSubmitPrimarySemaphoreIndex);
	auto queueSubmitAdditionalSemaphore = m_ResourcePipelineMgr.GetGfxSemaphore(m_QueueSubmitAdditionalSemaphoreIndex);
	auto imageIndex = VulkanGraphicsResourceSwapchain::GetAcquiredImageIndex();
	std::vector<VkSemaphore> waitSemaphoreArray;
	{
		//waitSemaphoreArray.push_back(queueSubmitPrimarySemaphore);
		waitSemaphoreArray.push_back(queueSubmitAdditionalSemaphore);
	}

	auto presentInfo = VkPresentInfoKHR();
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = NULL;
	presentInfo.waitSemaphoreCount = waitSemaphoreArray.size();
	presentInfo.pWaitSemaphores = waitSemaphoreArray.data();
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &m_ResourceSwapchain.GetSwapchain();
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = NULL;

	auto result = vkQueuePresentKHR(m_ResourceDevice.GetGraphicsQueue(), &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		Invalidate();
		return;
	}

	if (result)
	{
		printf_console("[VulkanGraphics] failed to present a queue with error code %d\n", result);

		throw;
	}
}

void VulkanGraphics::BeginRenderPass(const VkCommandBuffer& commandBuffer, int renderPassIndex)
{
	uint32_t width, height, imageIndex;
	m_ResourceSwapchain.GetSwapchainSize(width, height);
	imageIndex = VulkanGraphicsResourceSwapchain::GetAcquiredImageIndex();

	std::vector<VkClearValue> clearValueArray;
	{
		auto clearValue = VkClearValue();
		clearValue.color = { 0.0f, 0.0f, 1.0f, 1.0f };
		clearValue.depthStencil.depth = 0.0f;
		clearValue.depthStencil.stencil = 0;
		clearValueArray.push_back(clearValue);
	}
	{
		auto clearValue = VkClearValue();
		clearValue.color = { 0.0f, 0.0f, 0.0f, 0.0f };
		clearValue.depthStencil.depth = 1.0f;
		clearValue.depthStencil.stencil = 0;
		clearValueArray.push_back(clearValue);
	}

	auto renderPassBegin = VkRenderPassBeginInfo();
	renderPassBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBegin.pNext = NULL;
	renderPassBegin.renderPass = m_ResourceRenderPassMgr.GetRenderPass(renderPassIndex);
	renderPassBegin.framebuffer = m_ResourceRenderPassMgr.GetFramebuffer(m_BackBufferIndexArray[imageIndex]);
	renderPassBegin.renderArea = { {0, 0}, {width, height} };
	renderPassBegin.clearValueCount = clearValueArray.size();
	renderPassBegin.pClearValues = clearValueArray.data();
	vkCmdBeginRenderPass(commandBuffer, &renderPassBegin, VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanGraphics::EndRenderPass(const VkCommandBuffer& commandBuffer)
{
	vkCmdEndRenderPass(commandBuffer);
}

void VulkanGraphics::DoTest()
{
	auto imageInfo = VkDescriptorImageInfo();
	imageInfo.sampler = m_CharacterHeadSampler.GetSampler();
	imageInfo.imageView = m_ColorBufferArray[1].GetImageView();
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	auto descriptorSet = ImGui_ImplVulkan_AddTexture(imageInfo);
	EditorGameView::SetTexture(m_ColorBufferArray[1].GetImage(), descriptorSet);
}

void VulkanGraphics::BuildRenderLoop()
{
	m_ResourceCommandBufferMgr.AllocatePrimaryBufferArray();

	std::vector<VkAttachmentDescription> attachmentDescArray;
	{
		auto desc = VkAttachmentDescription();
		desc.flags = 0;
		desc.format = VulkanGraphicsResourceSwapchain::GetSwapchainFormat();
		desc.samples = VK_SAMPLE_COUNT_1_BIT; // TODO: need to be modified when starting to consider msaa...(NECESSARY!!!)
		desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		desc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachmentDescArray.push_back(desc);
	}
	{
		auto desc = VkAttachmentDescription();
		desc.flags = 0;
		desc.format = VK_FORMAT_D32_SFLOAT; // TODO: need to define depth format (NECESSARY!!!)
		desc.samples = VK_SAMPLE_COUNT_1_BIT; // TODO: need to be modified when starting to consider msaa...(NECESSARY!!!)
		desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		desc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		attachmentDescArray.push_back(desc);
	}

	// TODO: think about optimizable use-cases of subpass...one can be postprocess...
	std::vector<VkSubpassDescription> subPassDescArray;
	std::vector<VkAttachmentReference> subPassInputAttachmentArray;
	std::vector<VkAttachmentReference> subPassColorAttachmentArray;
	auto subPassDepthStencilAttachment = VkAttachmentReference();
	std::vector<uint32_t> subPassPreserveAttachmentArray;
	{
		// subPassInputAttachmentArray
		{
			// TODO: find out when we have to use this...
		}

		// subPassColorAttachmentArray
		{
			auto ref = VkAttachmentReference();
			ref.attachment = 0;
			ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			subPassColorAttachmentArray.push_back(ref);
		}

		subPassDepthStencilAttachment.attachment = 1;
		subPassDepthStencilAttachment.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		
		// subPassPreserveAttachmentArray
		{
			// TODO: find out when we have to use this...
		}

		auto desc = VkSubpassDescription();
		desc.flags = 0; // TODO: subpass has various flags...need to check what these all are for...
		desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // TODO: one day we need to implement compute pipeline as well...(NECESSARY!!!)
		desc.inputAttachmentCount = subPassInputAttachmentArray.size();
		desc.pInputAttachments = subPassInputAttachmentArray.data();
		desc.colorAttachmentCount = subPassColorAttachmentArray.size();
		desc.pColorAttachments = subPassColorAttachmentArray.data();
		desc.pResolveAttachments = NULL; // TODO: msaa...(NECESSARY!!!)
		desc.pDepthStencilAttachment = &subPassDepthStencilAttachment;
		desc.preserveAttachmentCount = subPassPreserveAttachmentArray.size();
		desc.pPreserveAttachments = subPassPreserveAttachmentArray.data();
		subPassDescArray.push_back(desc);
	}

	// TODO: still it's not perfectly clear... let's study more on this...
	std::vector<VkSubpassDependency> subPassDepArray;
	{
		auto dep = VkSubpassDependency();
		dep.srcSubpass = VK_SUBPASS_EXTERNAL;
		dep.dstSubpass = 0;
		dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dep.srcAccessMask = 0;
		dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;// VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dep.dependencyFlags = 0;
		subPassDepArray.push_back(dep);
	}

	int renderPassIndex = m_ResourceRenderPassMgr.CreateRenderPass(attachmentDescArray, subPassDescArray, subPassDepArray);
	int swapchainImageViewCount = m_ResourceSwapchain.GetImageViewCount();
	uint32_t width, height, layers;
	m_ResourceSwapchain.GetSwapchainSize(width, height);

	m_BackBufferIndexArray.clear();

	for (int i = 0; i < swapchainImageViewCount; ++i)
	{
		m_BackBufferIndexArray.push_back(m_ResourceRenderPassMgr.CreateFramebuffer(renderPassIndex, { m_ResourceSwapchain.GetImageView(i), m_DepthBuffer.GetImageView() }, width, height, 1));
	}

	for (int i = 0; i < 1; ++i)
	{
		VulkanGraphicsObjectTexture colorBuffer;
		colorBuffer.CreateAsColorBuffer();
		m_ColorBufferArray.push_back(colorBuffer);
		m_FrontBufferIndexArray.push_back(m_ResourceRenderPassMgr.CreateFramebuffer(renderPassIndex, { colorBuffer.GetImageView(), m_DepthBuffer.GetImageView() }, width, height, 1));
	}

	{
		VulkanGraphicsObjectTexture colorBuffer;
		colorBuffer.CreateAsColorBufferForGUI();
		m_ColorBufferArray.push_back(colorBuffer);
	}

	std::vector<int> descSetLayoutArray;
	{
		descSetLayoutArray.push_back(m_ResourcePipelineMgr.CreateDescriptorSetLayout());
	}
	
	// TODO: when we have proper scene setting flow, remove this codes and replace with that...
	glm::mat4x4 modelMatrix = glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)), glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	glm::mat4x4 viewMatrix = glm::lookAt(glm::vec3(0.0f, 0.0f, 120.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4x4 pushProjectionMatrix = glm::perspective(glm::radians(60.0f), (float)width / height, 0.01f, 1000.0f);
	glm::mat4x4 pushMVPMatrix = pushProjectionMatrix * viewMatrix * modelMatrix;
	glm::vec3 mainLightDirection = glm::vec3(0.0f, 0.0f, 1.0f);
	std::vector<int> pushConstantRangeArray;
	{
		pushConstantRangeArray.push_back(m_ResourcePipelineMgr.CreatePushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushMVPMatrix) + sizeof(mainLightDirection)));
	}	

	{
		auto commandPtr = VulnerableLayer::AllocateCommand<VulnerableCommand::CreateShader>();
		commandPtr->m_Identifier = VulkanGraphicsResourceShaderManager::GetInstance().AllocateIdentifier();
		commandPtr->m_UploadBufferID = VulnerableUploadBufferManager::LoadFromFile("../Shaders/Output/coloredtriangle_vert.spv");
		commandPtr->m_MetaData.m_Type = EVulkanShaderType::VERTEX;
		commandPtr->m_MetaData.m_Name = "../Shaders/Output/coloredtriangle_vert.spv";
		commandPtr->m_MetaData.m_PushConstantOffset = 0;
		commandPtr->m_MetaData.m_PushConstantSize = 16 + 12; // mat4x4 + vec3
		commandPtr->m_MetaData.m_VertexInputArray.push_back(EVulkanShaderVertexInput::VECTOR3); // position
		commandPtr->m_MetaData.m_VertexInputArray.push_back(EVulkanShaderVertexInput::VECTOR2); // uv
		commandPtr->m_MetaData.m_VertexInputArray.push_back(EVulkanShaderVertexInput::VECTOR3); // normal
		commandPtr->m_MetaData.m_VertexInputArray.push_back(EVulkanShaderVertexInput::VECTOR1); // material
		g_VertexShaderIdentifier = commandPtr->m_Identifier;
	}

	{
		auto commandPtr = VulnerableLayer::AllocateCommand<VulnerableCommand::CreateShader>();
		commandPtr->m_Identifier = VulkanGraphicsResourceShaderManager::GetInstance().AllocateIdentifier();
		commandPtr->m_UploadBufferID = VulnerableUploadBufferManager::LoadFromFile("../Shaders/Output/coloredtriangle_frag.spv");
		commandPtr->m_MetaData.m_Type = EVulkanShaderType::FRAGMENT;
		commandPtr->m_MetaData.m_Name = "../Shaders/Output/coloredtriangle_frag.spv";
		commandPtr->m_MetaData.m_InputBindingArray.push_back(EVulkanShaderBindingResource::TEXTURE2D); // samplerDiffuseHead
		commandPtr->m_MetaData.m_InputBindingArray.push_back(EVulkanShaderBindingResource::TEXTURE2D); // samplerDiffuseBody
		g_fragmentShaderIdentifier = commandPtr->m_Identifier;
	}

	size_t testIdentifier = 0;

	// TODO: still need to handle frame buffer and render pass...
	{
		auto commandPtr = VulnerableLayer::AllocateCommand<VulnerableCommand::CreateGraphicsPipeline>();
		commandPtr->m_Identifier = VulkanGraphicsResourceGraphicsPipelineManager::GetInstance().AllocateIdentifier();
		commandPtr->m_InputData.m_ShaderIdentifiers[EVulkanShaderType::VERTEX] = g_VertexShaderIdentifier;
		commandPtr->m_InputData.m_ShaderIdentifiers[EVulkanShaderType::FRAGMENT] = g_fragmentShaderIdentifier;
	}

	VulnerableLayer::ExecuteAllCommands();
	

	int pipelineLayoutIndex = m_ResourcePipelineMgr.CreatePipelineLayout(descSetLayoutArray, pushConstantRangeArray);
	m_ResourcePipelineMgr.BeginToCreateGraphicsPipeline();

	int pipelineIndex = m_ResourcePipelineMgr.CreateGraphicsPipeline(g_VertexShaderIdentifier, g_fragmentShaderIdentifier, pipelineLayoutIndex, renderPassIndex, 0);
	m_ResourcePipelineMgr.EndToCreateGraphicsPipeline();

	auto poolSizeArray = std::vector<VkDescriptorPoolSize>();
	{
		auto poolSize = VkDescriptorPoolSize();
		poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSize.descriptorCount = 2;
		poolSizeArray.push_back(poolSize);
	}

	int descriptorPoolIndex = m_ResourcePipelineMgr.CreateDescriptorPool(poolSizeArray);
	int descriptorSetIndex = m_ResourcePipelineMgr.AllocateDescriptorSet(descriptorPoolIndex, descSetLayoutArray[0]);
	m_ResourcePipelineMgr.UpdateDescriptorSet(descriptorSetIndex, 0, m_CharacterHeadTexture.GetImageView(), m_CharacterHeadSampler.GetSampler());
	m_ResourcePipelineMgr.UpdateDescriptorSet(descriptorSetIndex, 1, m_CharacterBodyTexture.GetImageView(), m_CharacterBodySampler.GetSampler());

	auto beginInfo = VkCommandBufferBeginInfo();
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.pNext = NULL;
	beginInfo.flags = NULL;
	beginInfo.pInheritanceInfo = NULL; // TODO: use this when building a secondary command buffer...(NECESSARY!!!)

	int count = VulkanGraphicsResourceSwapchain::GetImageViewCount();

	for (int i = 0; i < count; ++i)
	{
		auto commandBuffer = m_ResourceCommandBufferMgr.GetPrimaryCommandBuffer(i);
		auto result = vkBeginCommandBuffer(commandBuffer, &beginInfo);

		if (result)
		{
			printf_console("[VulkanGraphics] failed to begin a command buffer with error code %d\n", result);

			throw;
		}

		std::vector<VkClearValue> clearValueArray;
		{
			auto clearValue = VkClearValue();
			clearValue.color = { 0.0f, 0.0f, 1.0f, 1.0f };
			clearValue.depthStencil.depth = 0.0f;
			clearValue.depthStencil.stencil = 0;
			clearValueArray.push_back(clearValue);
		}
		{
			auto clearValue = VkClearValue();
			clearValue.color = { 0.0f, 0.0f, 0.0f, 0.0f };
			clearValue.depthStencil.depth = 1.0f;
			clearValue.depthStencil.stencil = 0;
			clearValueArray.push_back(clearValue);
		}

		auto pipelineLayout = m_ResourcePipelineMgr.GetPipelineLayout(pipelineLayoutIndex);
		auto pipeline = m_ResourcePipelineMgr.GetGraphicsPipeline(pipelineIndex);
		auto renderPassBegin = VkRenderPassBeginInfo();
		renderPassBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBegin.pNext = NULL;
		renderPassBegin.renderPass = m_ResourceRenderPassMgr.GetRenderPass(renderPassIndex);
		//renderPassBegin.framebuffer = m_ResourceRenderPassMgr.GetFramebuffer(m_BackBufferIndexArray[i]);
		renderPassBegin.framebuffer = m_ResourceRenderPassMgr.GetFramebuffer(m_FrontBufferIndexArray[0]);
		renderPassBegin.renderArea = { {0, 0}, {width, height} };
		renderPassBegin.clearValueCount = clearValueArray.size();
		renderPassBegin.pClearValues = clearValueArray.data();
		vkCmdBeginRenderPass(commandBuffer, &renderPassBegin, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushMVPMatrix), &pushMVPMatrix);
		vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(pushMVPMatrix), sizeof(mainLightDirection), &mainLightDirection);
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &m_CharacterMesh.GetVertexBuffer(), new VkDeviceSize[]{ 0 }); // TODO: one day consider to bind multiple vertex buffers simultaneously...
		vkCmdBindIndexBuffer(commandBuffer, m_CharacterMesh.GetIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, m_ResourcePipelineMgr.GetDescriptorSetArray().size(), m_ResourcePipelineMgr.GetDescriptorSetArray().data(), 0, NULL); // TODO: what is dynamic offset and when do we need it?
		vkCmdDrawIndexed(commandBuffer, m_CharacterMesh.GetIndexCount(), 1, 0, 0, 0);
		vkCmdEndRenderPass(commandBuffer);

		auto imageBarrierArray = std::vector<VkImageMemoryBarrier>();
		{
			auto imageBarrier = VkImageMemoryBarrier();
			imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageBarrier.pNext = NULL;
			imageBarrier.srcAccessMask = 0;
			imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			imageBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			imageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageBarrier.image = m_ColorBufferArray[0].GetImage();
			imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageBarrier.subresourceRange.baseMipLevel = 0;
			imageBarrier.subresourceRange.levelCount = 1;
			imageBarrier.subresourceRange.baseArrayLayer = 0;
			imageBarrier.subresourceRange.layerCount = 1;
			imageBarrierArray.push_back(imageBarrier);
		}
		{
			auto imageBarrier = VkImageMemoryBarrier();
			imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageBarrier.pNext = NULL;
			imageBarrier.srcAccessMask = 0;
			imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			imageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageBarrier.image = m_ColorBufferArray[1].GetImage();
			imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageBarrier.subresourceRange.baseMipLevel = 0;
			imageBarrier.subresourceRange.levelCount = 1;
			imageBarrier.subresourceRange.baseArrayLayer = 0;
			imageBarrier.subresourceRange.layerCount = 1;
			imageBarrierArray.push_back(imageBarrier);
		}

		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, NULL, 0, NULL, imageBarrierArray.size(), imageBarrierArray.data());

		auto imageRegion = VkImageCopy();
		imageRegion.srcSubresource = VkImageSubresourceLayers { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
		imageRegion.srcOffset = VkOffset3D { 0, 0, 0 };
		imageRegion.dstSubresource = VkImageSubresourceLayers { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
		imageRegion.dstOffset = VkOffset3D { 0, 0, 0 };
		imageRegion.extent = VkExtent3D { width, height, 1 };
		vkCmdCopyImage(commandBuffer, m_ColorBufferArray[0].GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_ColorBufferArray[1].GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageRegion);
		imageBarrierArray.clear();

		{
			auto imageBarrier = VkImageMemoryBarrier();
			imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageBarrier.pNext = NULL;
			imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			imageBarrier.dstAccessMask = 0;
			imageBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			imageBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageBarrier.image = m_ColorBufferArray[0].GetImage();
			imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageBarrier.subresourceRange.baseMipLevel = 0;
			imageBarrier.subresourceRange.levelCount = 1;
			imageBarrier.subresourceRange.baseArrayLayer = 0;
			imageBarrier.subresourceRange.layerCount = 1;
			imageBarrierArray.push_back(imageBarrier);
		}
		{
			auto imageBarrier = VkImageMemoryBarrier();
			imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageBarrier.pNext = NULL;
			imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			imageBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			imageBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageBarrier.image = m_ColorBufferArray[1].GetImage();
			imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageBarrier.subresourceRange.baseMipLevel = 0;
			imageBarrier.subresourceRange.levelCount = 1;
			imageBarrier.subresourceRange.baseArrayLayer = 0;
			imageBarrier.subresourceRange.layerCount = 1;
			imageBarrierArray.push_back(imageBarrier);
		}

		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, NULL, 0, NULL, imageBarrierArray.size(), imageBarrierArray.data());

		result = vkEndCommandBuffer(commandBuffer);

		if (result)
		{
			printf_console("[VulkanGraphics] failed to record a command buffer with error code %d\n", result);

			throw;
		}
	}
}

void VulkanGraphics::CreateDescriptorSet()
{
	auto poolSizeArray = new VkDescriptorPoolSize[1];
	poolSizeArray[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizeArray[0].descriptorCount = 1;

	auto createInfo = VkDescriptorPoolCreateInfo();
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	createInfo.pNext = NULL;
	createInfo.maxSets = 1;
	createInfo.poolSizeCount = 1;
	createInfo.pPoolSizes = poolSizeArray;

	auto result = vkCreateDescriptorPool(VulkanGraphicsResourceDevice::GetLogicalDevice(), &createInfo, NULL, &m_DescriptorPool);

	if (result)
	{
		printf_console("[VulkanGraphics] failed to create a descriptor pool with error code %d\n", result);
		throw;
	}

	auto allocationInfo = VkDescriptorSetAllocateInfo();
	allocationInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocationInfo.pNext = NULL;
	allocationInfo.descriptorPool = m_DescriptorPool;
	allocationInfo.descriptorSetCount = 1;
	allocationInfo.pSetLayouts = &m_DescriptorSetLayout;
	result = vkAllocateDescriptorSets(VulkanGraphicsResourceDevice::GetLogicalDevice(), &allocationInfo, &m_DescriptorSet); // TODO: in the future let's allocate multiple descriptor sets at the same time

	if (result)
	{
		printf_console("[VulkanGraphics] failed to allocate a descriptor set with error code %d\n", result);
		throw;
	}

	auto bufferInfo = VkDescriptorBufferInfo();
	bufferInfo.buffer = m_MVPMatrixUniformBuffer.GetBuffer();
	bufferInfo.offset = 0;
	bufferInfo.range = sizeof(m_MVPMatrixUniformBuffer);

	auto writeDescriptorSet = VkWriteDescriptorSet();
	vkUpdateDescriptorSets(VulkanGraphicsResourceDevice::GetLogicalDevice(), 1, &writeDescriptorSet, 0, NULL); // TODO: let's figure out what the copy descriptor set is for...
}

void VulkanGraphics::DestroyDescriptorSet()
{
	vkFreeDescriptorSets(VulkanGraphicsResourceDevice::GetLogicalDevice(), m_DescriptorPool, 1, &m_DescriptorSet);
	vkDestroyDescriptorPool(VulkanGraphicsResourceDevice::GetLogicalDevice(), m_DescriptorPool, NULL);
}
