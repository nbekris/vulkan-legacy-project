
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>              // for memcpy
#include <vector>
#include <array>
#include <math.h>

#include "vkapp.h"

#define GLM_FORCE_RADIANS
#define GLM_SWIZZLE
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>

using namespace glm;

#define STBI_FAILURE_USERMSG
#include "stb_image.h"

#include "app.h"
#include "shaders/shared_structs.h"

void VkApp::createRenderTarget()
{
    VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT;
    VkImageUsageFlags flags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    VkMemoryPropertyFlags mem = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    VkImageAspectFlagBits aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    VkImageLayout layout = VK_IMAGE_LAYOUT_GENERAL;

    initImageWrap(m_renderTarget, m_windowSize, format, flags, mem, aspect, layout);
    initTextureSampler(m_renderTarget);

    // @@ Destroy m_renderTarget with m_renderTarget.destroy(m_device);
    // @@ Resize m_renderTarget
}

// Create a Vulkan buffer to hold the camera matrices, products and inverses.
// Will be included in a descriptor set for use in shaders.
void VkApp::createMatrixBuffer()
{
    initBufferWrap(m_matrixBuff, sizeof(MatrixUniforms),
                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
                     | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    NAME(m_matrixBuff.buffer, VK_OBJECT_TYPE_BUFFER, "m_matrixBuff.buffer");

    // @@ Destroy with m_matrixBuff.destroy(m_device);
}

// Create a Vulkan buffer containing pointers to all object buffers
// (vertex, triangle indices, materials, and material indices. Will be
// included in a descriptor set for use in shaders.
void VkApp::createObjDescriptionBuffer()
{
    VkCommandBuffer cmdBuf = createTempCmdBuffer();
    initBufferWrapFromData(m_objDescriptionBuff, cmdBuf, m_objDesc,
                           VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    NAME(m_objDescriptionBuff.buffer, VK_OBJECT_TYPE_BUFFER, "m_objDescriptionBuff.buffer");
    submitTempCmdBuffer(cmdBuf);
    // @@ Destroy with m_objDescriptionBuff.destroy(m_device);
}

void VkApp::createScDescriptorSet()
{
    auto nbTxt = static_cast<uint32_t>(m_objText.size());

    // This descriptor set is being created for both the scanline and
    // raytracing pipelines; Note the mention of VERTEX, FRAGMENT, and
    // RAYGEN shader stages.
    m_scDesc.setBindings(m_device, {
            {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR},
            {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
                | VK_SHADER_STAGE_RAYGEN_BIT_KHR},
            {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nbTxt,
                VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR}
        });
              
    m_scDesc.write(m_device, 0, m_matrixBuff.buffer);
    m_scDesc.write(m_device, 1, m_objDescriptionBuff.buffer);
    m_scDesc.write(m_device, 2, m_objText);  

    // @@ Destroy m_scDesc with m_scDesc.destroy(m_device);
}

void VkApp::createScPipeline()
{
    VkPushConstantRange pushConstantRange = {
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstantRaster)};

    // Creating the Pipeline Layout
    VkPipelineLayoutCreateInfo createInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    createInfo.setLayoutCount         = 1;
    createInfo.pSetLayouts            = &m_scDesc.descSetLayout;
    createInfo.pushConstantRangeCount = 1;
    createInfo.pPushConstantRanges    = &pushConstantRange;
    vkCreatePipelineLayout(m_device, &createInfo, nullptr, &m_scPipelineLayout);

    VkShaderModule vertShaderModule = createShaderModuleFromFile("spv/scanline.vert.spv");
    VkShaderModule fragShaderModule = createShaderModuleFromFile("spv/scanline.frag.spv");

    VkPipelineShaderStageCreateInfo
        vertShaderStageInfo{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo
        fragShaderStageInfo{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    VkVertexInputBindingDescription bindingDescription
        {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX};

    std::vector<VkVertexInputAttributeDescription> attributeDescriptions {
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(Vertex, pos))},
        {1, 0, VK_FORMAT_R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(Vertex, nrm))},
        {2, 0, VK_FORMAT_R32G32_SFLOAT, static_cast<uint32_t>(offsetof(Vertex, texCoord))}};

    VkPipelineVertexInputStateCreateInfo
        vertexInputInfo{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        
    vertexInputInfo.vertexAttributeDescriptionCount = attributeDescriptions.size();
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo
        inputAssembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) m_windowSize.width;
    viewport.height = (float) m_windowSize.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = m_windowSize;

    VkPipelineViewportStateCreateInfo
        viewportState{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo
        rasterizer{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE; //??
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo
        multisampling{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo
        depthStencil{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;// BEWARE!!  NECESSARY!!
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo
        colorBlending{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    // For dynamic rendering
    VkFormat cFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
    VkFormat dFormat = VK_FORMAT_D32_SFLOAT;
    
    VkPipelineRenderingCreateInfo drInfo{VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
    drInfo.viewMask = 0;
    drInfo.colorAttachmentCount = 1;
    drInfo.pColorAttachmentFormats = &cFormat;
    drInfo.depthAttachmentFormat = dFormat;
    drInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

    VkGraphicsPipelineCreateInfo pipelineInfo{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pipelineInfo.pNext = &drInfo; // For dynamic rendering
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = m_scPipelineLayout;
    pipelineInfo.renderPass = VK_NULL_HANDLE; // Since we are using dynamic rendering
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
                              &m_scPipeline);
    NAME(m_scPipeline, VK_OBJECT_TYPE_PIPELINE, "scanline pipeline");

    // Done with the temporary spv shader modules.
    vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
    vkDestroyShaderModule(m_device, vertShaderModule, nullptr);

    // @@ Verify success of vkCreateGraphicsPipelines
    // @@ Verify success of vkCreatePipelineLayout (found about 135 lines upward).
    
    // @@ Destroy both:
    //      vkDestroyPipelineLayout(m_device, m_scPipelineLayout, nullptr);
    //      vkDestroyPipeline(m_device, m_scPipeline, nullptr);

    // @@ Resize both m_scPipelineLayout and m_scPipeline.
}

void VkApp::rasterize()
{
    VkDeviceSize offset{0};
    
    VkRenderingAttachmentInfo colorAttachment{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    colorAttachment.imageView   = m_renderTarget.imageView;
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; 
    colorAttachment.resolveMode = VK_RESOLVE_MODE_NONE;
    colorAttachment.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue.color = { 0.0f, 0.0f, 0.0f, 1.0f };
    
    VkRenderingAttachmentInfo depthAttachment{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    depthAttachment.imageView   = m_depthImage.imageView;
    depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depthAttachment.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.clearValue.depthStencil = { 1.0f, 0 };

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = m_windowSize;

    VkRenderingInfo renderingInfo{VK_STRUCTURE_TYPE_RENDERING_INFO};
    renderingInfo.flags                = 0;
    renderingInfo.renderArea           = scissor;
    renderingInfo.layerCount           = 1;
    renderingInfo.viewMask             = 0;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments    = &colorAttachment;
    renderingInfo.pDepthAttachment     = &depthAttachment;
    renderingInfo.pStencilAttachment   = nullptr;

    WrapPipelineBarrier(m_commandBuffer, m_renderTarget.image, VK_IMAGE_ASPECT_COLOR_BIT,
      VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      VK_ACCESS_NONE,
      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
      VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

    WrapPipelineBarrier(m_commandBuffer, m_depthImage.image, VK_IMAGE_ASPECT_DEPTH_BIT,
      VK_IMAGE_LAYOUT_UNDEFINED, 
      VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
      VK_ACCESS_NONE,
      VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
      VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
      VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);
    
    vkCmdBeginRendering(m_commandBuffer, &renderingInfo);
    {   // extra indent for scanline pass commands

        vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_scPipeline);
        vkCmdBindDescriptorSets(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                m_scPipelineLayout, 0, 1, &m_scDesc.descSet, 0, nullptr);

        for(const ObjInst& inst : m_objInst) {
            auto& object            = m_objData[inst.objIndex];
            // Information pushed at each draw call
            PushConstantRaster pcRaster{
                scLightPos,
                scLightInt,
                scLightAmb,
                inst.transform,      // Object's instance transform.
                inst.objIndex       // instance Id
            };
        
            pcRaster.objIndex    = inst.objIndex;  // Telling which object is drawn
            pcRaster.modelMatrix = inst.transform;

            vkCmdPushConstants(m_commandBuffer, m_scPipelineLayout,
                               VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                               sizeof(PushConstantRaster), &pcRaster);
            vkCmdBindVertexBuffers(m_commandBuffer, 0, 1, &object.vertexBuffer.buffer, &offset);
            vkCmdBindIndexBuffer(m_commandBuffer, object.indexBuffer.buffer, 0,
                             VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(m_commandBuffer, object.nbIndices, 1, 0, 0, 0); }
    }    

    vkCmdEndRendering(m_commandBuffer);

    WrapPipelineBarrier(m_commandBuffer, m_renderTarget.image, VK_IMAGE_ASPECT_COLOR_BIT, 
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 
      VK_IMAGE_LAYOUT_GENERAL,
      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      VK_ACCESS_NONE,
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

    WrapPipelineBarrier(m_commandBuffer, m_depthImage.image, VK_IMAGE_ASPECT_DEPTH_BIT, 
      VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, 
      VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
      VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
      VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
      VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
      VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT); 
}


void VkApp::updateCameraBuffer()
{
    // Prepare new UBO contents on host.
    const float    aspectRatio = m_windowSize.width / static_cast<float>(m_windowSize.height);
    glm::mat4    view = app->myCamera.view(glfwGetTime());
    glm::mat4    proj = app->myCamera.perspective(aspectRatio);
    //std::cout << "view: " << glm::to_string(view) << std::endl;
    //std::cout << "proj: " << glm::to_string(proj) << std::endl;
  
    MatrixUniforms hostUBO;
    hostUBO.priorViewProj = m_priorViewProj;
    hostUBO.viewProj    = proj * view;
    m_priorViewProj       = hostUBO.viewProj;
    hostUBO.viewInverse = glm::inverse(view);
    hostUBO.projInverse = glm::inverse(proj);

    // UBO on the device, and what stages access it.
    VkBuffer deviceUBO      = m_matrixBuff.buffer;
    auto     uboUsageStages = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT
                            | VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;

    // Ensure that the modified UBO is not visible to previous frames.
    VkBufferMemoryBarrier beforeBarrier{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
    beforeBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    beforeBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    beforeBarrier.buffer        = deviceUBO;
    beforeBarrier.offset        = 0;
    beforeBarrier.size          = sizeof(hostUBO);
    vkCmdPipelineBarrier(m_commandBuffer, uboUsageStages, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_DEPENDENCY_DEVICE_GROUP_BIT, 0,
                         nullptr, 1, &beforeBarrier, 0, nullptr);


    // Schedule the host-to-device upload. (hostUBO is copied into the cmd
    // buffer so it is okay to deallocate when the function returns).
    vkCmdUpdateBuffer(m_commandBuffer, m_matrixBuff.buffer, 0, sizeof(MatrixUniforms), &hostUBO);

    // Making sure the updated UBO will be visible.
    VkBufferMemoryBarrier afterBarrier{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
    afterBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    afterBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    afterBarrier.buffer        = deviceUBO;
    afterBarrier.offset        = 0;
    afterBarrier.size          = sizeof(hostUBO);
    vkCmdPipelineBarrier(m_commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, uboUsageStages,
                         VK_DEPENDENCY_DEVICE_GROUP_BIT, 0,
                         nullptr, 1, &afterBarrier, 0, nullptr);
}
