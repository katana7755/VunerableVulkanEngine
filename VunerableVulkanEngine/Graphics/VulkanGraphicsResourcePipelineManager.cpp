#include "VulkanGraphicsResourcePipelineManager.h"
#include "../DebugUtility.h"
#include "VulkanGraphicsResourceDevice.h"
#include "VulkanGraphicsResourceSwapchain.h"
#include "VulkanGraphicsResourceRenderPassManager.h"
#include "VulkanGraphicsObjectMesh.h"
#include <stdio.h>
#include <unordered_map>
#include <limits>
#include "VulkanGraphicsResourceShaderManager.h"

const char* PIPELINE_CACHE_DATA_FILE_NAME = "vkpipeline_cache_data.bin";

VkPipelineCache s_PipelineCache;
std::vector<VkFence> s_GfxFenceArray;
std::vector<VkSemaphore> s_GfxSemaphoreArray;
std::vector<VkDescriptorSetLayout> s_DescriptorSetLayoutArray;
std::vector<VkPushConstantRange> s_PushConstantRangeArray;
std::vector<VkPipelineLayout> s_PipelineLayoutArray;
std::vector<VkGraphicsPipelineCreateInfo> s_GraphicsPipelineCreateInfoArray;
std::vector<VkPipeline> s_GraphicsPipelineArray;
std::vector<VkDescriptorPool> s_DescriptorPoolArray;
std::vector<int> s_DescriptorPoolIndexArray;
std::vector<VkDescriptorSet> s_DescriptorSetArray;

const VkPipelineCache& VulkanGraphicsResourcePipelineManager::GetPipelineCache()
{
    return s_PipelineCache;
}

const VkDescriptorPool& VulkanGraphicsResourcePipelineManager::GetDescriptorPool(int index)
{
    return s_DescriptorPoolArray[index];
}

int VulkanGraphicsResourcePipelineManager::CreateGfxFence()
{
    auto createInfo = VkFenceCreateInfo();
    createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    createInfo.pNext = NULL;
    createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    auto fence = VkFence();
    auto result = vkCreateFence(VulkanGraphicsResourceDevice::GetLogicalDevice(), &createInfo, NULL, &fence);

    if (result)
    {
        printf_console("[VulkanGraphics] failed to create a fence with error code %d\n", result);

        throw;
    }

    s_GfxFenceArray.push_back(fence);

    return s_GfxFenceArray.size() - 1;
}

const VkFence& VulkanGraphicsResourcePipelineManager::GetGfxFence(int index)
{
    return s_GfxFenceArray[index];
}

void VulkanGraphicsResourcePipelineManager::DestroyGfxFence(int index)
{
    vkDestroyFence(VulkanGraphicsResourceDevice::GetLogicalDevice(), s_GfxFenceArray[index], NULL);
    s_GfxFenceArray.erase(s_GfxFenceArray.begin() + index);
}

int VulkanGraphicsResourcePipelineManager::CreateGfxSemaphore()
{
    auto createInfo = VkSemaphoreCreateInfo();
    createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    createInfo.pNext = NULL;
    createInfo.flags = 0;

    auto semaphore = VkSemaphore();
    auto result = vkCreateSemaphore(VulkanGraphicsResourceDevice::GetLogicalDevice(), &createInfo, NULL, &semaphore);

    if (result)
    {
        printf_console("[VulkanGraphics] failed to create a semaphore with error code %d\n", result);

        throw;
    }

    s_GfxSemaphoreArray.push_back(semaphore);

    return s_GfxSemaphoreArray.size() - 1;
}

const VkSemaphore& VulkanGraphicsResourcePipelineManager::GetGfxSemaphore(int index)
{
    return s_GfxSemaphoreArray[index];
}

void VulkanGraphicsResourcePipelineManager::DestroyGfxSemaphore(int index)
{
    vkDestroySemaphore(VulkanGraphicsResourceDevice::GetLogicalDevice(), s_GfxSemaphoreArray[index], NULL);
    s_GfxSemaphoreArray.erase(s_GfxSemaphoreArray.begin() + index);
}

