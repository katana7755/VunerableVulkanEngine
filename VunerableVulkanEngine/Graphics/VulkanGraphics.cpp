#define GLM_FORCE_RADIANS

#include <Windows.h>
#include <stdio.h>
#include "VulkanGraphics.h"
#include "vulkan/vulkan_win32.h" // TODO: need to consider other platfroms such as Android, Linux etc... in the future
#include "../DebugUtility.h"
#include "glm/gtc/matrix_transform.hpp"

#include "../IMGUI/imgui.h"
#include "../IMGUI/imgui_impl_win32.h"
#include "../IMGUI/imgui_impl_vulkan.h"
#include "../Editor/EditorGameView.h"
#include "../Editor/EditorManager.h"

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

int gImGuiRenderPassIndex = -1;
int gImGuiDescriptorPoolIndex = -1;
bool gImGuiFontUpdated = false;

void check_vk_result(VkResult err)
{
	if (err == 0)
		return;

	printf_console("[vulkan] Error: VkResult = %d\n", err);

	throw;
}

VulkanGraphics::VulkanGraphics()
{
	m_ResourceInstance.Create();
}

VulkanGraphics::~VulkanGraphics()
{
	vkDeviceWaitIdle(VulkanGraphicsResourceDevice::GetLogicalDevice());
	DeinitializeGUI();
	EditorGameView::SetTexture(NULL, NULL);
	VulnerableLayer::Deinitialize();
	m_MVPMatrixUniformBuffer.Destroy();

	for (auto colorBuffer : m_ColorBufferArray)
	{
		colorBuffer.Destroy();
	}

	m_ColorBufferArray.clear();
	m_DepthBuffer.Destroy();
	m_ColorBufferSampler.Destroy();
	m_CharacterHeadSampler.Destroy();
	m_CharacterHeadTexture.Destroy();
	m_CharacterBodySampler.Destroy();
	m_CharacterBodyTexture.Destroy();
	m_CharacterMesh.Destroy();

	m_ResourcePipelineMgr.Destroy();
	m_ResourceRenderPassMgr.Destroy();
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
	m_ResourceRenderPassMgr.Create();
	m_ResourcePipelineMgr.Create();

	m_CharacterMesh.CreateFromFBX("../FBXs/free_male_1.FBX");
	m_CharacterHeadTexture.CreateAsTexture("../PNGs/free_male_1_head_diffuse.png");
	m_CharacterHeadSampler.Create();
	m_CharacterBodyTexture.CreateAsTexture("../PNGs/free_male_1_body_diffuse.png");
	m_CharacterBodySampler.Create();
	m_ColorBufferSampler.Create();
	m_DepthBuffer.CreateAsDepthBuffer();
	m_MVPMatrixUniformBuffer.Create();

	m_AcquireNextImageSemaphoreIndex = m_ResourcePipelineMgr.CreateGfxSemaphore();
	m_QueueSubmitFenceIndex = m_ResourcePipelineMgr.CreateGfxFence();
	m_QueueSubmitPrimarySemaphoreIndex = m_ResourcePipelineMgr.CreateGfxSemaphore();

	char buffer[1024];
	GetCurrentDirectory(1024, buffer);
	printf_console("[VulkanGraphics] ***** current directory is ... %s\n", buffer);

	VulnerableLayer::Initialize();

	BuildRenderLoop();
	InitializeGUI(hWnd);
}
#endif

