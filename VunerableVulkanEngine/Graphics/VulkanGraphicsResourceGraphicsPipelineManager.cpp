#include "VulkanGraphicsResourceGraphicsPipelineManager.h"
#include "VulkanGraphicsResourceDevice.h"
#include "VulkanGraphicsResourcePipelineCache.h"
#include "VulkanGraphicsResourceShaderManager.h"
#include "VulkanGraphicsResourceSwapchain.h"
#include "VulkanGraphicsResourceDescriptorSetLayoutManager.h"
#include "VulkanGraphicsResourcePipelineLayoutManager.h"
#include "VulkanGraphicsResourceRenderPassManager.h"

VulkanGraphicsResourceGraphicsPipelineManager g_Instance;

VulkanGraphicsResourceGraphicsPipelineManager& VulkanGraphicsResourceGraphicsPipelineManager::GetInstance()
{
	return g_Instance;
}

VulkanGraphicsPipelineOutputData VulkanGraphicsResourceGraphicsPipelineManager::CreateResourcePhysically(const VulkanGraphicsPipelineInputData& inputData)
{
    auto outputData = VulkanGraphicsPipelineOutputData();
    auto pipelineLayoutInputData = VulkanPipelineLayoutInputData();

    for (int i = 0; i < EVulkanShaderType::MAX; ++i)
    {
        if (inputData.m_ShaderIdentifiers[i] == -1)
        {
            continue;
        }

        auto shaderMetaData = VulkanGraphicsResourceShaderManager::GetInstance().GetResourceKey(inputData.m_ShaderIdentifiers[i]);

        if (shaderMetaData.DoesHaveInputBinding())
        {
            size_t identifier = VulkanGraphicsResourceDescriptorSetLayoutManager::GetInstance().AllocateIdentifier();
            outputData.m_DescriptorSetLayoutIdentifiers[i] = identifier;
            VulkanGraphicsResourceDescriptorSetLayoutManager::GetInstance().CreateResource(identifier, shaderMetaData, shaderMetaData);
            pipelineLayoutInputData.m_DescriptorSetLayoutArray.push_back(VulkanGraphicsResourceDescriptorSetLayoutManager::GetInstance().GetResource(identifier));
        }

        if (shaderMetaData.DoesHavePushConstant())
        {
            auto pushConstantRange = VkPushConstantRange();
            pushConstantRange.stageFlags = shaderMetaData.GetShaderStageBits();
            pushConstantRange.offset = shaderMetaData.m_PushConstantOffset;
            pushConstantRange.size = shaderMetaData.m_PushConstantSize;
            pipelineLayoutInputData.m_PushConstantRangeArray.push_back(pushConstantRange);
        }
    }
    
    auto pipelineLayout = VkPipelineLayout();
    
    {
        size_t identifier = VulkanGraphicsResourcePipelineLayoutManager::GetInstance().AllocateIdentifier();
        outputData.m_PipelineLayoutIdentifier = identifier;
        VulkanGraphicsResourcePipelineLayoutManager::GetInstance().CreateResource(identifier, pipelineLayoutInputData, pipelineLayoutInputData);
        pipelineLayout = VulkanGraphicsResourcePipelineLayoutManager::GetInstance().GetResource(identifier);
    }

	auto createInfo = GenerateCreateInfo(inputData, pipelineLayout);
	auto result = vkCreateGraphicsPipelines(VulkanGraphicsResourceDevice::GetLogicalDevice(), VulkanGraphicsResourcePipelineCache::GetInstance().GetPipelineCache(), 1, &createInfo, NULL, &outputData.m_Pipeline);

    if (result)
    {
        printf_console("[VulkanGraphics] failed to create graphics pipelines with error code %d\n", result);

        throw;
    }
    
    return outputData;
}

void VulkanGraphicsResourceGraphicsPipelineManager::DestroyResourcePhysicially(const VulkanGraphicsPipelineOutputData& outputData)
{
    vkDestroyPipeline(VulkanGraphicsResourceDevice::GetLogicalDevice(), outputData.m_Pipeline, NULL);
    
    if (outputData.m_PipelineLayoutIdentifier != -1)
    {
        size_t identifier = outputData.m_PipelineLayoutIdentifier;
        VulkanGraphicsResourcePipelineLayoutManager::GetInstance().DestroyResource(identifier);
        VulkanGraphicsResourcePipelineLayoutManager::GetInstance().ReleaseIdentifier(identifier);
    }

    for (int i = 0; i < EVulkanShaderType::MAX; ++i)
    {
        size_t identifier = outputData.m_DescriptorSetLayoutIdentifiers[i];

        if (identifier == -1)
        {
            continue;
        }

        VulkanGraphicsResourceDescriptorSetLayoutManager::GetInstance().DestroyResource(identifier);
        VulkanGraphicsResourceDescriptorSetLayoutManager::GetInstance().ReleaseIdentifier(identifier);
    }
}