// TODO: we need to make this feasible to support variable cases...
int VulkanGraphicsResourcePipelineManager::CreateDescriptorSetLayout()
{
    std::vector<VkDescriptorSetLayoutBinding> descSetLayoutBindingArray;
    {
        auto binding = VkDescriptorSetLayoutBinding();
        binding.binding = 0;
        binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        binding.descriptorCount = 1;
        binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        binding.pImmutableSamplers = NULL;
        descSetLayoutBindingArray.push_back(binding);
    }

    {
        auto binding = VkDescriptorSetLayoutBinding();
        binding.binding = 1;
        binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        binding.descriptorCount = 1;
        binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        binding.pImmutableSamplers = NULL;
        descSetLayoutBindingArray.push_back(binding);
    }

    auto createInfo = VkDescriptorSetLayoutCreateInfo();
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    createInfo.pNext = NULL;
    createInfo.flags = 0; // TODO: this also seems useful... but still don't understand what this is for exactly...
    createInfo.bindingCount = descSetLayoutBindingArray.size();
    createInfo.pBindings = descSetLayoutBindingArray.data();

    auto descSetLayout = VkDescriptorSetLayout();
    auto result = vkCreateDescriptorSetLayout(VulkanGraphicsResourceDevice::GetLogicalDevice(), &createInfo, NULL, &descSetLayout);

    if (result)
    {
        printf_console("[VulkanGraphics] failed to create a descriptor set layout with error code %d\n", result);

        throw;
    }

    s_DescriptorSetLayoutArray.push_back(descSetLayout);

    return s_DescriptorSetLayoutArray.size() - 1;
}

void VulkanGraphicsResourcePipelineManager::DestroyDescriptorSetLayout(int index)
{
    vkDestroyDescriptorSetLayout(VulkanGraphicsResourceDevice::GetLogicalDevice(), s_DescriptorSetLayoutArray[index], NULL);
    s_DescriptorSetLayoutArray.erase(s_DescriptorSetLayoutArray.begin() + index);
}

int VulkanGraphicsResourcePipelineManager::CreatePushConstantRange(VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size)
{
    auto pushConstantRange = VkPushConstantRange();
    pushConstantRange.stageFlags = stageFlags;
    pushConstantRange.offset = offset;
    pushConstantRange.size = size;
    s_PushConstantRangeArray.push_back(pushConstantRange);

    return s_PushConstantRangeArray.size() - 1;
}

void VulkanGraphicsResourcePipelineManager::DestroyPushConstantRange(int index)
{
    s_PushConstantRangeArray.erase(s_PushConstantRangeArray.begin() + index);
}

int VulkanGraphicsResourcePipelineManager::CreatePipelineLayout(std::vector<int> desciptorSetLayoutIndexArray, std::vector<int> pushConstantRangeIndexArray)
{
    std::vector<VkDescriptorSetLayout> descriptorSetLayoutArray;

    for (auto index : desciptorSetLayoutIndexArray)
    {
        descriptorSetLayoutArray.push_back(s_DescriptorSetLayoutArray[index]);
    }

    std::vector<VkPushConstantRange> pushConstantRangeArray;
    
    for (auto index : pushConstantRangeIndexArray)
    {
        pushConstantRangeArray.push_back(s_PushConstantRangeArray[index]);
    }

    auto createInfo = VkPipelineLayoutCreateInfo();
    createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    createInfo.pNext = NULL;
    createInfo.flags = 0;
    createInfo.setLayoutCount = descriptorSetLayoutArray.size();
    createInfo.pSetLayouts = descriptorSetLayoutArray.data();
    createInfo.pushConstantRangeCount = pushConstantRangeArray.size();
    createInfo.pPushConstantRanges = pushConstantRangeArray.data();

    auto pipelineLayout = VkPipelineLayout();
    auto result = vkCreatePipelineLayout(VulkanGraphicsResourceDevice::GetLogicalDevice(), &createInfo, NULL, &pipelineLayout);

    if (result)
    {
        printf_console("[VulkanGraphics] failed to create a pipeline layout with error code %d\n", result);

        throw;
    }

    s_PipelineLayoutArray.push_back(pipelineLayout);

    return s_PipelineLayoutArray.size() - 1;
}

