#define GLM_FORCE_RADIANS

#include <Windows.h>
#include <stdio.h>
#include "VulkanGraphics.h"
#include "vulkan/vulkan_win32.h" // TODO: need to consider other platfroms such as Android, Linux etc... in the future
#include "../DebugUtility.h"
#include "glm/gtc/matrix_transform.hpp"

const int MAX_COMMAND_BUFFER_COUNT = 1;

VulkanGraphics::VulkanGraphics()
{
	m_ResourceInstance.Create();
}

VulkanGraphics::~VulkanGraphics()
{
	vkDeviceWaitIdle(VulkanGraphicsResourceDevice::GetLogicalDevice());

	m_MVPMatrixUniformBuffer.Destroy();
	m_DepthTexture.Destroy();
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
	m_CharacterBodyTexture.CreateAsTexture("../PNGs/free_male_1_body_diffuse.png");
	m_CharacterBodySampler.Create();
	m_DepthTexture.CreateAsDepthBuffer();
	m_MVPMatrixUniformBuffer.Create();

	m_AcquireNextImageSemaphoreIndex = m_ResourcePipelineMgr.CreateGfxSemaphore();
	m_QueueSubmitFenceIndex = m_ResourcePipelineMgr.CreateGfxFence();
	m_QueueSubmitSemaphoreIndex = m_ResourcePipelineMgr.CreateGfxSemaphore();

	char buffer[1024];
	GetCurrentDirectory(1024, buffer);
	printf_console("[VulkanGraphics] ***** current directory is ... %s\n", buffer);

	BuildRenderLoop();
}
#endif

void VulkanGraphics::Invalidate()
{
	m_MVPMatrixUniformBuffer.Destroy();
	m_DepthTexture.Destroy();
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
	m_CharacterBodyTexture.CreateAsTexture("../PNGs/free_male_1_body_diffuse.png");
	m_CharacterBodySampler.Create();
	m_DepthTexture.CreateAsDepthBuffer();
	m_MVPMatrixUniformBuffer.Create();

	m_AcquireNextImageSemaphoreIndex = m_ResourcePipelineMgr.CreateGfxSemaphore();
	m_QueueSubmitFenceIndex = m_ResourcePipelineMgr.CreateGfxFence();
	m_QueueSubmitSemaphoreIndex = m_ResourcePipelineMgr.CreateGfxSemaphore();

	BuildRenderLoop();
}

void VulkanGraphics::TransferAllStagingBuffers()
{
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

	m_ResourceCommandBufferMgr.ClearAdditionalCommandBuffer();
	m_CharacterBodyTexture.ClearStagingBuffer();
}

void VulkanGraphics::DrawFrame()
{
	TransferAllStagingBuffers();

	auto acquireNextImageSemaphore = m_ResourcePipelineMgr.GetGfxSemaphore(m_AcquireNextImageSemaphoreIndex);
	auto queueSubmitFence = m_ResourcePipelineMgr.GetGfxFence(m_QueueSubmitFenceIndex);
	auto queueSubmitSemaphore = m_ResourcePipelineMgr.GetGfxSemaphore(m_QueueSubmitSemaphoreIndex);
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
		signalSemaphoreArray.push_back(queueSubmitSemaphore);
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

	auto presentInfo = VkPresentInfoKHR();
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = NULL;
	presentInfo.waitSemaphoreCount = signalSemaphoreArray.size();
	presentInfo.pWaitSemaphores = signalSemaphoreArray.data();
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &m_ResourceSwapchain.GetSwapchain();
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = NULL;
	result = vkQueuePresentKHR(m_ResourceDevice.GetGraphicsQueue(), &presentInfo);

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
		dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;// VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dep.dependencyFlags = 0;
		subPassDepArray.push_back(dep);
	}

	int renderPassIndex = m_ResourceRenderPassMgr.CreateRenderPass(attachmentDescArray, subPassDescArray, subPassDepArray);
	int swapchainImageViewCount = m_ResourceSwapchain.GetImageViewCount();
	uint32_t width, height, layers;
	m_ResourceSwapchain.GetSwapchainSize(width, height);

	for (int i = 0; i < swapchainImageViewCount; ++i)
	{
		m_ResourceRenderPassMgr.CreateFramebuffer(renderPassIndex, { m_ResourceSwapchain.GetImageView(i), m_DepthTexture.GetImageView() }, width, height, 1);
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

	int vertexShaderModuleIndex = m_ResourcePipelineMgr.CreateShaderModule("../Shaders/Output/coloredtriangle_vert.spv");
	int fragmentShaderModuleIndex = m_ResourcePipelineMgr.CreateShaderModule("../Shaders/Output/coloredtriangle_frag.spv");
	int pipelineLayoutIndex = m_ResourcePipelineMgr.CreatePipelineLayout(descSetLayoutArray, pushConstantRangeArray);
	m_ResourcePipelineMgr.BeginToCreateGraphicsPipeline();

	int pipelineIndex = m_ResourcePipelineMgr.CreateGraphicsPipeline(vertexShaderModuleIndex, fragmentShaderModuleIndex, pipelineLayoutIndex, renderPassIndex, 0);
	m_ResourcePipelineMgr.EndToCreateGraphicsPipeline();

	int descriptorPoolIndex = m_ResourcePipelineMgr.CreateDescriptorPool();
	int descriptorSetIndex = m_ResourcePipelineMgr.AllocateDescriptorSet(descriptorPoolIndex, descSetLayoutArray[0]);
	m_ResourcePipelineMgr.UpdateDescriptorSet(descriptorSetIndex, m_CharacterBodyTexture.GetImageView(), m_CharacterBodySampler.GetSampler());

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
		renderPassBegin.framebuffer = m_ResourceRenderPassMgr.GetFramebuffer(i);
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

void VulkanGraphics::CreateShaderModule()
{
	//auto createInfo = VkShaderModuleCreateInfo();
	//createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	//vkCreateShaderModule(VulkanGraphicsResourceDevice::GetLogicalDevice(), )
}

void VulkanGraphics::DestroyShaderModule()
{
}