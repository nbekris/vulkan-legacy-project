
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <array>
#include <math.h>

#include "vkapp.h"

#define GLM_FORCE_RADIANS
#define GLM_SWIZZLE
#include <glm/glm.hpp>
using namespace glm;

#include "app.h"
#include "shaders/shared_structs.h"

void VkApp::createRtBuffers()
{
    // Create m_rtColCurrBuffer for the ray tracer output, and eventually 5 more buffers.
    
    VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT;
    VkImageUsageFlags flags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    VkMemoryPropertyFlags mem = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    VkImageAspectFlagBits aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    VkImageLayout layout = VK_IMAGE_LAYOUT_GENERAL;

    initImageWrap(m_rtColCurrBuffer, m_windowSize, format, flags, mem, aspect, layout);
    // @@ Eventually there will be 5 more buffers similarly created, and named:
    //    m_rtColPrevBuffer,
    //    m_rtKdCurrBuffer, 
    //    m_rtKdPrevBuffer, 
    //    m_rtNdCurrBuffer, 
    //    m_rtNdPrevBuffer, 

    // @@ Find and complete destroyRtBuffers() below.
    // @@ Destroy the rt buffers with destroyRtBuffers()
    // @@ Resize with destroyRtBuffers() and createRtBuffers() in recreateSizedVulkanResources.
}

void VkApp::destroyRtBuffers()
{
    // @@ Destroy m_rtColCurrBuffer with m_rtColCurrBuffer.destroy(m_device)
    //   and eventually destroy the 5 other buffers.
}

// Initialize ray tracing
void VkApp::initRayTracing()
{
    m_pcRay.exposure = 2.0;
    
    // Requesting ray tracing properties
    VkPhysicalDeviceProperties2 prop2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR rtProps
        {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR};
    prop2.pNext = &rtProps;
    vkGetPhysicalDeviceProperties2(m_physicalDevice, &prop2);

    handleSize      = rtProps.shaderGroupHandleSize;
    handleAlignment = rtProps.shaderGroupHandleAlignment;
    baseAlignment   = rtProps.shaderGroupBaseAlignment;

    // This initializes the acceleration structure helper class
    m_rtBuilder.setup(this, m_device, m_graphicsQueueIndex);

    // @@ *HERE* Destroy the temporary m_rtBuilder with m_rtBuilder.destroy().
    // 
    // There is some question whether this destroy can go here (since
    // the builder structure has finished it's job of building the
    // acceleration structure), or at program shutdown (when
    // everything else is destroyed).  Both options work for me, but
    // one student found validation errors if done here.
}

//-------------------------------------------------------------------------------------------------
// This descriptor set holds the Acceleration structure and the output image
//
void VkApp::createRtDescriptorSet()
{
    m_rtDesc.setBindings(m_device, {
            {0, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1,  // TLAS
             VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR},
            {1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1,  // Col output image
             VK_SHADER_STAGE_RAYGEN_BIT_KHR},
        });
    

    // Note: This will grow to include more buffers.

    m_rtDesc.write(m_device, 0, m_rtBuilder.getAccelerationStructure());
    m_rtDesc.write(m_device, 1, m_rtColCurrBuffer.Descriptor());
    // @@ Destroy the descriptor set with: m_rtDesc.destroy(m_device)
    // @@ Resize m_rtDesc;  This might not be necessary.  Please experiment and tell me the result.
}

