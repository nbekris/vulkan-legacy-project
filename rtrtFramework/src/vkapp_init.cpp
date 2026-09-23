#include <array>
#include <iostream>     // std::cout
#include <fstream>      // std::ifstream

#include <unordered_set>
#include <unordered_map>

#ifdef GUI
#include "backends/imgui_impl_glfw.h"
#include "imgui.h"
#include "backends/imgui_impl_vulkan.h"
#endif

#include "vkapp.h"
#include "app.h"
#include "extensions_vk.hpp"

void VkApp::createInstance()
{
    uint32_t countGLFWextensions{0};
    const char** reqGLFWextensions = glfwGetRequiredInstanceExtensions(&countGLFWextensions);

    // @@ Append each GLFW required extension in reqGLFWextensions to reqInstanceExtensions
    reqInstanceExtensions.insert(
        reqInstanceExtensions.end(),
        reqGLFWextensions,
        reqGLFWextensions + countGLFWextensions
    );

    // Print them out while you are at it
    printf("GLFW required extensions:\n");
    for (uint32_t i = 0; i < countGLFWextensions; ++i) {
        printf("  %s\n", reqGLFWextensions[i]);
    }

    // Retired code:
    // If included, the api_dump layer should be first on reqInstanceLayers
    // if (app->doApiDump)
    //     reqInstanceLayers.insert(reqInstanceLayers.begin(), "VK_LAYER_LUNARG_api_dump");
  
    uint32_t count;
    // @@ Understand the following three step process for requesting a
    // variable length list from Vulkan.  You will use this many more
    // times.
    //   Step 1: Ask for the count but not the data
    //   Step 2: Create a vector<...> of the correct size,
    //           or resize(...) an existing vector<...>.
    //   Step 3: Ask for the data to be filled into the vector<...>
    vkEnumerateInstanceLayerProperties(&count, nullptr);
    std::vector<VkLayerProperties> availableLayers(count);
    vkEnumerateInstanceLayerProperties(&count, availableLayers.data());

    // @@ Print out the availableLayers
    printf("InstanceLayer count: %d\n", count);
    // ...  use availableLayers[i].layerName
    for (uint32_t i = 0; i < availableLayers.size(); ++i) {
        printf("  %s\n", availableLayers[i].layerName);
    }

    // Another three step dance
    vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(count);
    vkEnumerateInstanceExtensionProperties(nullptr, &count, availableExtensions.data());

    // @@ Print out the availableExtensions
    printf("InstanceExtensions count: %d\n", count);
    // ...  use availableExtensions[i].extensionName
    for (uint32_t i = 0; i < availableExtensions.size(); ++i) {
        printf("  %s\n", availableExtensions[i].extensionName);
    }

    VkApplicationInfo applicationInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    applicationInfo.pApplicationName = "rtrt";
    applicationInfo.pEngineName      = "no-engine";
    applicationInfo.apiVersion       = VK_MAKE_VERSION(1, 3, 0); // or VK_API_VERSION_1_3
    // API version 1.3 supports VK_KHR_dynamic_rendering, a feature this app uses.

    VkInstanceCreateInfo instanceCreateInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    instanceCreateInfo.pNext                   = nullptr;
    instanceCreateInfo.pApplicationInfo        = &applicationInfo;
    
    instanceCreateInfo.enabledExtensionCount   = reqInstanceExtensions.size();
    instanceCreateInfo.ppEnabledExtensionNames = reqInstanceExtensions.data();
    
    instanceCreateInfo.enabledLayerCount       = reqInstanceLayers.size();
    instanceCreateInfo.ppEnabledLayerNames     = reqInstanceLayers.data();

    //vkCreateInstance(&instanceCreateInfo, nullptr, &m_instance);

    // @@ Verify success of vkCreateInstance like this:
    if (vkCreateInstance(&instanceCreateInfo, nullptr, &m_instance) != VK_SUCCESS)
        throw std::runtime_error("vkCreateInstance failed.");
    // @@ To destroy: vkDestroyInstance(m_instance, nullptr);
    // @@ Cut-and-paste the above three list printouts into your report.

    //vkDestroyInstance(m_instance, nullptr);
}