const VkPipelineLayout& VulkanGraphicsResourcePipelineManager::GetPipelineLayout(int index)
{
    return s_PipelineLayoutArray[index];
}

void VulkanGraphicsResourcePipelineManager::DestroyPipelineLayout(int index)
{
    vkDestroyPipelineLayout(VulkanGraphicsResourceDevice::GetLogicalDevice(), s_PipelineLayoutArray[index], NULL);
    s_PipelineLayoutArray.erase(s_PipelineLayoutArray.begin() + index);
}

void VulkanGraphicsResourcePipelineManager::BeginToCreateGraphicsPipeline()
{
    s_GraphicsPipelineCreateInfoArray.clear();
}

int VulkanGraphicsResourcePipelineManager::CreateGraphicsPipeline(const size_t vertexShaderIdentifier, const size_t fragmentShaderIdentifier, int pipelineLayoutIndex, int renderPassIndex, int subPassIndex)
//int VulkanGraphicsResourcePipelineManager::CreateGraphicsPipeline(int vertexShaderModuleIndex, int fragmentShaderModuleIndex, int pipelineLayoutIndex, int renderPassIndex, int subPassIndex)
{
    VkResult result;

    static std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfoArray;
    shaderStageCreateInfoArray.clear();
    {
        auto stageCreateInfo = VkPipelineShaderStageCreateInfo();
        stageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stageCreateInfo.pNext = NULL;
        stageCreateInfo.flags = 0;
        stageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        stageCreateInfo.module = VulkanGraphicsResourceShaderManager::GetInstance().GetResource(vertexShaderIdentifier);
        stageCreateInfo.pName = "main";
        stageCreateInfo.pSpecializationInfo = NULL; // TODO: find out what this is for...
        shaderStageCreateInfoArray.push_back(stageCreateInfo);
    }
    {
        auto stageCreateInfo = VkPipelineShaderStageCreateInfo();
        stageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stageCreateInfo.pNext = NULL;
        stageCreateInfo.flags = 0;
        stageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stageCreateInfo.module = VulkanGraphicsResourceShaderManager::GetInstance().GetResource(fragmentShaderIdentifier);
        stageCreateInfo.pName = "main";
        stageCreateInfo.pSpecializationInfo = NULL; // TODO: find out what this is for...
        shaderStageCreateInfoArray.push_back(stageCreateInfo);
    }

    // TODO: need to support multiple vertex buffers...(NECESSARY!!!)
    static std::vector<VkVertexInputBindingDescription> vertexInputBindingDescArray;
    vertexInputBindingDescArray.clear();
    {
        auto bindingDesc = VkVertexInputBindingDescription();
        bindingDesc.binding = 0;
        bindingDesc.stride = sizeof(VertexData);
        bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        vertexInputBindingDescArray.push_back(bindingDesc);
    }

    // TODO: need to support more various vertex channel like a vertex color...(NECESSARY!!!)
    static std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescArray;
    vertexInputAttributeDescArray.clear();
    {
        auto attributeDesc = VkVertexInputAttributeDescription();
        attributeDesc.location = 0;
        attributeDesc.binding = 0;
        attributeDesc.format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDesc.offset = offsetof(VertexData, m_Position);
        vertexInputAttributeDescArray.push_back(attributeDesc);
    }
    {
        auto attributeDesc = VkVertexInputAttributeDescription();
        attributeDesc.location = 1;
        attributeDesc.binding = 0;
        attributeDesc.format = VK_FORMAT_R32G32_SFLOAT;
        attributeDesc.offset = offsetof(VertexData, m_UV);
        vertexInputAttributeDescArray.push_back(attributeDesc);
    }
    {
        auto attributeDesc = VkVertexInputAttributeDescription();
        attributeDesc.location = 2;
        attributeDesc.binding = 0;
        attributeDesc.format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDesc.offset = offsetof(VertexData, m_Normal);
        vertexInputAttributeDescArray.push_back(attributeDesc);
    }
    {
        auto attributeDesc = VkVertexInputAttributeDescription();
        attributeDesc.location = 3;
        attributeDesc.binding = 0;
        attributeDesc.format = VK_FORMAT_R32_SFLOAT;
        attributeDesc.offset = offsetof(VertexData, m_Material);
        vertexInputAttributeDescArray.push_back(attributeDesc);
    }

    static auto vertexInputStateCreateInfo = VkPipelineVertexInputStateCreateInfo();
    vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputStateCreateInfo.pNext = NULL;
    vertexInputStateCreateInfo.flags = 0;
    vertexInputStateCreateInfo.vertexBindingDescriptionCount = vertexInputBindingDescArray.size();
    vertexInputStateCreateInfo.pVertexBindingDescriptions = vertexInputBindingDescArray.data();
    vertexInputStateCreateInfo.vertexAttributeDescriptionCount = vertexInputAttributeDescArray.size();
    vertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexInputAttributeDescArray.data();

    static auto inputAssemblyState = VkPipelineInputAssemblyStateCreateInfo();
    inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyState.pNext = NULL;
    inputAssemblyState.flags = 0;
    inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // TODO: need to support other types of topology...
    inputAssemblyState.primitiveRestartEnable = VK_FALSE; // TODO: find out when this is needed...

    // Tessellation?

    uint32_t swapchainWidth, swapchainHeight;
    VulkanGraphicsResourceSwapchain::GetSwapchainSize(swapchainWidth, swapchainHeight);

    static std::vector<VkViewport> viewportArray;
    viewportArray.clear();
    {
        auto viewport = VkViewport();
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = swapchainWidth;
        viewport.height = swapchainHeight;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        viewportArray.push_back(viewport);
    }

    static std::vector<VkRect2D> scissorArray;
    scissorArray.clear();
    {
        auto scissor = VkRect2D();
        scissor.offset = { 0, 0 };
        scissor.extent = { swapchainWidth, swapchainHeight };
        scissorArray.push_back(scissor);
    }

    static auto viewportState = VkPipelineViewportStateCreateInfo();
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.pNext = NULL;
    viewportState.flags = 0;
    viewportState.viewportCount = viewportArray.size(); // TODO: it might be needed when starting to consider VR...
    viewportState.pViewports = viewportArray.data();
    viewportState.scissorCount = scissorArray.size(); // TODO: it might be needed when starting to consider VR...
    viewportState.pScissors = scissorArray.data();

    static auto rasterizationState = VkPipelineRasterizationStateCreateInfo();
    rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationState.pNext = NULL;
    rasterizationState.flags = 0;
    rasterizationState.depthClampEnable = VK_FALSE;
    rasterizationState.rasterizerDiscardEnable = VK_FALSE; // TODO: this seems to be used when there is no Geometry Shader and Tessellation...
    rasterizationState.polygonMode = VK_POLYGON_MODE_FILL; // TODO: there are additional modes, one of these is drawing in wire frame mode...
    rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizationState.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizationState.depthBiasEnable = VK_FALSE; // TODO: this will be used in shadow and decal rendering
    rasterizationState.depthBiasConstantFactor = 0.0f;
    rasterizationState.depthBiasClamp = 0.0f;
    rasterizationState.depthBiasSlopeFactor = 0.0f;
    rasterizationState.lineWidth = 1.0f;

    static auto multisampleState = VkPipelineMultisampleStateCreateInfo();
    multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleState.pNext = NULL;
    multisampleState.flags = 0;
    multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT; // TODO: consider MSAA in the future(NECESSARY!!!)...
    multisampleState.sampleShadingEnable = VK_FALSE; // TODO: let's figure out what this is...
    multisampleState.minSampleShading = 1.0f;
    multisampleState.pSampleMask = NULL;
    multisampleState.alphaToCoverageEnable = VK_FALSE;
    multisampleState.alphaToOneEnable = VK_FALSE;

    static auto depthStencilState = VkPipelineDepthStencilStateCreateInfo();
    depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilState.pNext = NULL;
    depthStencilState.flags = 0;
    depthStencilState.depthTestEnable = VK_TRUE;
    depthStencilState.depthWriteEnable = VK_TRUE;
    depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencilState.depthBoundsTestEnable = VK_FALSE; // TODO: let's find out use-cases of this functionality
    depthStencilState.stencilTestEnable = VK_FALSE; // TODO: leave it until we need to use stencil test...
    depthStencilState.front = VkStencilOpState{}; 
    depthStencilState.back = VkStencilOpState{};
    depthStencilState.minDepthBounds = 0.0f;
    depthStencilState.maxDepthBounds = 1.0f;

    // TODO: we should seperate transparent from opaque. for now we consider all objects are oppaque. (NECESSARY!!!)
    static std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachmentStateArray;
    colorBlendAttachmentStateArray.clear();
    {
        auto attachmentState = VkPipelineColorBlendAttachmentState();
        attachmentState.blendEnable = VK_FALSE;
        attachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        attachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        attachmentState.colorBlendOp = VK_BLEND_OP_ADD;
        attachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        attachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        attachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
        attachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachmentStateArray.push_back(attachmentState);
    }

    static auto colorBlendState = VkPipelineColorBlendStateCreateInfo();
    colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendState.pNext = NULL;
    colorBlendState.flags = 0;
    colorBlendState.logicOpEnable = VK_FALSE; // TODO: what can be proper use-cases for this functionalty...
    colorBlendState.logicOp = VK_LOGIC_OP_CLEAR;
    colorBlendState.attachmentCount = colorBlendAttachmentStateArray.size();
    colorBlendState.pAttachments = colorBlendAttachmentStateArray.data();
    colorBlendState.blendConstants[0] = 0.0f;
    colorBlendState.blendConstants[1] = 0.0f;
    colorBlendState.blendConstants[2] = 0.0f;
    colorBlendState.blendConstants[3] = 0.0f;

    static std::vector<VkDynamicState> dynamicStateArray;
    dynamicStateArray.clear();
    {
        // TODO: there are bunch of useful states for example viewport, scissor, vertex input binding stride and etc....(NECESSARY!!!)
    }

    static auto dynamicState = VkPipelineDynamicStateCreateInfo();
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.pNext = NULL;
    dynamicState.flags = 0;
    dynamicState.dynamicStateCount = dynamicStateArray.size();
    dynamicState.pDynamicStates = dynamicStateArray.data();

    static auto createInfo = VkGraphicsPipelineCreateInfo();
    createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    createInfo.pNext = NULL;
    createInfo.flags = 0; // TODO: check out other options...
    createInfo.stageCount = shaderStageCreateInfoArray.size();
    createInfo.pStages = shaderStageCreateInfoArray.data();
    createInfo.pVertexInputState = &vertexInputStateCreateInfo;
    createInfo.pInputAssemblyState = &inputAssemblyState;
    createInfo.pTessellationState = NULL; // TODO: we need this in the future...
    createInfo.pViewportState = &viewportState;
    createInfo.pRasterizationState = &rasterizationState;
    createInfo.pMultisampleState = &multisampleState;
    createInfo.pDepthStencilState = &depthStencilState;
    createInfo.pColorBlendState = &colorBlendState;
    createInfo.pDynamicState = NULL; // &dynamicState;
    createInfo.layout = s_PipelineLayoutArray[pipelineLayoutIndex];
    createInfo.renderPass = VulkanGraphicsResourceRenderPassManager::GetRenderPass(renderPassIndex);
    createInfo.subpass = (uint32_t)subPassIndex;
    createInfo.basePipelineHandle = VK_NULL_HANDLE; // TODO: in the future we might need to use this for optimization purpose...
    createInfo.basePipelineIndex = -1; // TODO: in the future we might need to use this for optimization purpose...
    s_GraphicsPipelineCreateInfoArray.push_back(createInfo);

    return s_GraphicsPipelineArray.size() + s_GraphicsPipelineCreateInfoArray.size() - 1;
}