void VulkanGraphics::Invalidate()
{
	VulnerableLayer::AllocateCommandWithSetter<VulnerableCommand::DestroyGraphicsPipeline>([&](auto* commandPtr) {
		commandPtr->m_Identifier = g_PipelineIdentifier;
		});
	VulnerableLayer::AllocateCommandWithSetter<VulnerableCommand::DestroyShader>([&](auto* commandPtr) {
		commandPtr->m_Identifier = g_VertexShaderIdentifier;
		});
	VulnerableLayer::AllocateCommandWithSetter<VulnerableCommand::DestroyShader>([&](auto* commandPtr) {
		commandPtr->m_Identifier = g_fragmentShaderIdentifier;
		});
	VulnerableLayer::ExecuteAllCommands();


	EditorGameView::SetTexture(NULL, NULL);

	m_MVPMatrixUniformBuffer.Destroy();

	for (auto colorBuffer : m_ColorBufferArray)
	{
		colorBuffer.Destroy();
	}

	m_ColorBufferArray.clear();
	m_DepthBuffer.Destroy();
	m_ColorBufferSampler.Destroy();
	m_CharacterHeadSampler.Destroy();
	m_CharacterHeadTexture.Destroy();
	m_CharacterBodySampler.Destroy();
	m_CharacterBodyTexture.Destroy();
	m_CharacterMesh.Destroy();

	m_ResourcePipelineMgr.Destroy();
	m_ResourceRenderPassMgr.Destroy();
	m_ResourceSwapchain.Destroy();
	m_ResourceSwapchain.Create();
	m_ResourceRenderPassMgr.Create();
	m_ResourcePipelineMgr.Create();

	m_CharacterMesh.CreateFromFBX("../FBXs/free_male_1.FBX");
	m_CharacterHeadTexture.CreateAsTexture("../PNGs/free_male_1_head_diffuse.png");
	m_CharacterHeadSampler.Create();
	m_CharacterBodyTexture.CreateAsTexture("../PNGs/free_male_1_body_diffuse.png");
	m_CharacterBodySampler.Create();
	m_ColorBufferSampler.Create();
	m_DepthBuffer.CreateAsDepthBuffer();
	m_MVPMatrixUniformBuffer.Create();

	m_AcquireNextImageSemaphoreIndex = m_ResourcePipelineMgr.CreateGfxSemaphore();
	m_QueueSubmitFenceIndex = m_ResourcePipelineMgr.CreateGfxFence();
	m_QueueSubmitPrimarySemaphoreIndex = m_ResourcePipelineMgr.CreateGfxSemaphore();

	BuildRenderLoop();
}

void VulkanGraphics::SubmitPrimary()
{
	auto acquireNextImageSemaphore = m_ResourcePipelineMgr.GetGfxSemaphore(m_AcquireNextImageSemaphoreIndex);
	VulkanGraphicsResourceSwapchain::AcquireNextImage(acquireNextImageSemaphore, VK_NULL_HANDLE);

	VulnerableLayer::AllocateCommandWithSetter<VulnerableCommand::ClearAllTemporaryResources>(NULL);
	TransferAllStagingBuffers();
	DrawGUI(acquireNextImageSemaphore);
	VulnerableLayer::AllocateCommandWithSetter<VulnerableCommand::SubmitAllCommandBuffers>([&](auto* commandPtr) {
		if (acquireNextImageSemaphore != VK_NULL_HANDLE)
		{
			commandPtr->m_WaitSemaphoreArray.push_back(acquireNextImageSemaphore);
		}		
		});
	VulnerableLayer::ExecuteAllCommands();
}