void VkApp::createPhysicalDevice()
{
    // Get the GPU list;  Another three-step list retrieval process:
    uint physicalDevicesCount;
    vkEnumeratePhysicalDevices(m_instance, &physicalDevicesCount, nullptr);
    std::vector<VkPhysicalDevice> physicalDevices(physicalDevicesCount);
    vkEnumeratePhysicalDevices(m_instance, &physicalDevicesCount, physicalDevices.data());

    std::vector<uint32_t> compatibleDevices;
  
    printf("%d devices\n", physicalDevicesCount);
    int i = 0;

    // For each GPU:
    for (auto physicalDevice : physicalDevices) {

        // Get the GPU's properties
        VkPhysicalDeviceProperties GPUproperties;
        vkGetPhysicalDeviceProperties(physicalDevice, &GPUproperties);

        // Get the GPU's extension list;  Another three-step list retrieval process:
        uint extCount;
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extCount, nullptr);
        std::vector<VkExtensionProperties> extensionProperties(extCount);
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr,
                                             &extCount, extensionProperties.data());

        std::unordered_set<std::string> extensionNames;
        for (const VkExtensionProperties& property : extensionProperties) {
            extensionNames.insert(property.extensionName);
        }

        bool compatibleGPU = GPUproperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
        bool supportsRequiredExtensions = true;

        for (const char* requiredExtension : reqDeviceExtensions) {
            if (extensionNames.find(requiredExtension) == extensionNames.end()) {
                supportsRequiredExtensions = false;
                break;
            }
        }

        if (compatibleGPU && supportsRequiredExtensions) {
            compatibleDevices.push_back(i);
        }

        ++i;

        // @@ This code is in a loop iterating variable physicalDevice
        // through a list of all physicalDevices(GPUs).  The
        // physicalDevice's properties (GPUproperties) and a list of
        // its extension properties (extensionProperties) are retrieved
        // above, and here we judge if the physicalDevice
        // is compatible with our requirements.

        // We consider a GPU to be compatible if it satisfies both:
        //    GPUproperties.deviceType==VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
        // and
        //    All reqDeviceExtensions can be found in the GPUs extensionProperties list
        //      That is: for all i, there exists a j such that:
        //                 reqDeviceExtensions[i] == extensionProperties[j].extensionName

        //  If a GPU is found to be compatible record its index in compatibleDevices

        // Hint: Instead of a double nested pair of loops consider
        // making an std::unordered_set of all the device's
        // extensionNames and using its "find" method to search for
        // each required extension.
    
    } // End of physicalDevices loop.

    // If no compatible Devices are found, declare failure and abort
    //    (And then find a better GPU for this class.)
    // If several are found, choose one (how?), and tell me about your system.

    if (compatibleDevices.empty()) throw std::runtime_error("No discrete GPU supports all required extensions");

    // Record the chosen device in m_physicalDevice
    // m_physicalDevice = ...
    const uint32_t selectedIndex = compatibleDevices.front();
    m_physicalDevice = physicalDevices[selectedIndex];

    // @@ Document the chosen GPU, and any rejected GPUs.
    VkPhysicalDeviceProperties selectedProperties;
    vkGetPhysicalDeviceProperties(m_physicalDevice, &selectedProperties);
    printf("Selected GPU [%u]: %s\n", selectedIndex, selectedProperties.deviceName);
    
    // Oddly, there is nothing to destroy here.
}

void VkApp::chooseQueueIndex()
{
    VkQueueFlags requiredQueueFlags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT
                                      | VK_QUEUE_TRANSFER_BIT;

    // Retrieve the list of queue families. (3 step.)
    uint32_t mpCount;
    vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &mpCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueProperties(mpCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &mpCount, queueProperties.data());

    // @@ How many queue families does your Vulkan offer?  Which of
    // the three flags does each offer?  Which of them, by index, has
    // the above three required flags?

    // @@ Search the list for (the index of) the first queue family
    // that has the required flags in queueProperties[i].queueFlags.  Record the index in
    // m_graphicsQueueIndex.

    int i = 0;
    for (const auto& queueProperty : queueProperties) {
        if ((queueProperty.queueFlags & requiredQueueFlags) == requiredQueueFlags) {
            m_graphicsQueueIndex = i;
            break;
        }
        ++i;
    }

    // Nothing to destroy as m_graphicsQueueIndex is just an integer.
}


