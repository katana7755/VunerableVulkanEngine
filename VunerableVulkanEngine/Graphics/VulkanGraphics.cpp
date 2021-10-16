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
#include "VulkanGraphicsResourcePipelineLayoutManager.h"
#include "VulkanGraphicsResourceGraphicsPipelineManager.h"
#include "VulkanGraphicsResourceDescriptorSetLayoutManager.h"

const int MAX_COMMAND_BUFFER_COUNT = 1;

size_t g_VertexShaderIdentifier = -1;
size_t g_fragmentShaderIdentifier = -1;
size_t g_PipelineIdentifier = -1;
size_t g_RenderingCommandBufferIdentifier = -1;

VulkanGraphics::VulkanGraphics()
{
	m_ResourceInstance.Create();
}

VulkanGraphics::~VulkanGraphics()
{
	EditorGameView::SetTexture(NULL, NULL);

	vkDeviceWaitIdle(VulkanGraphicsResourceDevice::GetLogicalDevice());

	VulnerableLayer::Deinitialize();

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

	VulnerableLayer::Initialize();

	BuildRenderLoop();
}
#endif

void VulkanGraphics::Invalidate()
{
	{
		auto commandPtr = VulnerableLayer::AllocateCommand<VulnerableCommand::DestroyGraphicsPipeline>();
		commandPtr->m_Identifier = g_PipelineIdentifier;
	}

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
	VulkanGraphicsResourceSwapchain::AcquireNextImage(acquireNextImageSemaphore, VK_NULL_HANDLE);


	{
		auto commandPtr = VulnerableLayer::AllocateCommand<VulnerableCommand::SubmitAllCommandBuffers>();
		commandPtr->m_WaitSemaphoreArray.push_back(acquireNextImageSemaphore);
	}

	// Execute body commands
	VulnerableLayer::ExecuteAllCommands();
}