void VulkanGraphics::PresentFrame()
{
	auto imageIndex = VulkanGraphicsResourceSwapchain::GetAcquiredImageIndex();
	std::vector<VkSemaphore> waitSemaphoreArray;
	{
		auto semaphore = VulkanGraphicsResourceCommandBufferManager::GetInstance().GetSemaphoreForSubmission();

		if (semaphore != VK_NULL_HANDLE)
		{
			waitSemaphoreArray.push_back(semaphore);
		}		
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

	VulnerableLayer::AllocateCommandWithSetter<VulnerableCommand::CreateShader>([&](auto* commandPtr) {
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
		});
	VulnerableLayer::AllocateCommandWithSetter<VulnerableCommand::CreateShader>([&](auto* commandPtr) {
		commandPtr->m_Identifier = VulkanGraphicsResourceShaderManager::GetInstance().AllocateIdentifier();
		commandPtr->m_UploadBufferID = VulnerableUploadBufferManager::LoadFromFile("../Shaders/Output/coloredtriangle_frag.spv");
		commandPtr->m_MetaData.m_Type = EVulkanShaderType::FRAGMENT;
		commandPtr->m_MetaData.m_Name = "../Shaders/Output/coloredtriangle_frag.spv";
		commandPtr->m_MetaData.m_InputBindingArray.push_back(EVulkanShaderBindingResource::TEXTURE2D); // samplerDiffuseHead
		commandPtr->m_MetaData.m_InputBindingArray.push_back(EVulkanShaderBindingResource::TEXTURE2D); // samplerDiffuseBody
		g_fragmentShaderIdentifier = commandPtr->m_Identifier;
		});

	// TODO: still need to handle frame buffer and render pass...
	VulnerableLayer::AllocateCommandWithSetter<VulnerableCommand::CreateGraphicsPipeline>([&](auto* commandPtr) {
		commandPtr->m_Identifier = VulkanGraphicsResourceGraphicsPipelineManager::GetInstance().AllocateIdentifier();
		commandPtr->m_InputData.m_RenderPassIndex = renderPassIndex; // this need to be reworked after modifying the render pass manager
		commandPtr->m_InputData.m_SubPassIndex = 0; // this need to be reworked after modifying the render pass manager
		commandPtr->m_InputData.m_ShaderIdentifiers[EVulkanShaderType::VERTEX] = g_VertexShaderIdentifier;
		commandPtr->m_InputData.m_ShaderIdentifiers[EVulkanShaderType::FRAGMENT] = g_fragmentShaderIdentifier;
		g_PipelineIdentifier = commandPtr->m_Identifier;
		});

	// Execute header commands (I handled in this way because still didn't create the descriptor set manager)....
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

		VulnerableLayer::AllocateCommandWithSetter<VulnerableCommand::CreateCommandBuffer>([&](auto* commandPtr) {
			commandPtr->m_Identifier = g_RenderingCommandBufferIdentifier;
			commandPtr->m_InputData.m_CommandType = EVulkanCommandType::GRAPHICS;
			commandPtr->m_InputData.m_IsTransient = false;
			commandPtr->m_InputData.m_SortOrder = 1;
			});

		auto* recordingCommandPtr = VulnerableLayer::AllocateCommand<VulnerableCommand::RecordCommandBuffer>();
		recordingCommandPtr->m_Identifier = g_RenderingCommandBufferIdentifier;

		VulkanGfxExecution::AllocateExecutionWithSetter<VulkanGfxExecution::BeginRenderPassExecution>(recordingCommandPtr->m_ExecutionPtrArray, [&](auto* gfxExecutionPtr) {
			gfxExecutionPtr->m_RenderPassIndex = renderPassIndex;
			gfxExecutionPtr->m_FramebufferIndex = m_FrontBufferIndexArray[0];
			gfxExecutionPtr->m_FrameBufferColor = m_ColorBufferArray[0].GetImage();
			gfxExecutionPtr->m_FrameBufferDepth = m_DepthBuffer.GetImage();
			gfxExecutionPtr->m_RenderArea = { {0, 0}, {width, height} };
			});
		VulkanGfxExecution::AllocateExecutionWithSetter<VulkanGfxExecution::BindPipelineExecution>(recordingCommandPtr->m_ExecutionPtrArray, [&](auto* gfxExecutionPtr) {
			gfxExecutionPtr->m_PipelineIdentifier = g_PipelineIdentifier;
			});
		VulkanGfxExecution::AllocateExecutionWithSetter<VulkanGfxExecution::PushConstantsExecution>(recordingCommandPtr->m_ExecutionPtrArray, [&](auto* gfxExecutionPtr) {
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
			});
		VulkanGfxExecution::AllocateExecutionWithSetter<VulkanGfxExecution::BindDescriptorSetsExecution>(recordingCommandPtr->m_ExecutionPtrArray, [&](auto* gfxExecutionPtr) {
			gfxExecutionPtr->m_PipelineIdentifier = g_PipelineIdentifier;
			gfxExecutionPtr->m_DescriptorSetArray = std::vector<VkDescriptorSet>(m_ResourcePipelineMgr.GetDescriptorSetArray());
			gfxExecutionPtr->m_GfxObjectUsage.m_ReadTextureArray.push_back(m_CharacterHeadTexture.GetImage());
			gfxExecutionPtr->m_GfxObjectUsage.m_ReadTextureArray.push_back(m_CharacterBodyTexture.GetImage());
			});
		VulkanGfxExecution::AllocateExecutionWithSetter<VulkanGfxExecution::DrawIndexedExectuion>(recordingCommandPtr->m_ExecutionPtrArray, [&](auto* gfxExecutionPtr) {
			gfxExecutionPtr->m_PipelineIdentifier = g_PipelineIdentifier;
			gfxExecutionPtr->m_VertexBuffer = m_CharacterMesh.GetVertexBuffer();
			gfxExecutionPtr->m_IndexBuffer = m_CharacterMesh.GetIndexBuffer();
			gfxExecutionPtr->m_InstanceCount = 1;
			gfxExecutionPtr->m_IndexCount = m_CharacterMesh.GetIndexCount();
			gfxExecutionPtr->m_InstanceCount = 1;
			gfxExecutionPtr->m_GfxObjectUsage.m_ReadBufferArray.push_back(m_CharacterMesh.GetVertexBuffer());
			gfxExecutionPtr->m_GfxObjectUsage.m_ReadBufferArray.push_back(m_CharacterMesh.GetIndexBuffer());
			});
		VulkanGfxExecution::AllocateExecutionWithSetter<VulkanGfxExecution::EndRenderPassExecution>(recordingCommandPtr->m_ExecutionPtrArray, NULL);
		VulkanGfxExecution::AllocateExecutionWithSetter<VulkanGfxExecution::CopyImageExecution>(recordingCommandPtr->m_ExecutionPtrArray, [&](auto* gfxExecutionPtr) {
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
			});
	}

	// Execute body commands
	VulnerableLayer::ExecuteAllCommands();
}