void VkApp::createDevice()
{
    // @@ Build a pNext chain of the following six "feature" structures:
    //   features2->features11->features12->features13->accelFeature->rtPipelineFeature->NULL

    // Hint: Keep it simple; add a second parameter (for the pNext
    // pointer) to each structure's initializer pointing up to the
    // previous structure.
    
    // =============
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtPipelineFeature{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR,
        NULL
    };
    
    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelFeature{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
        &rtPipelineFeature
    };
    
    VkPhysicalDeviceVulkan13Features features13{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        &accelFeature
    };
    
    VkPhysicalDeviceVulkan12Features features12{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        &features13
    };
    
    VkPhysicalDeviceVulkan11Features features11{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
        &features12
    };
    
    VkPhysicalDeviceFeatures2 features2{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        &features11
    };
    // =============
    
    // Ask Vulkan to fill in all structures on the pNext chain
    vkGetPhysicalDeviceFeatures2(m_physicalDevice, &features2);

    // If this triggers, fall back on an earlier version of this class
    // framework, or use more modern hardware.  Talk to the instructor.
    assert(features13.dynamicRendering == VK_TRUE);

    float priority = 1.0;
    VkDeviceQueueCreateInfo queueInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    queueInfo.queueFamilyIndex = m_graphicsQueueIndex;
    queueInfo.queueCount       = 1;
    queueInfo.pQueuePriorities = &priority;
    
    VkDeviceCreateInfo deviceCreateInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    deviceCreateInfo.pNext            = &features2; // This is the whole pNext chain
  
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos    = &queueInfo;
    
    deviceCreateInfo.enabledExtensionCount   = static_cast<uint32_t>(reqDeviceExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = reqDeviceExtensions.data();

    //vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &m_device);

    // @@ Verify success of vkCreateDevice.
    // To destroy: vkDestroyDevice(m_device, nullptr);
    if (vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &m_device) != VK_SUCCESS)
        throw std::runtime_error("vkCreateDevice failed.");

}

void VkApp::getCommandQueue()
{
    vkGetDeviceQueue(m_device, m_graphicsQueueIndex, 0, &m_queue);
    // Returns void -- nothing to verify
    // Nothing to destroy -- the queue is owned by the device.
}

// Create a command buffer pool, and m_imageCount command buffers from that pool.
void VkApp::createCommandPool()
{
    VkCommandPoolCreateInfo poolCreateInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    poolCreateInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolCreateInfo.queueFamilyIndex = m_graphicsQueueIndex;
    VkResult cmdPoolResult          = vkCreateCommandPool(m_device, &poolCreateInfo, nullptr, &m_cmdPool);

    // Create command buffers
    m_commandBuffers.resize(m_imageCount);
    VkCommandBufferAllocateInfo allocateInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocateInfo.commandPool        = m_cmdPool;
    allocateInfo.commandBufferCount = m_imageCount;
    allocateInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	VkResult cmdBufferResult        = vkAllocateCommandBuffers(m_device, &allocateInfo, m_commandBuffers.data());

    // always keep these two in sync
    currentFrame = 0;
    m_commandBuffer = m_commandBuffers[currentFrame];
    // @@ Verify success of vkCreateCommandPool
	if (cmdPoolResult != VK_SUCCESS) {
		throw std::runtime_error("failed to create command pool!");
	}
    // @@ Verify success of vkAllocateCommandBuffers
	if (cmdBufferResult != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}
    // @@ Destroy: m_cmdPool with vkDestroyCommandPool(m_device, m_cmdPool, nullptr);
    //    No need to destroy m_commandBuffers[] as the pool owns them.
}
 
// Calling load_VK_EXTENSIONS from extensions_vk.cpp.  A Python script
// from NVIDIA created extensions_vk.cpp from the current Vulkan spec
// for the purpose of loading the symbols for all registered
// extension.  This be (indistinguishable from) magic.
void VkApp::loadExtensions()
{
    load_VK_EXTENSIONS(m_instance, vkGetInstanceProcAddr, m_device, vkGetDeviceProcAddr);
}