void VulkanGraphicsResourcePipelineManager::EndToCreateGraphicsPipeline()
{
    int offset = s_GraphicsPipelineArray.size();
    s_GraphicsPipelineArray.resize(s_GraphicsPipelineArray.size() + s_GraphicsPipelineCreateInfoArray.size());
    
    auto result = vkCreateGraphicsPipelines(VulkanGraphicsResourceDevice::GetLogicalDevice(), s_PipelineCache, s_GraphicsPipelineCreateInfoArray.size(), s_GraphicsPipelineCreateInfoArray.data(), NULL, s_GraphicsPipelineArray.data() + offset);

    if (result)
    {
        printf_console("[VulkanGraphics] failed to create graphics pipelines with error code %d\n", result);

        throw;
    }

    s_GraphicsPipelineCreateInfoArray.clear();
}

const VkPipeline& VulkanGraphicsResourcePipelineManager::GetGraphicsPipeline(int index)
{
    return s_GraphicsPipelineArray[index];
}

void VulkanGraphicsResourcePipelineManager::DestroyGraphicsPipeline(int index)
{
    vkDestroyPipeline(VulkanGraphicsResourceDevice::GetLogicalDevice(), s_GraphicsPipelineArray[index], NULL);
    s_GraphicsPipelineArray.erase(s_GraphicsPipelineArray.begin() + index);
}