void VulkanGraphics::TransferAllStagingBuffers()
{
	if (!m_CharacterHeadTexture.IsStagingBufferExist() && !m_CharacterBodyTexture.IsStagingBufferExist())
	{
		m_CharacterHeadTexture.TryToClearStagingBuffer();
		m_CharacterBodyTexture.TryToClearStagingBuffer();

		return;
	}

	size_t commandBufferIdentifier = VulkanGraphicsResourceGraphicsPipelineManager::GetInstance().AllocateIdentifier();
	VulnerableLayer::AllocateCommandWithSetter<VulnerableCommand::CreateCommandBuffer>([&](auto* commandPtr) {
		commandPtr->m_Identifier = commandBufferIdentifier;
		commandPtr->m_InputData.m_CommandType = EVulkanCommandType::GRAPHICS; // graphics and compute queue both have transfer functionality, so it isn't necessary to use the specific transfer queue...
		commandPtr->m_InputData.m_IsTransient = true;
		commandPtr->m_InputData.m_SortOrder = 0;
		});

	auto* recordingCommandPtr = VulnerableLayer::AllocateCommand<VulnerableCommand::RecordCommandBuffer>();
	recordingCommandPtr->m_Identifier = commandBufferIdentifier;

	if (m_CharacterHeadTexture.IsStagingBufferExist())
	{
		m_CharacterHeadTexture.ResetStagingBufferExist();
		VulkanGfxExecution::AllocateExecutionWithSetter<VulkanGfxExecution::CopyBufferToImageExecution>(recordingCommandPtr->m_ExecutionPtrArray, [&](auto* gfxExecutionPtr) {
			gfxExecutionPtr->m_SourceBuffer = m_CharacterHeadTexture.GetStagingBuffer();
			gfxExecutionPtr->m_DestinationImage = m_CharacterHeadTexture.GetImage();
			gfxExecutionPtr->m_DestinationLayoutFrom = VK_IMAGE_LAYOUT_UNDEFINED;
			gfxExecutionPtr->m_DestinationLayoutTo = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			gfxExecutionPtr->m_DestinationAccessMaskFrom = 0;
			gfxExecutionPtr->m_DestinationAccessMaskTo = VK_ACCESS_SHADER_READ_BIT;
			gfxExecutionPtr->m_Width = m_CharacterHeadTexture.GetWidth();
			gfxExecutionPtr->m_Height = m_CharacterHeadTexture.GetHeight();
			});
	}

	if (m_CharacterBodyTexture.IsStagingBufferExist())
	{
		m_CharacterBodyTexture.ResetStagingBufferExist();
		VulkanGfxExecution::AllocateExecutionWithSetter<VulkanGfxExecution::CopyBufferToImageExecution>(recordingCommandPtr->m_ExecutionPtrArray, [&](auto* gfxExecutionPtr) {
			gfxExecutionPtr->m_SourceBuffer = m_CharacterBodyTexture.GetStagingBuffer();
			gfxExecutionPtr->m_DestinationImage = m_CharacterBodyTexture.GetImage();
			gfxExecutionPtr->m_DestinationLayoutFrom = VK_IMAGE_LAYOUT_UNDEFINED;
			gfxExecutionPtr->m_DestinationLayoutTo = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			gfxExecutionPtr->m_DestinationAccessMaskFrom = 0;
			gfxExecutionPtr->m_DestinationAccessMaskTo = VK_ACCESS_SHADER_READ_BIT;
			gfxExecutionPtr->m_Width = m_CharacterBodyTexture.GetWidth();
			gfxExecutionPtr->m_Height = m_CharacterBodyTexture.GetHeight();
			});
	}
}