//  VkSurface is Vulkan's name for the screen.  Since GLFW creates and
//  manages the window, it creates the VkSurface at our request.
void VkApp::getSurface()
{
    VkBool32 isSupported;   // Supports drawing(presenting) on a screen

    // @@ Verify success of glfwCreateWindowSurface.
    if (glfwCreateWindowSurface(m_instance, app->GLFW_window, nullptr, &m_surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
    // @@ Verify success of vkGetPhysicalDeviceSurfaceSupportKHR.
    if (vkGetPhysicalDeviceSurfaceSupportKHR(m_physicalDevice, m_graphicsQueueIndex,
        m_surface, &isSupported) != VK_SUCCESS)
    {
		throw std::runtime_error("failed to get physical device surface support!");
    }
    // @@ Verify isSupported==VK_TRUE, meaning that Vulkan supports presenting on this surface.
    if (isSupported != VK_TRUE) 
    {
		throw std::runtime_error("Vulkan does not support presenting on this surface!");
    }
    // @@ To destroy: vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
}

// 
void VkApp::createSwapchain()
{
    // This code is more disruptive than necessary
    VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE;
    vkDeviceWaitIdle(m_device);
    
    // If creating a new (resized) swapchain, instead of the above, do:
    //  Tell the new swapchain about the old one.
    //  Remove the vkDeviceWaitIdle call
    //  Eventually destroy the old swapchain when all uses of its images have completed.

    // Get the surface's capabilities
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface, &capabilities);

    // Everything related to the swap chain keys off m_imageCount.
    m_imageCount = capabilities.minImageCount;  // Controversial:
    // minImageCount+1 was originally advocated;
    // Later =2, and =3 were suggested.
    // I will choose the minimum until I'm convinced something else matters.
    
    // @@  Roll your own three step process to retrieve a list of PRESENT MODEs into
    //    std::vector<VkPresentModeKHR> presentModes;
    // using vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &count, nullptr);
    // @@ Document your PRESENT MODEs. I especially want to know if
    // your system offers VK_PRESENT_MODE_MAILBOX_KHR mode.  My
    // high-end windows desktop does; My higher-end Linux laptop
    // doesn't.

    uint count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &count, nullptr);
    std::vector<VkPresentModeKHR> presentModes(count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &count, presentModes.data());

	for (const auto& presentMode : presentModes) {
		printf("Present Mode Found: %d\n", presentMode);
	}

    // Choose VK_PRESENT_MODE_FIFO_KHR as a default (this must be supported)
    VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR; // Support is required.
    // @@ But choose VK_PRESENT_MODE_MAILBOX_KHR if it can be found in
    // the retrieved presentModes. Several Vulkan tutorials opine that
    // MODE_MAILBOX is the premier mode, but this may not be best for
    // us.

    for (const auto& availablePresentMode : presentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            swapchainPresentMode = availablePresentMode;
            break;
        }
    }
  
    // Get the list of VkFormat's that are supported:
    // @@ Do the three step process to retrieve a list of surface formats into
    //   std::vector<VkSurfaceFormatKHR> formats;
    // using  vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &count, nullptr);
    // @@ Document the list you get.

    uint formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount, formats.data());

    for (const auto& format : formats) {
		// Print out the format and color space for each surface format
		printf("Surface Format: %d, Color Space: %d\n", format.format, format.colorSpace);
    }

    // @@ Choose the surface format and color space here:
    // Start with the first pair
    m_surfaceFormat = formats[0].format;
    m_surfaceColor  = formats[0].colorSpace;

    // @@ Then search the formats (from several lines up) to choose
    // format VK_FORMAT_B8G8R8A8_UNORM (and its color space) if such
    // exists.  Document your list of formats/color-spaces, and your
    // particular choice.

    for (const auto& availableFormat : formats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            m_surfaceFormat = availableFormat.format;
            m_surfaceColor = availableFormat.colorSpace;
            printf("Selected Surface Format: %d, Color Space: %d\n", availableFormat.format, availableFormat.colorSpace);
            break;
        }
    }
    
    // Get the swap chain extent
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        m_windowSize = capabilities.currentExtent; }
    else {
        // Does this case ever happen?
        int width, height;
        glfwGetFramebufferSize(app->GLFW_window, &width, &height);

        m_windowSize = VkExtent2D{static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

        m_windowSize.width = std::clamp(m_windowSize.width,
                                           capabilities.minImageExtent.width,
                                           capabilities.maxImageExtent.width);
        m_windowSize.height = std::clamp(m_windowSize.height,
                                            capabilities.minImageExtent.height,
                                            capabilities.maxImageExtent.height); }

    // Test against valid size, typically hit when windows are minimized.
    // The app must prevent triggering this code in such a case
    assert(m_windowSize.width && m_windowSize.height);
    // If this assert fires, we have some work to do to better deal
    // with the situation.

    // Create the swap chain
    VkImageUsageFlags imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
                                 | VK_IMAGE_USAGE_STORAGE_BIT
                                 | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    
    VkSwapchainCreateInfoKHR createInfo = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    createInfo.surface                  = m_surface;
    createInfo.minImageCount            = m_imageCount;
    createInfo.imageFormat              = m_surfaceFormat;
    createInfo.imageColorSpace          = m_surfaceColor;
    createInfo.imageExtent              = m_windowSize;
    createInfo.imageUsage               = imageUsage;
    createInfo.preTransform             = capabilities.currentTransform;
    createInfo.compositeAlpha           = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.imageArrayLayers         = 1;
    createInfo.imageSharingMode         = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount    = 1;
    createInfo.pQueueFamilyIndices      = &m_graphicsQueueIndex;
    createInfo.presentMode              = swapchainPresentMode;
    createInfo.oldSwapchain             = oldSwapchain;
    createInfo.clipped                  = true;

    // @@ Verify success of vkCreateSwapchainKHR
    if (vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapchain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }
    
    // @@ Do the three step process to retrieve the list of swapchain images into
    //    std::vector<VkImage> m_swapchainImages;
    // using call vkGetSwapchainImagesKHR(m_device, m_swapchain, &m_imageCount, nullptr);
	//std::vector<VkImage> m_swapchainImages;
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &m_imageCount, nullptr);
    m_swapchainImages.resize(m_imageCount);

    // @@ Verify success of vkGetSwapchainImagesKHR
    if (vkGetSwapchainImagesKHR(m_device, m_swapchain, &m_imageCount, m_swapchainImages.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to retrieve swap chain images!");
    }

    // Resize containers for objects that are created for each swapchain image.
    m_imageViews.resize(m_imageCount);
    m_imageAvailableSemaphores.resize(m_imageCount);
    m_renderFinishedSemaphores.resize(m_imageCount);
    m_inFlightFences.resize(m_imageCount);

    // Create an VkImageView for each swap chain image.
    for (uint i=0;  i<m_imageCount;  i++) {
        VkImageViewCreateInfo createInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
            createInfo.image = m_swapchainImages[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = m_surfaceFormat;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            vkCreateImageView(m_device, &createInfo, nullptr, &m_imageViews[i]); }

    // Create the three synchronization objects.  These are not
    // technically part of the swap chain, but they are used
    // exclusively for synchronizing the swap chain, so I include them
    // here.
    for (uint i=0;  i<m_imageCount;  i++) {
        VkFenceCreateInfo fenceCreateInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        vkCreateFence(m_device, &fenceCreateInfo, nullptr, &m_inFlightFences[i]);
        NAME(m_inFlightFences[i], VK_OBJECT_TYPE_FENCE, "m_inFlightFence");
    
        VkSemaphoreCreateInfo semCreateInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        vkCreateSemaphore(m_device, &semCreateInfo, nullptr, &m_imageAvailableSemaphores[i]);
        vkCreateSemaphore(m_device, &semCreateInfo, nullptr, &m_renderFinishedSemaphores[i]);
        NAME(m_imageAvailableSemaphores[i], VK_OBJECT_TYPE_SEMAPHORE, "m_imageAvailableSemaphore");
        NAME(m_renderFinishedSemaphores[i], VK_OBJECT_TYPE_SEMAPHORE, "m_renderFinishedSemaphore");
    }
    
    // @@ Destroying the swapchain is complex enough to warrant a separate procedure.
    //     find destroySwapchain below and follow its instructions.
}

void VkApp::destroySwapchain()
{
    // In a loop (0 to <m_imageCount) destroy ALL m_imageViews and
    // synchronization items before destroying the swapchain itself.
    for (uint32_t i = 0; i < m_imageCount; ++i) {
        vkDestroyImageView(m_device, m_imageViews[i], nullptr);
        vkDestroyFence(m_device, m_inFlightFences[i], nullptr);
        vkDestroySemaphore(m_device, m_imageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(m_device, m_renderFinishedSemaphores[i], nullptr);
    }

    // Destroy the actual swapchain after all of its dependent resources.
    vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);

    m_imageViews.clear();
    m_inFlightFences.clear();
    m_imageAvailableSemaphores.clear();
    m_renderFinishedSemaphores.clear();
    m_swapchainImages.clear();
    m_swapchain = VK_NULL_HANDLE;
    m_imageCount = 0;
}