// TODO: we will implement the function in which we can create any descriptor freely
int VulkanGraphicsResourcePipelineManager::CreateDescriptorPool(const std::vector<VkDescriptorPoolSize>& poolSizeArray)
{
    auto createInfo = VkDescriptorPoolCreateInfo();
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    createInfo.pNext = NULL;
    createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    createInfo.maxSets = 2;
    createInfo.poolSizeCount = poolSizeArray.size();
    createInfo.pPoolSizes = poolSizeArray.data();

    auto descriptorPool = VkDescriptorPool();
    auto result = vkCreateDescriptorPool(VulkanGraphicsResourceDevice::GetLogicalDevice(), &createInfo, NULL, &descriptorPool);

    if (result)
    {
        printf_console("[VulkanGraphics] failed to create a descriptor pool with error code %d\n", result);

        throw;
    }

    s_DescriptorPoolArray.push_back(descriptorPool);

    return s_DescriptorPoolArray.size() - 1;
}

void VulkanGraphicsResourcePipelineManager::DestroyDescriptorPool(int index)
{
    vkDestroyDescriptorPool(VulkanGraphicsResourceDevice::GetLogicalDevice(), s_DescriptorPoolArray[index], NULL);
    s_DescriptorPoolArray.erase(s_DescriptorPoolArray.begin() + index);
}