VkPipelineShaderStageCreateInfo CreateShaderStageInfo(const size_t& shaderIdentifier)
{
    auto metaData = VulkanGraphicsResourceShaderManager::GetInstance().GetResourceKey(shaderIdentifier);
    auto info = VkPipelineShaderStageCreateInfo();
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    info.pNext = NULL;
    info.flags = 0;
    info.stage = metaData.GetShaderStageBits();
    info.module = VulkanGraphicsResourceShaderManager::GetInstance().GetResource(shaderIdentifier);
    info.pName = "main";
    info.pSpecializationInfo = NULL; // TODO: find out what this is for...

    return info;
}

VkGraphicsPipelineCreateInfo VulkanGraphicsResourceGraphicsPipelineManager::GenerateCreateInfo(const VulkanGraphicsPipelineInputData& inputData, const VkPipelineLayout& pipelineLayout)
{
    static std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfoArray;
    shaderStageCreateInfoArray.clear();

    for (int i = 0; i < EVulkanShaderType::MAX; ++i)
    {
        if (inputData.m_ShaderIdentifiers[i] == -1)
        {
            continue;
        }

        shaderStageCreateInfoArray.push_back(CreateShaderStageInfo(inputData.m_ShaderIdentifiers[i]));
    }

    // TODO: need to support multiple vertex buffers...(NECESSARY!!!)
    static std::vector<VkVertexInputBindingDescription> vertexInputBindingDescArray;
    vertexInputBindingDescArray.clear();

    // TODO: need to support more various vertex channel like a vertex color...(NECESSARY!!!)
    static std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescArray;
    vertexInputAttributeDescArray.clear();

    if (inputData.m_ShaderIdentifiers[EVulkanShaderType::VERTEX] != -1)
    {
        auto metaData = VulkanGraphicsResourceShaderManager::GetInstance().GetResourceKey(inputData.m_ShaderIdentifiers[EVulkanShaderType::VERTEX]);

        if (metaData.DoesHaveVertexInput())
        {
            auto bindingDescription = VkVertexInputBindingDescription();
            bindingDescription.binding = 0;
            
            uint32_t totalSize = 0;

            for (int i = 0; i < metaData.m_VertexInputArray.size(); ++i)
            {
                auto vertexInput = metaData.m_VertexInputArray[i];

                if (vertexInput <= EVulkanShaderVertexInput::NONE || vertexInput >= EVulkanShaderVertexInput::MAX)
                {
                    continue;
                }

                auto attributeDescription = VkVertexInputAttributeDescription();
                attributeDescription.location = i;
                attributeDescription.binding = 0;

                switch (vertexInput)
                {
                    case EVulkanShaderVertexInput::VECTOR1:
                        attributeDescription.format = VK_FORMAT_R32_SFLOAT;
                        attributeDescription.offset = totalSize;
                        totalSize += 4 * 1;
                        break;

                    case EVulkanShaderVertexInput::VECTOR2:
                        attributeDescription.format = VK_FORMAT_R32G32_SFLOAT;
                        attributeDescription.offset = totalSize;
                        totalSize += 4 * 2;
                        break;

                    case EVulkanShaderVertexInput::VECTOR3:
                        attributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
                        attributeDescription.offset = totalSize;
                        totalSize += 4 * 3;
                        break;

                    case EVulkanShaderVertexInput::VECTOR4:
                        attributeDescription.format = VK_FORMAT_R32G32B32A32_SFLOAT;
                        attributeDescription.offset = totalSize;
                        totalSize += 4 * 4;
                        break;

                    default:
                        printf_console("[VulkanGraphics] invalid vertex input while creating gfx pipeline %d\n", vertexInput);
                        throw;
                }

                vertexInputAttributeDescArray.push_back(attributeDescription);
            }

            bindingDescription.stride = totalSize;
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            vertexInputBindingDescArray.push_back(bindingDescription);
        }
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
    createInfo.layout = pipelineLayout;
    createInfo.renderPass = VulkanGraphicsResourceRenderPassManager::GetRenderPass(inputData.m_RenderPassIndex);
    createInfo.subpass = inputData.m_SubPassIndex;
    createInfo.basePipelineHandle = VK_NULL_HANDLE; // TODO: in the future we might need to use this for optimization purpose...
    createInfo.basePipelineIndex = -1; // TODO: in the future we might need to use this for optimization purpose...

    return createInfo;
}