void VulkanGraphics::SubmitAdditional()
{
	auto queueSubmitAdditionalSemaphore = m_ResourcePipelineMgr.GetGfxSemaphore(m_QueueSubmitAdditionalSemaphoreIndex);
	auto imageIndex = VulkanGraphicsResourceSwapchain::GetAcquiredImageIndex();

	std::vector<VkSemaphore> waitSemaphoreArray;
	{
		waitSemaphoreArray.push_back(VulkanGraphicsResourceCommandBufferManager::GetInstance().GetSemaphoreForSubmission());
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

	auto result = vkQueuePresentKHR(m_ResourceDevice.GetPresentQueue(), &presentInfo);

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

	m_FrontBufferIndexArray.clear();
	m_ColorBufferArray.clear();

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

	
	// TODO: when we have proper scene setting flow, remove this codes and replace with that...
	glm::mat4x4 modelMatrix = glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)), glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	glm::mat4x4 viewMatrix = glm::lookAt(glm::vec3(0.0f, 0.0f, 120.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4x4 pushProjectionMatrix = glm::perspective(glm::radians(60.0f), (float)width / height, 0.01f, 1000.0f);
	glm::mat4x4 pushMVPMatrix = pushProjectionMatrix * viewMatrix * modelMatrix;
	glm::vec3 mainLightDirection = glm::vec3(0.0f, 0.0f, 1.0f);


	{
		auto commandPtr = VulnerableLayer::AllocateCommand<VulnerableCommand::CreateShader>();
		commandPtr->m_Identifier = VulkanGraphicsResourceShaderManager::GetInstance().AllocateIdentifier();
		commandPtr->m_UploadBufferID = VulnerableUploadBufferManager::LoadFromFile("../Shaders/Output/coloredtriangle_vert.spv");
		commandPtr->m_MetaData.m_Type = EVulkanShaderType::VERTEX;
		commandPtr->m_MetaData.m_Name = "../Shaders/Output/coloredtriangle_vert.spv";
		commandPtr->m_MetaData.m_PushConstantsSize = 64 + 12; // mat4x4 + vec3
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

	// TODO: still need to handle frame buffer and render pass...
	{
		auto commandPtr = VulnerableLayer::AllocateCommand<VulnerableCommand::CreateGraphicsPipeline>();
		commandPtr->m_Identifier = VulkanGraphicsResourceGraphicsPipelineManager::GetInstance().AllocateIdentifier();
		commandPtr->m_InputData.m_RenderPassIndex = renderPassIndex; // this need to be reworked after modifying the render pass manager
		commandPtr->m_InputData.m_SubPassIndex = 0; // this need to be reworked after modifying the render pass manager
		commandPtr->m_InputData.m_ShaderIdentifiers[EVulkanShaderType::VERTEX] = g_VertexShaderIdentifier;
		commandPtr->m_InputData.m_ShaderIdentifiers[EVulkanShaderType::FRAGMENT] = g_fragmentShaderIdentifier;
		g_PipelineIdentifier = commandPtr->m_Identifier;
	}

	// Execute header commands
	VulnerableLayer::ExecuteAllCommands();


	auto pipelineData = VulkanGraphicsResourceGraphicsPipelineManager::GetInstance().GetResource(g_PipelineIdentifier);
	auto descriptorSetLayout = VulkanGraphicsResourceDescriptorSetLayoutManager::GetInstance().GetResource(pipelineData.m_DescriptorSetLayoutIdentifiers[EVulkanShaderType::FRAGMENT]);

	auto poolSizeArray = std::vector<VkDescriptorPoolSize>();
	{
		auto poolSize = VkDescriptorPoolSize();
		poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSize.descriptorCount = 2;
		poolSizeArray.push_back(poolSize);
	}

	int descriptorPoolIndex = m_ResourcePipelineMgr.CreateDescriptorPool(poolSizeArray);
	int descriptorSetIndex = m_ResourcePipelineMgr.AllocateDescriptorSet(descriptorPoolIndex, descriptorSetLayout);
	m_ResourcePipelineMgr.UpdateDescriptorSet(descriptorSetIndex, 0, m_CharacterHeadTexture.GetImageView(), m_CharacterHeadSampler.GetSampler());
	m_ResourcePipelineMgr.UpdateDescriptorSet(descriptorSetIndex, 1, m_CharacterBodyTexture.GetImageView(), m_CharacterBodySampler.GetSampler());


	if (g_RenderingCommandBufferIdentifier == (size_t)-1)
	{
		g_RenderingCommandBufferIdentifier = VulkanGraphicsResourceGraphicsPipelineManager::GetInstance().AllocateIdentifier();

		{
			auto commandPtr = VulnerableLayer::AllocateCommand<VulnerableCommand::CreateCommandBuffer>();
			commandPtr->m_Identifier = g_RenderingCommandBufferIdentifier;
			commandPtr->m_InputData.m_CommandType = EVulkanCommandType::GRAPHICS;
			commandPtr->m_InputData.m_IsTransient = false;
			commandPtr->m_InputData.m_SortOrder = 0;
		}

		{
			auto commandPtr = VulnerableLayer::AllocateCommand<VulnerableCommand::RecordCommandBuffer>();
			commandPtr->m_Identifier = g_RenderingCommandBufferIdentifier;

			auto gfxExecutionPtr = VulkanGfxExecution::AllocateExecution<VulkanGfxExecution::BeginRenderPassExecution>();
			gfxExecutionPtr->m_RenderPassIndex = renderPassIndex;
			gfxExecutionPtr->m_FramebufferIndex = m_FrontBufferIndexArray[0];
			gfxExecutionPtr->m_FrameBufferColor = m_ColorBufferArray[0].GetImage();
			gfxExecutionPtr->m_FrameBufferDepth = m_DepthBuffer.GetImage();
			gfxExecutionPtr->m_RenderArea = { {0, 0}, {width, height} };
			commandPtr->m_ExecutionPtr = gfxExecutionPtr;
		}

		{
			auto commandPtr = VulnerableLayer::AllocateCommand<VulnerableCommand::RecordCommandBuffer>();
			commandPtr->m_Identifier = g_RenderingCommandBufferIdentifier;

			auto gfxExecutionPtr = VulkanGfxExecution::AllocateExecution<VulkanGfxExecution::BindPipelineExecution>();
			gfxExecutionPtr->m_PipelineIdentifier = g_PipelineIdentifier;
			commandPtr->m_ExecutionPtr = gfxExecutionPtr;
		}

		{
			auto commandPtr = VulnerableLayer::AllocateCommand<VulnerableCommand::RecordCommandBuffer>();
			commandPtr->m_Identifier = g_RenderingCommandBufferIdentifier;

			auto gfxExecutionPtr = VulkanGfxExecution::AllocateExecution<VulkanGfxExecution::PushConstantsExecution>();
			gfxExecutionPtr->m_PipelineIdentifier = g_PipelineIdentifier;

			{
				auto dataPtr = (uint8_t*)(&pushMVPMatrix);
				size_t dataSize = sizeof(pushMVPMatrix);
				gfxExecutionPtr->m_RawDataArrays[EVulkanShaderType::VERTEX].push_back(VulkanPushContstansRawData(dataPtr, dataPtr + dataSize));
			}

			{
				auto dataPtr = (uint8_t*)(&mainLightDirection);
				size_t dataSize = sizeof(mainLightDirection);
				gfxExecutionPtr->m_RawDataArrays[EVulkanShaderType::VERTEX].push_back(VulkanPushContstansRawData(dataPtr, dataPtr + dataSize));
			}
			
			commandPtr->m_ExecutionPtr = gfxExecutionPtr;
		}

		{
			auto commandPtr = VulnerableLayer::AllocateCommand<VulnerableCommand::RecordCommandBuffer>();
			commandPtr->m_Identifier = g_RenderingCommandBufferIdentifier;

			auto gfxExecutionPtr = VulkanGfxExecution::AllocateExecution<VulkanGfxExecution::BindDescriptorSetsExecution>();
			gfxExecutionPtr->m_PipelineIdentifier = g_PipelineIdentifier;
			gfxExecutionPtr->m_DescriptorSetArray = std::vector<VkDescriptorSet>(m_ResourcePipelineMgr.GetDescriptorSetArray());
			gfxExecutionPtr->m_GfxObjectUsage.m_ReadTextureArray.push_back(m_CharacterHeadTexture.GetImage());
			gfxExecutionPtr->m_GfxObjectUsage.m_ReadTextureArray.push_back(m_CharacterBodyTexture.GetImage());
			commandPtr->m_ExecutionPtr = gfxExecutionPtr;
		}

		{
			auto commandPtr = VulnerableLayer::AllocateCommand<VulnerableCommand::RecordCommandBuffer>();
			commandPtr->m_Identifier = g_RenderingCommandBufferIdentifier;

			auto gfxExecutionPtr = VulkanGfxExecution::AllocateExecution<VulkanGfxExecution::DrawIndexedExectuion>();
			gfxExecutionPtr->m_PipelineIdentifier = g_PipelineIdentifier;
			gfxExecutionPtr->m_VertexBuffer = m_CharacterMesh.GetVertexBuffer();
			gfxExecutionPtr->m_IndexBuffer = m_CharacterMesh.GetIndexBuffer();
			gfxExecutionPtr->m_InstanceCount = 1;
			gfxExecutionPtr->m_IndexCount = m_CharacterMesh.GetIndexCount();
			gfxExecutionPtr->m_InstanceCount = 1;
			gfxExecutionPtr->m_GfxObjectUsage.m_ReadBufferArray.push_back(m_CharacterMesh.GetVertexBuffer());
			gfxExecutionPtr->m_GfxObjectUsage.m_ReadBufferArray.push_back(m_CharacterMesh.GetIndexBuffer());
			commandPtr->m_ExecutionPtr = gfxExecutionPtr;
		}

		{
			auto commandPtr = VulnerableLayer::AllocateCommand<VulnerableCommand::RecordCommandBuffer>();
			commandPtr->m_Identifier = g_RenderingCommandBufferIdentifier;
			commandPtr->m_ExecutionPtr = VulkanGfxExecution::AllocateExecution<VulkanGfxExecution::EndRenderPassExecution>();
		}

		{
			auto commandPtr = VulnerableLayer::AllocateCommand<VulnerableCommand::RecordCommandBuffer>();
			commandPtr->m_Identifier = g_RenderingCommandBufferIdentifier;

			auto gfxExecutionPtr = VulkanGfxExecution::AllocateExecution<VulkanGfxExecution::CopyImageExecution>();
			gfxExecutionPtr->m_SourceImage = m_ColorBufferArray[0].GetImage();
			gfxExecutionPtr->m_SourceLayoutFrom = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			gfxExecutionPtr->m_SourceLayoutTo = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			gfxExecutionPtr->m_SourceAccessMaskFrom = 0;
			gfxExecutionPtr->m_SourceAccessMaskTo = 0;
			gfxExecutionPtr->m_DestinationImage = m_ColorBufferArray[1].GetImage();
			gfxExecutionPtr->m_DestinationLayoutFrom = VK_IMAGE_LAYOUT_UNDEFINED;
			gfxExecutionPtr->m_DestinationLayoutTo = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			gfxExecutionPtr->m_DestinationAccessMaskFrom = 0;
			gfxExecutionPtr->m_DestinationAccessMaskTo = VK_ACCESS_SHADER_READ_BIT;
			gfxExecutionPtr->m_Width = width;
			gfxExecutionPtr->m_Height = height;
			commandPtr->m_ExecutionPtr = gfxExecutionPtr;
		}

		{
			auto commandPtr = VulnerableLayer::AllocateCommand<VulnerableCommand::BuildAllCommandBuffers>();
		}
	}

	// Execute body commands
	VulnerableLayer::ExecuteAllCommands();
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