// TODO: we will implement the function in which we can create any descriptor freely
int VulkanGraphicsResourcePipelineManager::AllocateDescriptorSet(int poolIndex, const VkDescriptorSetLayout& descriptorSetLayout)
{
    auto allocateInfo = VkDescriptorSetAllocateInfo();
    allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.pNext = NULL;
    allocateInfo.descriptorPool = s_DescriptorPoolArray[poolIndex];
    allocateInfo.descriptorSetCount = 1;
    allocateInfo.pSetLayouts = &descriptorSetLayout;

    auto descriptorSet = VkDescriptorSet();
    auto result = vkAllocateDescriptorSets(VulkanGraphicsResourceDevice::GetLogicalDevice(), &allocateInfo, &descriptorSet);

    if (result)
    {
        printf_console("[VulkanGraphics] failed to allocate a descriptor set with error code %d\n", result);

        throw;
    }

    s_DescriptorSetArray.push_back(descriptorSet);

    return s_DescriptorSetArray.size() - 1;
}

void VulkanGraphicsResourcePipelineManager::UpdateDescriptorSet(int index, int binding, const VkImageView& imageView, const VkSampler& sampler)
{
    std::vector<VkWriteDescriptorSet> writeSetArray;
    {
        auto imageInfo = VkDescriptorImageInfo();
        imageInfo.sampler = sampler;
        imageInfo.imageView = imageView;
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        auto writeSet = VkWriteDescriptorSet();
        writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeSet.pNext = NULL;
        writeSet.dstSet = s_DescriptorSetArray[index];
        writeSet.dstBinding = binding;
        writeSet.dstArrayElement = 0;
        writeSet.descriptorCount = 1;
        writeSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeSet.pImageInfo = &imageInfo;
        writeSet.pBufferInfo = NULL;
        writeSet.pTexelBufferView = NULL;
        writeSetArray.push_back(writeSet);
    }

    vkUpdateDescriptorSets(VulkanGraphicsResourceDevice::GetLogicalDevice(), 1, writeSetArray.data(), 0, NULL);
}