#ifdef _WIN32
void VulkanGraphics::InitializeGUI(HWND hWnd)
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
		desc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
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
		dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dep.dependencyFlags = 0;
		subPassDepArray.push_back(dep);
	}

	gImGuiRenderPassIndex = VulkanGraphicsResourceRenderPassManager::CreateRenderPass(attachmentDescArray, subPassDescArray, subPassDepArray);

	auto poolSizeArray = std::vector<VkDescriptorPoolSize>();
	{
		auto poolSize = VkDescriptorPoolSize();
		poolSize.type = VK_DESCRIPTOR_TYPE_SAMPLER;
		poolSize.descriptorCount = 10;
		poolSizeArray.push_back(poolSize);
	}
	{
		auto poolSize = VkDescriptorPoolSize();
		poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSize.descriptorCount = 10;
		poolSizeArray.push_back(poolSize);
	}
	{
		auto poolSize = VkDescriptorPoolSize();
		poolSize.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		poolSize.descriptorCount = 10;
		poolSizeArray.push_back(poolSize);
	}
	{
		auto poolSize = VkDescriptorPoolSize();
		poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		poolSize.descriptorCount = 10;
		poolSizeArray.push_back(poolSize);
	}
	{
		auto poolSize = VkDescriptorPoolSize();
		poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
		poolSize.descriptorCount = 10;
		poolSizeArray.push_back(poolSize);
	}
	{
		auto poolSize = VkDescriptorPoolSize();
		poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
		poolSize.descriptorCount = 10;
		poolSizeArray.push_back(poolSize);
	}
	{
		auto poolSize = VkDescriptorPoolSize();
		poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSize.descriptorCount = 10;
		poolSizeArray.push_back(poolSize);
	}
	{
		auto poolSize = VkDescriptorPoolSize();
		poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		poolSize.descriptorCount = 10;
		poolSizeArray.push_back(poolSize);
	}
	{
		auto poolSize = VkDescriptorPoolSize();
		poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		poolSize.descriptorCount = 10;
		poolSizeArray.push_back(poolSize);
	}
	{
		auto poolSize = VkDescriptorPoolSize();
		poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
		poolSize.descriptorCount = 10;
		poolSizeArray.push_back(poolSize);
	}
	{
		auto poolSize = VkDescriptorPoolSize();
		poolSize.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		poolSize.descriptorCount = 10;
		poolSizeArray.push_back(poolSize);
	}

	gImGuiDescriptorPoolIndex = VulkanGraphicsResourcePipelineManager::CreateDescriptorPool(poolSizeArray);

	auto descriptorPool = VulkanGraphicsResourcePipelineManager::GetDescriptorPool(gImGuiDescriptorPoolIndex);

	//gImGuiWindow.Surface = VulkanGraphicsResourceSurface::GetSurface();
	//gImGuiWindow.SurfaceFormat = VkSurfaceFormatKHR { VK_FORMAT_R8G8B8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR };
	//gImGuiWindow.PresentMode = VK_PRESENT_MODE_FIFO_KHR;
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(hWnd);

	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = VulkanGraphicsResourceInstance::GetInstance();
	init_info.PhysicalDevice = VulkanGraphicsResourceDevice::GetPhysicalDevice();
	init_info.Device = VulkanGraphicsResourceDevice::GetLogicalDevice();
	init_info.QueueFamily = VulkanGraphicsResourceDevice::GetGraphicsQueueFamilyIndex();
	init_info.Queue = VulkanGraphicsResourceDevice::GetGraphicsQueue();
	init_info.PipelineCache = VulkanGraphicsResourcePipelineManager::GetPipelineCache();
	init_info.DescriptorPool = descriptorPool;
	init_info.Allocator = NULL;
	init_info.MinImageCount = 2;
	init_info.ImageCount = VulkanGraphicsResourceSwapchain::GetImageViewCount();
	init_info.CheckVkResultFn = check_vk_result;
	ImGui_ImplVulkan_Init(&init_info, VulkanGraphicsResourceRenderPassManager::GetRenderPass(gImGuiRenderPassIndex)); // TODO: will we need to make an individual render pass?
	gImGuiFontUpdated = false;

	auto imageInfo = VkDescriptorImageInfo();
	imageInfo.sampler = m_ColorBufferSampler.GetSampler();
	imageInfo.imageView = m_ColorBufferArray[1].GetImageView();
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	auto descriptorSet = ImGui_ImplVulkan_AddTexture(imageInfo);
	EditorGameView::SetTexture(m_ColorBufferArray[1].GetImage(), descriptorSet);
}
#endif