// Pipeline for the ray tracer: all shaders, raygen, chit, miss
//
void VkApp::createRtPipeline()
{
    ////////////////////////////////////////////////////////////////////////////////////////////
    // stages: Array of shaders: 1 raygen, 1 miss, 1 hit (later: an additional hit/miss pair.)

    ////////////////////////////////////////////////////////////////////////////////////////////
    // Group the shaders.  Raygen and miss shaders get their own
    // groups. Hit shaders can group with any-hit and intersection
    // shaders -- of which we have none -- so the hit shader(s) get
    // their own group also.
    std::vector<VkPipelineShaderStageCreateInfo> stages{};
    std::vector<VkRayTracingShaderGroupCreateInfoKHR> groups{};

    VkPipelineShaderStageCreateInfo stage{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    stage.pName = "main";  // All the same entry point

    VkRayTracingShaderGroupCreateInfoKHR group
        {VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR};
    group.anyHitShader       = VK_SHADER_UNUSED_KHR;
    group.closestHitShader   = VK_SHADER_UNUSED_KHR;
    group.generalShader      = VK_SHADER_UNUSED_KHR;
    group.intersectionShader = VK_SHADER_UNUSED_KHR;

    // Raygen shader stage and group appended to stages and groups lists
    stage.module = createShaderModuleFromFile("spv/raytrace.rgen.spv");
    stage.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    stages.push_back(stage);
    
    group.type          = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    group.generalShader = stages.size()-1;    // Index of raygen shader
    groups.push_back(group);
    group.generalShader    = VK_SHADER_UNUSED_KHR;
    
    // Miss shader stage and group appended to stages and groups lists
    stage.module = createShaderModuleFromFile("spv/raytrace.rmiss.spv");
    stage.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
    stages.push_back(stage);
    
    group.type          = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    group.generalShader = stages.size()-1;    // Index of miss shader
    groups.push_back(group);
    group.generalShader    = VK_SHADER_UNUSED_KHR;
    
    // Closest hit shader stage and group appended to stages and groups lists
    stage.module = createShaderModuleFromFile("spv/raytrace.rchit.spv");
    stage.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    stages.push_back(stage);

    group.type             = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
    group.closestHitShader = stages.size()-1;   // Index of hit shader
    groups.push_back(group);

    ////////////////////////////////////////////////////////////////////////////////////////////
    // Create the ray tracing pipeline layout.
    // Push constant: we want to be able to update constants used by the shaders
    VkPushConstantRange pushConstant{VK_SHADER_STAGE_RAYGEN_BIT_KHR
        | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR,
        0, sizeof(PushConstantRay)};

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo
        {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pipelineLayoutCreateInfo.pPushConstantRanges    = &pushConstant;

    // Descriptor sets: one specific to ray tracing, and one shared with the rasterization pipeline
    std::vector<VkDescriptorSetLayout> rtDescSetLayouts =
        {m_rtDesc.descSetLayout, m_scDesc.descSetLayout};
    pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(rtDescSetLayouts.size());
    pipelineLayoutCreateInfo.pSetLayouts = rtDescSetLayouts.data();

    vkCreatePipelineLayout(m_device, &pipelineLayoutCreateInfo, nullptr, &m_rtPipelineLayout);

    ////////////////////////////////////////////////////////////////////////////////////////////
    // Create the ray tracing pipeline.
    // Assemble the shader stages and recursion depth info into the ray tracing pipeline
    VkRayTracingPipelineCreateInfoKHR rayPipelineInfo
        {VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR};
    rayPipelineInfo.stageCount = static_cast<uint32_t>(stages.size());  // Stages are shaders
    rayPipelineInfo.pStages    = stages.data();

    rayPipelineInfo.groupCount = static_cast<uint32_t>(groups.size());
    rayPipelineInfo.pGroups    = groups.data();
    
    rayPipelineInfo.layout = m_rtPipelineLayout;

    // What the hell!  The raytracing recursion depth is initialized
    // to zero on at least one Vulkan implementation.  A Linux laptop
    // with Nvidia 3060 doesn't seem to care but a Windows desktop
    // with Nvidia 2080 very much fails to ray trace properly if left
    // at zero.
    rayPipelineInfo.maxPipelineRayRecursionDepth = 10;  // Max ray recursion depth

    vkCreateRayTracingPipelinesKHR(m_device, {}, {}, 1, &rayPipelineInfo, nullptr, &m_rtPipeline);

    // The shaders are now copied into the pipeline and can be deleted here immediately:
    for (auto& s : stages)
        vkDestroyShaderModule(m_device, s.module, nullptr);

    // @@ Destroy pipeline and its layout with
    //      vkDestroyPipelineLayout(m_device, m_rtPipelineLayout, nullptr);
    //      vkDestroyPipeline(m_device, m_rtPipeline, nullptr);
    // @@ Resize both m_rtPipelineLayout and m_rtPipeline.
}

//--------------------------------------------------------------------------------------------------
// The Shader Binding Table (SBT)
// - getting all shader handles and write them in a SBT buffer
//
template <class integral>
constexpr integral align_up(integral x, size_t a) noexcept
{
    return integral((x + (integral(a) - 1)) & ~integral(a - 1));
}

void VkApp::createRtShaderBindingTable()
{
    uint32_t missCount{1};
    uint32_t hitCount{1};

    uint32_t handleCount = 1 + missCount + hitCount;

    // The SBT (buffer) needs to have starting group to be aligned
    // and handles in the group to be aligned.
    uint32_t handleSizeAligned = align_up(handleSize, handleAlignment);  // handleAlignment==32

    m_rgenRegion.stride = align_up(handleSizeAligned, baseAlignment); //baseAlignment==64
    m_rgenRegion.size = m_rgenRegion.stride;  // The size member must be equal to its stride member
    
    m_missRegion.stride = handleSizeAligned;
    m_missRegion.size   = align_up(missCount * handleSizeAligned, baseAlignment);
    
    m_hitRegion.stride  = handleSizeAligned;
    m_hitRegion.size    = align_up(hitCount * handleSizeAligned, baseAlignment);

    printf("Shader binding table:\n");
    printf("  alignments:\n");
    printf("    handleAlignment: %d\n", handleAlignment);
    printf("    baseAlignment:   %d\n", baseAlignment);
    printf("  counts:\n");
    printf("    miss:   %d\n", missCount);
    printf("    hit:    %d\n", hitCount);
    printf("    handle: %d = 1+missCount+hitCount\n", handleCount);
    printf("  regions stride:size:\n");
    printf("    rgen %2zd:%2zd\n", m_rgenRegion.stride, m_rgenRegion.size);
    printf("    miss %2zd:%2zd\n", m_missRegion.stride, m_missRegion.size);
    printf("    hit  %2ld:%2ld\n", m_hitRegion.stride,  m_hitRegion.size);
    printf("    call %2ld:%2ld\n", m_callRegion.stride, m_callRegion.size);

    // Get the shader group handles.  This is a byte array retrieved
    // from the pipeline.
    uint32_t             dataSize = handleCount * handleSize;
    std::vector<uint8_t> handles(dataSize);
    printf("\n");
    vkGetRayTracingShaderGroupHandlesKHR(m_device, m_rtPipeline,
                                         0, handleCount, dataSize, handles.data());
    // @@ Verify success of vkGetRayTracingShaderGroupHandlesKHR.

    // Allocate a buffer for storing the SBT, and a staging buffer for transferring data to it.
    VkDeviceSize sbtSize = m_rgenRegion.size + m_missRegion.size
        + m_hitRegion.size + m_callRegion.size;
    BufferWrap staging;
    initBufferWrap(staging, sbtSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                   | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    initBufferWrap(m_shaderBindingTableBuff, sbtSize,
                   VK_BUFFER_USAGE_TRANSFER_DST_BIT
                   | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
                   | VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR,
                   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    // Find the SBT addresses of each group
    VkBufferDeviceAddressInfo info = {VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
    info.buffer                    = m_shaderBindingTableBuff.buffer;
    VkDeviceAddress sbtAddress = vkGetBufferDeviceAddress(m_device, &info);
    
    m_rgenRegion.deviceAddress = sbtAddress;
    m_missRegion.deviceAddress = sbtAddress + m_rgenRegion.size;
    m_hitRegion.deviceAddress  = sbtAddress + m_rgenRegion.size + m_missRegion.size;

    // Helper to retrieve the handle data
    auto getHandle = [&](int i) { return handles.data() + i * handleSize; };

    // Map the SBT buffer and write in the handles.
    uint8_t* mappedMemAddress;
    vkMapMemory(m_device, staging.memory, 0, sbtSize, 0, (void**)&mappedMemAddress);
    uint8_t offset = 0;

    // Raygen
    uint32_t handleIdx{0};
    memcpy(mappedMemAddress+offset, getHandle(handleIdx++), handleSize);

    // Miss
    offset = m_rgenRegion.size;
    for(uint32_t c = 0; c < missCount; c++) {
        memcpy(mappedMemAddress+offset, getHandle(handleIdx++), handleSize);
        offset += m_missRegion.stride; }

    // Hit
    offset = m_rgenRegion.size + m_missRegion.size;
    for(uint32_t c = 0; c < hitCount; c++) {
        memcpy(mappedMemAddress+offset, getHandle(handleIdx++), handleSize);
        offset += m_hitRegion.stride; }

    vkUnmapMemory(m_device, staging.memory);
    
    copyBuffer(staging.buffer, m_shaderBindingTableBuff.buffer, sbtSize);

    staging.destroy(m_device);

    // @@ Destroy the acceleration structure with m_shaderBindingTableBuff.destroy(m_device);
}

void VkApp::raytrace()
{
    // Fill in the push constant m_pcRay (of class PushConstantRay as
    // defined in shaders/shared_structs.h) for the ray tracing
    // pipeline.  Each of the following projects, RayCasting,
    // PathTracing, History, and Denoise will modify this structure.
    
    // @@ Raycasting:	As directed in the project 3 document, provide 3 temporary light values.
    // @@ Pathtracing:	As directed in the project 4 document, remove 3 temporary light values.
    // @@ History:	As directed in the project 5 document, ...
    // @@ Denoise:	As directed in the project 6 document, ...
    m_pcRay.alignmentTest = 1234;

    // Bind the ray tracing pipeline
    vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_rtPipeline);

    // Bind two descriptor sets (the ray tracing specific one, and the
    // full model descriptor)
    std::vector<VkDescriptorSet> descSets{m_rtDesc.descSet, m_scDesc.descSet};
    vkCmdBindDescriptorSets(m_commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
                            m_rtPipelineLayout, 0,
                            descSets.size(), descSets.data(),
                            0, nullptr);

    // Push the push constants
    vkCmdPushConstants(m_commandBuffer, m_rtPipelineLayout,
                       VK_SHADER_STAGE_RAYGEN_BIT_KHR
                       | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR
                       | VK_SHADER_STAGE_MISS_BIT_KHR,
                       0, sizeof(PushConstantRay), &m_pcRay);
    m_pcRay.clear = false;  // Allow accumulation after at least one path tracing pass.

    // This dispatches the ray generation shader for each pixel on screen.
    vkCmdTraceRaysKHR(m_commandBuffer, &m_rgenRegion, &m_missRegion, &m_hitRegion,
                      &m_callRegion, m_windowSize.width, m_windowSize.height, 1);
    frameCount++;

    
    // Copy the ray tracer output image m_rtColCurrBuffer to
    // m_renderTarget which feeds into the Project 1 postProcess step
    // (which outputs to a swapchain image for presentation).
    CmdCopyImage(m_rtColCurrBuffer, m_renderTarget);

    // @@ History and Denoising: Copy the three Curr buffers into the three Prev buffers.
    //    m_rtColCurrBuffer ==>  m_rtColPrevBuffer;
    //    m_rtKdCurrBuffer  ==>  m_rtKdPrevBuffer
    //    m_rtNdCurrBuffer  ==>  m_rtNdPrevBuffer
}