void VulkanGraphicsResourcePipelineManager::ReleaseDescriptorSet(int index)
{
    int poolIndex = s_DescriptorPoolIndexArray[index];
    vkFreeDescriptorSets(VulkanGraphicsResourceDevice::GetLogicalDevice(), s_DescriptorPoolArray[poolIndex], 1, &s_DescriptorSetArray[index]);
    s_DescriptorPoolIndexArray.erase(s_DescriptorPoolIndexArray.begin() + index);
    s_DescriptorSetArray.erase(s_DescriptorSetArray.begin() + index);
}

const std::vector<VkDescriptorSet>& VulkanGraphicsResourcePipelineManager::GetDescriptorSetArray()
{
    return s_DescriptorSetArray;
}

bool VulkanGraphicsResourcePipelineManager::CreateInternal()
{
	std::vector<uint8_t> dataArray;
	FILE* file = fopen(PIPELINE_CACHE_DATA_FILE_NAME, "rb");

	if (file)
	{
		fseek(file, 0, SEEK_END);

		size_t dataSize = ftell(file);
		dataArray.resize(dataSize);
		rewind(file);

		size_t readSize = fread(dataArray.data(), 1, dataSize, file);

		if (dataSize != readSize)
		{
			dataArray.clear();
		}

		fclose(file);
	}

	if (dataArray.size() > 0)
	{
        uint8_t* pStart = dataArray.data();
        uint32_t headerLength = 0;
        uint32_t cacheHeaderVersion = 0;
        uint32_t vendorID = 0;
        uint32_t deviceID = 0;
        uint8_t pipelineCacheUUID[VK_UUID_SIZE] = {};

        memcpy(&headerLength, pStart + 0, 4);
        memcpy(&cacheHeaderVersion, pStart + 4, 4);
        memcpy(&vendorID, pStart + 8, 4);
        memcpy(&deviceID, pStart + 12, 4);
        memcpy(pipelineCacheUUID, pStart + 16, VK_UUID_SIZE);

        // Check each field and report bad values before freeing existing cache
        bool badCache = false;

        if (headerLength <= 0) 
        {
            badCache = true;
            printf_console("  Bad header length.\n");
            printf_console("    Cache contains: 0x%.8x\n", headerLength);
        }

        if (cacheHeaderVersion != VK_PIPELINE_CACHE_HEADER_VERSION_ONE) 
        {
            badCache = true;
            printf_console("  Unsupported cache header version.\n");
            printf_console("    Cache contains: 0x%.8x\n", cacheHeaderVersion);
        }

        auto deviceProperties = VulkanGraphicsResourceDevice::GetPhysicalDeviceProperties();

        if (vendorID != deviceProperties.vendorID) 
        {
            badCache = true;
            printf_console("  Vendor ID mismatch.\n");
            printf_console("    Cache contains: 0x%.8x\n", vendorID);
            printf_console("    Driver expects: 0x%.8x\n", deviceProperties.vendorID);
        }

        if (deviceID != deviceProperties.deviceID) 
        {
            badCache = true;
            printf_console("  Device ID mismatch.\n");
            printf_console("    Cache contains: 0x%.8x\n", deviceID);
            printf_console("    Driver expects: 0x%.8x\n", deviceProperties.deviceID);
        }

        if (memcmp(pipelineCacheUUID, deviceProperties.pipelineCacheUUID, sizeof(pipelineCacheUUID)) != 0) 
        {
            badCache = true;
            printf_console("  UUID mismatch.\n");
        }

        if (badCache) 
        {
            printf_console("[VulkanGraphics] loaded an invalid pipeline cache.\n");
            dataArray.clear();
            //throw;  // TODO: need to check why invalid cache is created...(NECESSARY!!!)
        }
	}


	auto createInfo = VkPipelineCacheCreateInfo();
    createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    createInfo.pNext = NULL;
    createInfo.flags = 0; // TODO: check out what each option means...
    createInfo.initialDataSize = dataArray.size();
    createInfo.pInitialData = dataArray.data();

	auto result = vkCreatePipelineCache(VulkanGraphicsResourceDevice::GetLogicalDevice(), &createInfo, NULL, &s_PipelineCache);

    if (result)
    {
        printf_console("[VulkanGraphics] failed to create a pipeline cache with error code %d\n", result);
        throw;
    }

	//vkCreateGraphicsPipelines(VulkanGraphicsResourceDevice::GetLogicalDevice(), /*cache*/, /*count*/, /*pInfos*/, NULL, /*pPipeline*/);

	return true;
}

