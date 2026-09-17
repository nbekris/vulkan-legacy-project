
#include <array>
#include <iostream> 
#include <fstream>  


#ifdef WIN64
#else
#include <unistd.h>
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "vkapp.h"

#include "app.h"

#define GLM_FORCE_RADIANS
#define GLM_SWIZZLE
#include <glm/glm.hpp>

VkApp::VkApp(App* _app) : app(_app)
{
}

void VkApp::createAllVulkanResources()
{
    uint32_t version;
    vkEnumerateInstanceVersion(&version);
    printf("SDK Version: %d.%d.%d\n", VK_API_VERSION_MAJOR(version),
           VK_API_VERSION_MINOR(version), VK_API_VERSION_PATCH(version));

    createInstance();		// -> m_instance
    assert(m_instance);
    createPhysicalDevice();		// -> m_physicalDevice i.e. the GPU
    chooseQueueIndex();              // -> m_graphicsQueueIndex
    createDevice();			// -> m_device
    //getCommandQueue();               // -> m_queue
    //loadExtensions();		// Auto generated; loads namespace of all known extensions
    //getSurface(); 			// -> m_surface

    //createSwapchain();		// -> m_swapchain
    //createCommandPool();		// -> m_cmdPool..
    //createDepthResource();    	// -> m_depthImage, ...
    //createRenderTarget();   	  	// -> m_renderTarget
    //createPostDescriptor();     	// -> m_postDesc
    //createPostPipeline();       	// -> m_postPipelineLayout

    #ifdef GUI
    initGUI();
    #endif

    // Load model and create related entities
    // createModel();
    // createMatrixBuffer();
    // createObjDescriptionBuffer();
    
    // Scanline: Initialize scanline capabilities
    // createScDescriptorSet();
    // createScPipeline();

    // Raycasting: Initialize ray tracing capabilities
    // createRtBuffers();
    // initRayTracing();
    // createRtAccelerationStructure();
    // createRtDescriptorSet();
    // createRtPipeline();
    // createRtShaderBindingTable();

    // Denoising: Initialize denoising capabilities
    // createDenoiseBuffer();
    // createDenoiseDescriptorSet();
    // createDenoiseCompPipeline();

}

void VkApp::destroyAllVulkanResources()
{
    // @@  Uncomment these 3 lines when directed to do so at the end of project 1.
    //vkWaitForFences(m_device, 1, &m_inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
    //vkResetFences(m_device, 1, &m_inFlightFences[currentFrame]);
    //vkDeviceWaitIdle(m_device);
    
    #ifdef GUI
    ImGui_ImplVulkan_Shutdown();
    vkDestroyDescriptorPool(m_device, m_imguiDescPool, nullptr);
    #endif

    // Destroy here all Vulkan objects as directed by various @@ comments.
    
    // All objects created on m_device must be destroyed before m_device.
    // vkDestroyDevice(m_device, nullptr);
    // vkDestroyInstance(m_instance, nullptr);
}

void VkApp::recreateSizedVulkanResources()
{
    vkDeviceWaitIdle(m_device);

    // @@ Delete this throw when you are ready to handle resize events:
    throw std::runtime_error("Not ready for resize events.");

    // When directed to @@ RESIZE a Vulkan resource,
    //   place the destroy command here, and
    //   place the (re)create command where indicated below.

    // Destroy commands go below here:
    // Destroy commands go above here
    
    destroySwapchain();
    
    // Get new window size.
    int width = 0, height = 0;
    glfwGetFramebufferSize(app->GLFW_window, &width, &height);

    // If the window is minimized, wait for it to be restored.
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(app->GLFW_window, &width, &height);
        glfwWaitEvents(); }

    createSwapchain(); // Updates m_windowSize for the newly resized window

    currentFrame = 0; //  Not sure why I need this.  Ii suspect a bug ... somewhere ...
    m_commandBuffer = m_commandBuffers[currentFrame];
    
    // All (re)create commands go below here:
    // All (re)create commands go above here

}
 
#ifdef GUI
void VkApp::initGUI()
{
    m_ImGuiContext = ImGui::CreateContext();
    ImGui::SetCurrentContext(m_ImGuiContext); 
    
    ImGuiIO& io    = ImGui::GetIO();
    io.LogFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;

    ImGui_ImplGlfw_InitForVulkan(app->GLFW_window, true);
    
    // Init structure for ImGui_ImplVulkan_Init call
    std::vector<VkDescriptorPoolSize> poolSizes{
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,	IMGUI_IMPL_VULKAN_MINIMUM_SAMPLED_IMAGE_POOL_SIZE },
        { VK_DESCRIPTOR_TYPE_SAMPLER,		IMGUI_IMPL_VULKAN_MINIMUM_SAMPLER_POOL_SIZE },
    };

    VkDescriptorPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.poolSizeCount = poolSizes.size();
    poolInfo.pPoolSizes    = poolSizes.data();
    poolInfo.maxSets = 0;
    for (VkDescriptorPoolSize& poolSize : poolSizes)
        poolInfo.maxSets += poolSize.descriptorCount;
    vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_imguiDescPool);
    NAME(m_imguiDescPool, VK_OBJECT_TYPE_DESCRIPTOR_POOL, "imguiDescPool");

    // Setup Platform/Renderer back ends
    ImGui_ImplVulkan_InitInfo init_info{};
    init_info.Instance                  = m_instance;
    init_info.PhysicalDevice            = m_physicalDevice;
    init_info.Device                    = m_device;
    init_info.QueueFamily               = m_graphicsQueueIndex;
    init_info.Queue                     = m_queue;
    init_info.PipelineCache             = VK_NULL_HANDLE;
    init_info.DescriptorPool            = m_imguiDescPool;
    init_info.MinImageCount             = 2;
    init_info.ImageCount                = m_imageCount;
    init_info.CheckVkResultFn           = nullptr;
    init_info.Allocator                 = nullptr;
    
    init_info.UseDynamicRendering   	= true;

    init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.PipelineInfoMain.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    init_info.PipelineInfoMain.PipelineRenderingCreateInfo.pNext = nullptr; 
    init_info.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1; 
    init_info.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = &m_surfaceFormat;

    // Define depth/stencil states if your UI draw context uses them (otherwise leave UNDEFINED)
    init_info.PipelineInfoMain.PipelineRenderingCreateInfo.depthAttachmentFormat   = VK_FORMAT_D32_SFLOAT;
    //init_info.PipelineInfoMain.PipelineRenderingCreateInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

    ImGui_ImplVulkan_Init(&init_info);

}
#endif