void VulkanGraphics::DeinitializeGUI()
{
	ImGui_ImplVulkan_Shutdown();
	ImGui::DestroyContext();
	gImGuiFontUpdated = false;
	VulkanGraphicsResourcePipelineManager::DestroyDescriptorPool(gImGuiDescriptorPoolIndex);
	VulkanGraphicsResourceRenderPassManager::DestroyRenderPass(gImGuiRenderPassIndex);
	gImGuiDescriptorPoolIndex = -1;
	gImGuiRenderPassIndex = -1;
}

void VulkanGraphics::DrawGUI(VkSemaphore& acquireNextImageSemaphore)
{
	// Upload Fonts
	if (!gImGuiFontUpdated)
	{
		gImGuiFontUpdated = true;

		size_t commandBufferIdentifier = VulkanGraphicsResourceGraphicsPipelineManager::GetInstance().AllocateIdentifier();
		VulnerableLayer::AllocateCommandWithSetter<VulnerableCommand::CreateCommandBuffer>([&](auto* commandPtr) {
			commandPtr->m_Identifier = commandBufferIdentifier;
			commandPtr->m_InputData.m_CommandType = EVulkanCommandType::GRAPHICS;
			commandPtr->m_InputData.m_IsTransient = true;
			commandPtr->m_InputData.m_SortOrder = 10;
			});

		auto* recordingCommandPtr = VulnerableLayer::AllocateCommand<VulnerableCommand::RecordCommandBuffer>();
		recordingCommandPtr->m_Identifier = commandBufferIdentifier;

		VulkanGfxExecution::AllocateExecutionWithSetter<VulkanGfxExecution::GUICreateFontTexturesExecution>(recordingCommandPtr->m_ExecutionPtrArray, NULL);
		VulnerableLayer::AllocateCommandWithSetter<VulnerableCommand::SubmitAllCommandBuffers>([&](auto* commandPtr) {
			commandPtr->m_WaitSemaphoreArray.push_back(acquireNextImageSemaphore);
			acquireNextImageSemaphore = VK_NULL_HANDLE;
			});
		VulnerableLayer::ExecuteAllCommands();
	}

	// Start the Dear ImGui frame
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	EditorManager::DrawEditors();

	// Rendering
	ImGui::Render();
	ImDrawData* draw_data = ImGui::GetDrawData();
	const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);

	if (!is_minimized)
	{
		size_t commandBufferIdentifier = VulkanGraphicsResourceGraphicsPipelineManager::GetInstance().AllocateIdentifier();
		VulnerableLayer::AllocateCommandWithSetter<VulnerableCommand::CreateCommandBuffer>([&](auto* commandPtr) {
			commandPtr->m_Identifier = commandBufferIdentifier;
			commandPtr->m_InputData.m_CommandType = EVulkanCommandType::GRAPHICS;
			commandPtr->m_InputData.m_IsTransient = true;
			commandPtr->m_InputData.m_SortOrder = 10;
			});

		auto* recordingCommandPtr = VulnerableLayer::AllocateCommand<VulnerableCommand::RecordCommandBuffer>();
		recordingCommandPtr->m_Identifier = commandBufferIdentifier;

		uint32_t width, height, imageIndex;
		m_ResourceSwapchain.GetSwapchainSize(width, height);
		imageIndex = VulkanGraphicsResourceSwapchain::GetAcquiredImageIndex();
		VulkanGfxExecution::AllocateExecutionWithSetter<VulkanGfxExecution::BeginRenderPassExecution>(recordingCommandPtr->m_ExecutionPtrArray, [&](auto* gfxExecutionPtr) {
			gfxExecutionPtr->m_RenderPassIndex = gImGuiRenderPassIndex;
			gfxExecutionPtr->m_FramebufferIndex = m_BackBufferIndexArray[imageIndex];
			gfxExecutionPtr->m_FrameBufferColor = m_ResourceSwapchain.GetImage(imageIndex);
			gfxExecutionPtr->m_FrameBufferDepth = m_DepthBuffer.GetImage();
			gfxExecutionPtr->m_RenderArea = { {0, 0}, {width, height} };
			});
		VulkanGfxExecution::AllocateExecutionWithSetter<VulkanGfxExecution::GUIRenderDrawDataExecution>(recordingCommandPtr->m_ExecutionPtrArray, [&](auto* gfxExecutionPtr) {
			gfxExecutionPtr->m_DrawDataPtr = draw_data;
			});
		VulkanGfxExecution::AllocateExecutionWithSetter<VulkanGfxExecution::EndRenderPassExecution>(recordingCommandPtr->m_ExecutionPtrArray, NULL);
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