bool VulkanGraphicsResourcePipelineManager::DestroyInternal()
{
    std::vector<uint8_t> dataArray;
    size_t size;
    vkGetPipelineCacheData(VulkanGraphicsResourceDevice::GetLogicalDevice(), s_PipelineCache, &size, NULL);
    dataArray.resize(size);
    vkGetPipelineCacheData(VulkanGraphicsResourceDevice::GetLogicalDevice(), s_PipelineCache, &size, dataArray.data());

    FILE* file = fopen(PIPELINE_CACHE_DATA_FILE_NAME, "wb");

    if (file)
    {
        fwrite(dataArray.data(), 1, size, file);
        fclose(file);
    }

    // descriptor sets will be released when deleting descriptor pools
    s_DescriptorPoolIndexArray.clear();
    s_DescriptorSetArray.clear();

    for (auto descPool : s_DescriptorPoolArray)
    {
        vkDestroyDescriptorPool(VulkanGraphicsResourceDevice::GetLogicalDevice(), descPool, NULL);
    }

    s_DescriptorPoolArray.clear();

    for (auto pipeline : s_GraphicsPipelineArray)
    {
        vkDestroyPipeline(VulkanGraphicsResourceDevice::GetLogicalDevice(), pipeline, NULL);
    }

    s_GraphicsPipelineArray.clear();

    for (auto descSetLayout : s_DescriptorSetLayoutArray)
    {
        vkDestroyDescriptorSetLayout(VulkanGraphicsResourceDevice::GetLogicalDevice(), descSetLayout, NULL);
    }

    s_DescriptorSetLayoutArray.clear();

    for (auto pipelineLayout : s_PipelineLayoutArray)
    {
        vkDestroyPipelineLayout(VulkanGraphicsResourceDevice::GetLogicalDevice(), pipelineLayout, NULL);
    }

    s_PipelineLayoutArray.clear();
    s_PushConstantRangeArray.clear();

    for (auto semaphore : s_GfxSemaphoreArray)
    {
        vkDestroySemaphore(VulkanGraphicsResourceDevice::GetLogicalDevice(), semaphore, NULL);
    }

    s_GfxSemaphoreArray.clear();

    for (auto fence : s_GfxFenceArray)
    {
        vkDestroyFence(VulkanGraphicsResourceDevice::GetLogicalDevice(), fence, NULL);
    }

    s_GfxFenceArray.clear();
    vkDestroyPipelineCache(VulkanGraphicsResourceDevice::GetLogicalDevice(), s_PipelineCache, NULL);

	return true;
}
