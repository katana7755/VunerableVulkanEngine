#define GLM_FORCE_RADIANS

#include <stdio.h>
#include "VulkanGraphics.h"
#include "../DebugUtility.h"
#include "glm/gtc/matrix_transform.hpp"

#ifdef _WIN32
#include "vulkan/vulkan_win32.h" // TODO: need to consider other platfroms such as Android, Linux etc... in the future
#endif

#include "../IMGUI/imgui.h"
#include "../IMGUI/imgui_impl_vulkan.h"
#include "../Editor/EditorGameView.h"
#include "../Editor/EditorManager.h"

#ifdef _WIN32
#include "../IMGUI/imgui_impl_win32.h"
#endif

#include "VulkanGraphicsResourceInstance.h"
#include "VulkanGraphicsResourceSurface.h"
#include "VulkanGraphicsResourceDevice.h"
#include "VulkanGraphicsResourceSwapchain.h"
#include "VulkanGraphicsResourceRenderPassManager.h"
#include "VulkanGraphicsResourceFrameBufferManager.h"
#include "VulnerableUploadBufferManager.h"
#include "VulnerableLayer.h"
#include "VulkanGraphicsResourceShaderManager.h"
#include "VulkanGraphicsResourcePipelineLayoutManager.h"
#include "VulkanGraphicsResourceGraphicsPipelineManager.h"
#include "VulkanGraphicsResourceDescriptorSetLayoutManager.h"
#include "VulkanGraphicsResourceDescriptorSetManager.h"
#include "VulkanGraphicsResourcePipelineCache.h"
#include "VulkanGraphicsResourceDescriptorPoolManager.h"

const int MAX_COMMAND_BUFFER_COUNT = 1;

void check_vk_result(VkResult err)
{
	if (err == 0)
		return;

	printf_console("[vulkan] Error: VkResult = %d\n", err);

	throw;
}

VulkanGraphics::VulkanGraphics()
	: m_DescriptorPoolIdentifier(-1)
	, m_DescriptorSetIdentifier(-1)
	, m_DefaultRenderPassIdentifier(-1)
	, m_VertexShaderIdentifier(-1)
	, m_fragmentShaderIdentifier(-1)
	, m_PipelineIdentifier(-1)
	, m_RenderingCommandBufferIdentifier(-1)
	, m_ImGuiRenderPassIdentifier(-1)
	, m_ImGuiFontUpdated(false)
{
	VulkanGraphicsResourceInstance::GetInstance().Create();
}

VulkanGraphics::~VulkanGraphics()
{
	vkDeviceWaitIdle(VulkanGraphicsResourceDevice::GetInstance().GetLogicalDevice());
	DeinitializeGUI();
	EditorGameView::SetTexture(NULL, NULL);
	VulnerableLayer::Deinitialize();

	for (size_t identifier : m_BackBufferIdentifierArray)
	{
		VulkanGraphicsResourceFrameBufferManager::GetInstance().DestroyResource(identifier);
		VulkanGraphicsResourceFrameBufferManager::GetInstance().ReleaseIdentifier(identifier);
	}

	m_BackBufferIdentifierArray.clear();

	for (size_t identifier : m_FrontBufferIdentifierArray)
	{
		VulkanGraphicsResourceFrameBufferManager::GetInstance().DestroyResource(identifier);
		VulkanGraphicsResourceFrameBufferManager::GetInstance().ReleaseIdentifier(identifier);
	}

	m_FrontBufferIdentifierArray.clear();

	if (m_DefaultRenderPassIdentifier != -1)
	{
		VulkanGraphicsResourceRenderPassManager::GetInstance().DestroyResource(m_DefaultRenderPassIdentifier);
		VulkanGraphicsResourceRenderPassManager::GetInstance().ReleaseIdentifier(m_DefaultRenderPassIdentifier);
		m_DefaultRenderPassIdentifier = -1;
	}

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

	if (m_DescriptorPoolIdentifier != -1)
	{
		VulkanGraphicsResourceDescriptorPoolManager::GetInstance().DestroyResource(m_DescriptorPoolIdentifier);
		VulkanGraphicsResourceDescriptorPoolManager::GetInstance().ReleaseIdentifier(m_DescriptorPoolIdentifier);
		m_DescriptorPoolIdentifier = -1;
	}

	VulkanGraphicsResourceSwapchain::GetInstance().Destroy();
	VulkanGraphicsResourceSurface::GetInstance().Destroy();
	VulkanGraphicsResourceDevice::GetInstance().Destroy();
	VulkanGraphicsResourceInstance::GetInstance().Destroy();
}

#ifdef _WIN32
void VulkanGraphics::Initialize(HINSTANCE hInstance, HWND hWnd)
{
	m_HWnd = hWnd;

	VulkanGraphicsResourceSurface::GetInstance().Setup(hInstance, hWnd);
	VulkanGraphicsResourceSurface::GetInstance().Create();
	VulkanGraphicsResourceDevice::GetInstance().Create();
	VulkanGraphicsResourceSwapchain::GetInstance().Create();

	m_CharacterMesh.CreateFromFBX("../FBXs/free_male_1.FBX");
	m_CharacterHeadTexture.CreateAsTexture("../PNGs/free_male_1_head_diffuse.png");
	m_CharacterHeadSampler.Create();
	m_CharacterBodyTexture.CreateAsTexture("../PNGs/free_male_1_body_diffuse.png");
	m_CharacterBodySampler.Create();
	m_ColorBufferSampler.Create();
	m_DepthBuffer.CreateAsDepthBuffer();
	m_MVPMatrixUniformBuffer.Create();

	//char buffer[1024];
	//GetCurrentDirectory(1024, buffer);
	//printf_console("[VulkanGraphics] ***** current directory is ... %s\n", buffer);

	VulnerableLayer::Initialize();

	BuildRenderLoop();
	InitializeGUI();
}
#endif

void VulkanGraphics::Invalidate()
{
	vkDeviceWaitIdle(VulkanGraphicsResourceDevice::GetInstance().GetLogicalDevice());

	VulnerableLayer::AllocateCommandWithSetter<VulnerableCommand::DestroyCommandBuffer>([&](auto* commandPtr) {
		commandPtr->m_Identifier = m_RenderingCommandBufferIdentifier;
		});
	VulnerableLayer::AllocateCommandWithSetter<VulnerableCommand::DestroyGraphicsPipeline>([&](auto* commandPtr) {
		commandPtr->m_Identifier = m_PipelineIdentifier;
		});
	VulnerableLayer::AllocateCommandWithSetter<VulnerableCommand::DestroyShader>([&](auto* commandPtr) {
		commandPtr->m_Identifier = m_VertexShaderIdentifier;
		});
	VulnerableLayer::AllocateCommandWithSetter<VulnerableCommand::DestroyShader>([&](auto* commandPtr) {
		commandPtr->m_Identifier = m_fragmentShaderIdentifier;
		});
	VulnerableLayer::ExecuteAllCommands();

	m_RenderingCommandBufferIdentifier = (size_t)-1;
	DeinitializeGUI();
	EditorGameView::SetTexture(NULL, NULL);

	if (m_DescriptorSetIdentifier != -1)
	{
		VulkanGraphicsResourceDescriptorSetManager::GetInstance().DestroyResource(m_DescriptorSetIdentifier);
		VulkanGraphicsResourceDescriptorSetManager::GetInstance().ReleaseIdentifier(m_DescriptorSetIdentifier);
		m_DescriptorSetIdentifier = -1;
	}

	if (m_DescriptorPoolIdentifier != -1)
	{
		VulkanGraphicsResourceDescriptorPoolManager::GetInstance().DestroyResource(m_DescriptorPoolIdentifier);
		VulkanGraphicsResourceDescriptorPoolManager::GetInstance().ReleaseIdentifier(m_DescriptorPoolIdentifier);
		m_DescriptorPoolIdentifier = -1;
	}

	for (size_t identifier : m_BackBufferIdentifierArray)
	{
		VulkanGraphicsResourceFrameBufferManager::GetInstance().DestroyResource(identifier);
		VulkanGraphicsResourceFrameBufferManager::GetInstance().ReleaseIdentifier(identifier);
	}

	m_BackBufferIdentifierArray.clear();

	for (size_t identifier : m_FrontBufferIdentifierArray)
	{
		VulkanGraphicsResourceFrameBufferManager::GetInstance().DestroyResource(identifier);
		VulkanGraphicsResourceFrameBufferManager::GetInstance().ReleaseIdentifier(identifier);
	}

	m_FrontBufferIdentifierArray.clear();

	if (m_DefaultRenderPassIdentifier != -1)
	{
		VulkanGraphicsResourceRenderPassManager::GetInstance().DestroyResource(m_DefaultRenderPassIdentifier);
		VulkanGraphicsResourceRenderPassManager::GetInstance().ReleaseIdentifier(m_DefaultRenderPassIdentifier);
		m_DefaultRenderPassIdentifier = -1;
	}

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

	if (m_DescriptorPoolIdentifier != -1)
	{
		VulkanGraphicsResourceDescriptorPoolManager::GetInstance().DestroyResource(m_DescriptorPoolIdentifier);
		VulkanGraphicsResourceDescriptorPoolManager::GetInstance().ReleaseIdentifier(m_DescriptorPoolIdentifier);
		m_DescriptorPoolIdentifier = -1;
	}

	VulkanGraphicsResourceSwapchain::GetInstance().Destroy();
	VulkanGraphicsResourceSwapchain::GetInstance().Create();

	m_CharacterMesh.CreateFromFBX("../FBXs/free_male_1.FBX");
	m_CharacterHeadTexture.CreateAsTexture("../PNGs/free_male_1_head_diffuse.png");
	m_CharacterHeadSampler.Create();
	m_CharacterBodyTexture.CreateAsTexture("../PNGs/free_male_1_body_diffuse.png");
	m_CharacterBodySampler.Create();
	m_ColorBufferSampler.Create();
	m_DepthBuffer.CreateAsDepthBuffer();
	m_MVPMatrixUniformBuffer.Create();

	BuildRenderLoop();
	InitializeGUI();
}

void VulkanGraphics::SubmitPrimary()
{
	VulkanGraphicsResourceSwapchain::GetInstance().AcquireNextImage();

	auto acquireImageSemaphore = VulkanGraphicsResourceSwapchain::GetInstance().GetAcquireImageSemaphore();
	VulnerableLayer::AllocateCommandWithSetter<VulnerableCommand::ClearAllTemporaryResources>(NULL);
	TransferAllStagingBuffers();
	DrawGUI(acquireImageSemaphore);
	VulnerableLayer::AllocateCommandWithSetter<VulnerableCommand::SubmitAllCommandBuffers>([&](auto* commandPtr) {
		if (acquireImageSemaphore != VK_NULL_HANDLE)
		{
			commandPtr->m_WaitSemaphoreArray.push_back(acquireImageSemaphore);
		}		
		});
	VulnerableLayer::ExecuteAllCommands();
}

void VulkanGraphics::PresentFrame()
{
	auto imageIndex = VulkanGraphicsResourceSwapchain::GetInstance().GetAcquiredImageIndex();
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
	presentInfo.pSwapchains = &VulkanGraphicsResourceSwapchain::GetInstance().GetSwapchain();
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = NULL;

	auto result = vkQueuePresentKHR(VulkanGraphicsResourceDevice::GetInstance().GetPresentQueue(), &presentInfo);

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

void VulkanGraphics::BuildRenderLoop()
{
	m_DefaultRenderPassIdentifier = VulkanGraphicsResourceRenderPassManager::GetInstance().AllocateIdentifier();

	// TODO: this should be abstracted in order to be a high level api...
	// TODO: additionally, this is needed to be a header command in VulnerableLayer...
	auto renderPassInputData = VulkanRenderPassInputData();
	{
		auto desc = VkAttachmentDescription();
		desc.flags = 0;
		desc.format = VulkanGraphicsResourceSwapchain::GetInstance().GetSwapchainFormat();
		desc.samples = VK_SAMPLE_COUNT_1_BIT; // TODO: need to be modified when starting to consider msaa...(NECESSARY!!!)
		desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		desc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		renderPassInputData.m_AttachmentDescriptionArray.push_back(desc);
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
		renderPassInputData.m_AttachmentDescriptionArray.push_back(desc);
	}

	// TODO: think about optimizable use-cases of subpass...one can be postprocess...
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
		renderPassInputData.m_SubpassDescriptionArray.push_back(desc);
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
		renderPassInputData.m_SubpassDependencyArray.push_back(dep);
	}

	VulkanGraphicsResourceRenderPassManager::GetInstance().CreateResource(m_DefaultRenderPassIdentifier, renderPassInputData, m_DefaultRenderPassIdentifier);

	int swapchainImageViewCount = VulkanGraphicsResourceSwapchain::GetInstance().GetImageViewCount();
	uint32_t width, height, layers;
	VulkanGraphicsResourceSwapchain::GetInstance().GetSwapchainSize(width, height);

	for (int i = 0; i < swapchainImageViewCount; ++i)
	{
		auto frameBufferInputData = VulkanFrameBufferInputData();
		frameBufferInputData.m_RenderPassIdentifier = m_DefaultRenderPassIdentifier;
		frameBufferInputData.m_AttachmentArray.push_back(VulkanGraphicsResourceSwapchain::GetInstance().GetImageView(i));
		frameBufferInputData.m_AttachmentArray.push_back(m_DepthBuffer.GetImageView());
		frameBufferInputData.m_Width = width;
		frameBufferInputData.m_Height = height;
		frameBufferInputData.m_Layers = 1;

		size_t frameBufferIdentifier = VulkanGraphicsResourceFrameBufferManager::GetInstance().AllocateIdentifier();
		VulkanGraphicsResourceFrameBufferManager::GetInstance().CreateResource(frameBufferIdentifier, frameBufferInputData, frameBufferIdentifier);
		m_BackBufferIdentifierArray.push_back(frameBufferIdentifier);
	}

	m_ColorBufferArray.clear();

	for (int i = 0; i < 1; ++i)
	{
		VulkanGraphicsObjectTexture colorBuffer;
		colorBuffer.CreateAsColorBuffer();
		m_ColorBufferArray.push_back(colorBuffer);

		auto frameBufferInputData = VulkanFrameBufferInputData();
		frameBufferInputData.m_RenderPassIdentifier = m_DefaultRenderPassIdentifier;
		frameBufferInputData.m_AttachmentArray.push_back(colorBuffer.GetImageView());
		frameBufferInputData.m_AttachmentArray.push_back(m_DepthBuffer.GetImageView());
		frameBufferInputData.m_Width = width;
		frameBufferInputData.m_Height = height;
		frameBufferInputData.m_Layers = 1;

		size_t frameBufferIdentifier = VulkanGraphicsResourceFrameBufferManager::GetInstance().AllocateIdentifier();
		VulkanGraphicsResourceFrameBufferManager::GetInstance().CreateResource(frameBufferIdentifier, frameBufferInputData, frameBufferIdentifier);
		m_FrontBufferIdentifierArray.push_back(frameBufferIdentifier);
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
		m_VertexShaderIdentifier = commandPtr->m_Identifier;
		});
	VulnerableLayer::AllocateCommandWithSetter<VulnerableCommand::CreateShader>([&](auto* commandPtr) {
		commandPtr->m_Identifier = VulkanGraphicsResourceShaderManager::GetInstance().AllocateIdentifier();
		commandPtr->m_UploadBufferID = VulnerableUploadBufferManager::LoadFromFile("../Shaders/Output/coloredtriangle_frag.spv");
		commandPtr->m_MetaData.m_Type = EVulkanShaderType::FRAGMENT;
		commandPtr->m_MetaData.m_Name = "../Shaders/Output/coloredtriangle_frag.spv";
		commandPtr->m_MetaData.m_InputBindingArray.push_back(EVulkanShaderBindingResource::TEXTURE2D); // samplerDiffuseHead
		commandPtr->m_MetaData.m_InputBindingArray.push_back(EVulkanShaderBindingResource::TEXTURE2D); // samplerDiffuseBody
		m_fragmentShaderIdentifier = commandPtr->m_Identifier;
		});
	VulnerableLayer::AllocateCommandWithSetter<VulnerableCommand::CreateGraphicsPipeline>([&](auto* commandPtr) {
		commandPtr->m_Identifier = VulkanGraphicsResourceGraphicsPipelineManager::GetInstance().AllocateIdentifier();
		commandPtr->m_InputData.m_RenderPassIdentifier = m_DefaultRenderPassIdentifier; // this need to be reworked after modifying the render pass manager
		commandPtr->m_InputData.m_SubPassIndex = 0; // this need to be reworked after modifying the render pass manager
		commandPtr->m_InputData.m_ShaderIdentifiers[EVulkanShaderType::VERTEX] = m_VertexShaderIdentifier;
		commandPtr->m_InputData.m_ShaderIdentifiers[EVulkanShaderType::FRAGMENT] = m_fragmentShaderIdentifier;
		m_PipelineIdentifier = commandPtr->m_Identifier;
		});

	// Execute header commands (I handled in this way because still didn't create the descriptor set manager)....
	VulnerableLayer::ExecuteAllCommands();


	auto pipelineData = VulkanGraphicsResourceGraphicsPipelineManager::GetInstance().GetResource(m_PipelineIdentifier);
	auto descriptorSetLayout = VulkanGraphicsResourceDescriptorSetLayoutManager::GetInstance().GetResource(pipelineData.m_DescriptorSetLayoutIdentifiers[EVulkanShaderType::FRAGMENT]);

	if (m_DescriptorPoolIdentifier == -1)
	{
		m_DescriptorPoolIdentifier = VulkanGraphicsResourceDescriptorPoolManager::GetInstance().AllocateIdentifier();

		auto inputData = VulkanDescriptorPoolInputData();
		inputData.CreateDefault();
		VulkanGraphicsResourceDescriptorPoolManager::GetInstance().CreateResource(m_DescriptorPoolIdentifier, inputData, m_DescriptorPoolIdentifier);
	}

	if (m_DescriptorSetIdentifier == -1)
	{
		m_DescriptorSetIdentifier = VulkanGraphicsResourceDescriptorSetManager::GetInstance().AllocateIdentifier();

		auto inputData = VulkanDescriptorSetInputData();
		inputData.m_DescriptorPoolIdentifier = m_DescriptorPoolIdentifier;
		inputData.m_PipelineIdentifier = m_PipelineIdentifier;
		inputData.m_ShaderType = EVulkanShaderType::FRAGMENT;
		VulkanGraphicsResourceDescriptorSetManager::GetInstance().CreateResource(m_DescriptorSetIdentifier, inputData, m_DescriptorSetIdentifier);

		auto& outputData = VulkanGraphicsResourceDescriptorSetManager::GetInstance().GetResource(m_DescriptorSetIdentifier);
		outputData.ClearBindingInfos();
		outputData.BindCombinedSampler(0, m_CharacterHeadTexture.GetImage(), m_CharacterHeadTexture.GetImageView(), m_CharacterHeadSampler.GetSampler());
		outputData.BindCombinedSampler(1, m_CharacterBodyTexture.GetImage(), m_CharacterBodyTexture.GetImageView(), m_CharacterBodySampler.GetSampler());
		outputData.FlushBindingInfos();
	}

	if (m_RenderingCommandBufferIdentifier == (size_t)-1)
	{
		m_RenderingCommandBufferIdentifier = VulkanGraphicsResourceCommandBufferManager::GetInstance().AllocateIdentifier();

		VulnerableLayer::AllocateCommandWithSetter<VulnerableCommand::CreateCommandBuffer>([&](auto* commandPtr) {
			commandPtr->m_Identifier = m_RenderingCommandBufferIdentifier;
			commandPtr->m_InputData.m_CommandType = EVulkanCommandType::GRAPHICS;
			commandPtr->m_InputData.m_IsTransient = false;
			commandPtr->m_InputData.m_SortOrder = 1;
			});

		auto* recordingCommandPtr = VulnerableLayer::AllocateCommand<VulnerableCommand::RecordCommandBuffer>();
		recordingCommandPtr->m_Identifier = m_RenderingCommandBufferIdentifier;

		VulkanGfxExecution::AllocateExecutionWithSetter<VulkanGfxExecution::BeginRenderPassExecution>(recordingCommandPtr->m_ExecutionPtrArray, [&](auto* gfxExecutionPtr) {
			gfxExecutionPtr->m_RenderPassIdentifier = m_DefaultRenderPassIdentifier;
			gfxExecutionPtr->m_FrameBufferIdentifier = m_FrontBufferIdentifierArray[0];
			gfxExecutionPtr->m_FrameBufferColor = m_ColorBufferArray[0].GetImage();
			gfxExecutionPtr->m_FrameBufferDepth = m_DepthBuffer.GetImage();
			gfxExecutionPtr->m_RenderArea = { {0, 0}, {width, height} };
			});
		VulkanGfxExecution::AllocateExecutionWithSetter<VulkanGfxExecution::BindPipelineExecution>(recordingCommandPtr->m_ExecutionPtrArray, [&](auto* gfxExecutionPtr) {
			gfxExecutionPtr->m_PipelineIdentifier = m_PipelineIdentifier;
			});
		VulkanGfxExecution::AllocateExecutionWithSetter<VulkanGfxExecution::PushConstantsExecution>(recordingCommandPtr->m_ExecutionPtrArray, [&](auto* gfxExecutionPtr) {
			gfxExecutionPtr->m_PipelineIdentifier = m_PipelineIdentifier;
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
			gfxExecutionPtr->m_PipelineIdentifier = m_PipelineIdentifier;
			gfxExecutionPtr->m_DescriptorSetIdentifierArray.push_back(m_DescriptorSetIdentifier);
			gfxExecutionPtr->m_GfxObjectUsage.m_ReadTextureArray.push_back(m_CharacterHeadTexture.GetImage());
			gfxExecutionPtr->m_GfxObjectUsage.m_ReadTextureArray.push_back(m_CharacterBodyTexture.GetImage());
			});
		VulkanGfxExecution::AllocateExecutionWithSetter<VulkanGfxExecution::DrawIndexedExectuion>(recordingCommandPtr->m_ExecutionPtrArray, [&](auto* gfxExecutionPtr) {
			gfxExecutionPtr->m_PipelineIdentifier = m_PipelineIdentifier;
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

	size_t commandBufferIdentifier = VulkanGraphicsResourceCommandBufferManager::GetInstance().AllocateIdentifier();
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
void VulkanGraphics::InitializeGUI()
{
	m_ImGuiRenderPassIdentifier = VulkanGraphicsResourceRenderPassManager::GetInstance().AllocateIdentifier();

	auto renderPassInputData = VulkanRenderPassInputData();
	{
		auto desc = VkAttachmentDescription();
		desc.flags = 0;
		desc.format = VulkanGraphicsResourceSwapchain::GetInstance().GetSwapchainFormat();
		desc.samples = VK_SAMPLE_COUNT_1_BIT; // TODO: need to be modified when starting to consider msaa...(NECESSARY!!!)
		desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		desc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		renderPassInputData.m_AttachmentDescriptionArray.push_back(desc);
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
		renderPassInputData.m_AttachmentDescriptionArray.push_back(desc);
	}

	// TODO: think about optimizable use-cases of subpass...one can be postprocess...
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
		renderPassInputData.m_SubpassDescriptionArray.push_back(desc);
	}

	// TODO: still it's not perfectly clear... let's study more on this...
	{
		auto dep = VkSubpassDependency();
		dep.srcSubpass = VK_SUBPASS_EXTERNAL;
		dep.dstSubpass = 0;
		dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dep.srcAccessMask = 0;
		dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dep.dependencyFlags = 0;
		renderPassInputData.m_SubpassDependencyArray.push_back(dep);
	}

	VulkanGraphicsResourceRenderPassManager::GetInstance().CreateResource(m_ImGuiRenderPassIdentifier, renderPassInputData, m_ImGuiRenderPassIdentifier);

	if (m_DescriptorPoolIdentifier == -1)
	{
		m_DescriptorPoolIdentifier = VulkanGraphicsResourceDescriptorPoolManager::GetInstance().AllocateIdentifier();

		auto descriptorPoolInputData = VulkanDescriptorPoolInputData();
		descriptorPoolInputData.CreateDefault();
		VulkanGraphicsResourceDescriptorPoolManager::GetInstance().CreateResource(m_DescriptorPoolIdentifier, descriptorPoolInputData, m_DescriptorPoolIdentifier);
	}

	auto descriptorPool = VulkanGraphicsResourceDescriptorPoolManager::GetInstance().GetResource(m_DescriptorPoolIdentifier);

	//gImGuiWindow.Surface = VulkanGraphicsResourceSurface::GetSurface();
	//gImGuiWindow.SurfaceFormat = VkSurfaceFormatKHR { VK_FORMAT_R8G8B8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR };
	//gImGuiWindow.PresentMode = VK_PRESENT_MODE_FIFO_KHR;
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(m_HWnd);

	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = VulkanGraphicsResourceInstance::GetInstance().GetVkInstance();
	init_info.PhysicalDevice = VulkanGraphicsResourceDevice::GetInstance().GetPhysicalDevice();
	init_info.Device = VulkanGraphicsResourceDevice::GetInstance().GetLogicalDevice();
	init_info.QueueFamily = VulkanGraphicsResourceDevice::GetInstance().GetGraphicsQueueFamilyIndex();
	init_info.Queue = VulkanGraphicsResourceDevice::GetInstance().GetGraphicsQueue();
	init_info.PipelineCache = VulkanGraphicsResourcePipelineCache::GetInstance().GetPipelineCache();
	init_info.DescriptorPool = descriptorPool;
	init_info.Allocator = NULL;
	init_info.MinImageCount = 2;
	init_info.ImageCount = VulkanGraphicsResourceSwapchain::GetInstance().GetImageViewCount();
	init_info.CheckVkResultFn = check_vk_result;
	ImGui_ImplVulkan_Init(&init_info, VulkanGraphicsResourceRenderPassManager::GetInstance().GetResource(m_ImGuiRenderPassIdentifier)); // TODO: will we need to make an individual render pass?
	m_ImGuiFontUpdated = false;

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
	m_ImGuiFontUpdated = false;

	VulkanGraphicsResourceRenderPassManager::GetInstance().DestroyResource(m_ImGuiRenderPassIdentifier);
	VulkanGraphicsResourceRenderPassManager::GetInstance().ReleaseIdentifier(m_ImGuiRenderPassIdentifier);
	m_ImGuiRenderPassIdentifier = -1;
}

void VulkanGraphics::DrawGUI(VkSemaphore& acquireNextImageSemaphore)
{
	// Upload Fonts
	if (!m_ImGuiFontUpdated)
	{
		m_ImGuiFontUpdated = true;

		size_t commandBufferIdentifier = VulkanGraphicsResourceCommandBufferManager::GetInstance().AllocateIdentifier();
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
	EditorManager::GetInstance().DrawEditors();

	// Rendering
	ImGui::Render();
	ImDrawData* draw_data = ImGui::GetDrawData();
	const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);

	if (!is_minimized)
	{
		size_t commandBufferIdentifier = VulkanGraphicsResourceCommandBufferManager::GetInstance().AllocateIdentifier();
		VulnerableLayer::AllocateCommandWithSetter<VulnerableCommand::CreateCommandBuffer>([&](auto* commandPtr) {
			commandPtr->m_Identifier = commandBufferIdentifier;
			commandPtr->m_InputData.m_CommandType = EVulkanCommandType::GRAPHICS;
			commandPtr->m_InputData.m_IsTransient = true;
			commandPtr->m_InputData.m_SortOrder = 10;
			});

		auto* recordingCommandPtr = VulnerableLayer::AllocateCommand<VulnerableCommand::RecordCommandBuffer>();
		recordingCommandPtr->m_Identifier = commandBufferIdentifier;

		uint32_t width, height, imageIndex;
		VulkanGraphicsResourceSwapchain::GetInstance().GetSwapchainSize(width, height);
		imageIndex = VulkanGraphicsResourceSwapchain::GetInstance().GetAcquiredImageIndex();
		VulkanGfxExecution::AllocateExecutionWithSetter<VulkanGfxExecution::BeginRenderPassExecution>(recordingCommandPtr->m_ExecutionPtrArray, [&](auto* gfxExecutionPtr) {
			gfxExecutionPtr->m_RenderPassIdentifier = m_ImGuiRenderPassIdentifier;
			gfxExecutionPtr->m_FrameBufferIdentifier = m_BackBufferIdentifierArray[imageIndex];
			gfxExecutionPtr->m_FrameBufferColor = VulkanGraphicsResourceSwapchain::GetInstance().GetImage(imageIndex);
			gfxExecutionPtr->m_FrameBufferDepth = m_DepthBuffer.GetImage();
			gfxExecutionPtr->m_RenderArea = { {0, 0}, {width, height} };
			});
		VulkanGfxExecution::AllocateExecutionWithSetter<VulkanGfxExecution::GUIRenderDrawDataExecution>(recordingCommandPtr->m_ExecutionPtrArray, [&](auto* gfxExecutionPtr) {
			gfxExecutionPtr->m_DrawDataPtr = draw_data;
			});
		VulkanGfxExecution::AllocateExecutionWithSetter<VulkanGfxExecution::EndRenderPassExecution>(recordingCommandPtr->m_ExecutionPtrArray, NULL);
	}
}

